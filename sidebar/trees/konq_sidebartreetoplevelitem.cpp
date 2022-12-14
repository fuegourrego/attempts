/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

//#include "konq_treepart.h"
#include "konq_sidebartreemodule.h"
#include <kdirnotify.h>
#include <kio/paste.h>
#include <kprotocolinfo.h>
#include <k3urldrag.h>
#include <kconfiggroup.h>
#include <kfileitem.h>
#include <kmimetype.h>
#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QMimeData>
#include <kdesktopfile.h>

void KonqSidebarTreeTopLevelItem::init()
{
    QString desktopFile = m_path;
    if (isTopLevelGroup()) {
        desktopFile += "/.directory";
    }
    KDesktopFile cfg(desktopFile);
    m_comment = cfg.desktopGroup().readEntry("Comment");
}

void KonqSidebarTreeTopLevelItem::setOpen(bool open)
{
    if (open && module()) {
        module()->openTopLevelItem(this);
    }
    KonqSidebarTreeItem::setOpen(open);
}

void KonqSidebarTreeTopLevelItem::itemSelected()
{
    qCDebug(SIDEBAR_LOG) << "KonqSidebarTreeTopLevelItem::itemSelected";
    const QMimeData *data = QApplication::clipboard()->mimeData();
    const bool paste = m_bTopLevelGroup && data->hasUrls();
    tree()->enableActions(true, true, paste);
}

bool KonqSidebarTreeTopLevelItem::acceptsDrops(const Q3StrList &formats)
{
    return formats.contains("text/uri-list") &&
           (m_bTopLevelGroup || !externalURL().isEmpty());
}

void KonqSidebarTreeTopLevelItem::drop(QDropEvent *ev)
{
    if (m_bTopLevelGroup) {
        // When dropping something to "Network" or its subdirs, we want to create
        // a desktop link, not to move/copy/link - except for .desktop files :-}
        QList<QUrl> lst;
        if (K3URLDrag::decode(ev, lst) && !lst.isEmpty()) {   // Are they urls ?
            QList<QUrl>::Iterator it = lst.begin();
            for (; it != lst.end(); it++) {
                tree()->addUrl(this, *it);
            }
        } else {
            kError() << "No URL !?  " << endl;
        }
    } else { // Top level item, not group
        if (!externalURL().isEmpty()) {
            KonqOperations::doDrop(KFileItem(), externalURL(), ev, tree());
        }
    }
}

bool KonqSidebarTreeTopLevelItem::populateMimeData(QMimeData *mimeData, bool move)
{
    QList<QUrl> lst;
    lst.append(QUrl::fromLocalFile(path()));

    mimeData->setUrls(lst);
    KIO::setClipboardDataCut(mimeData, move);

#if 0
    const QPixmap *pix = pixmap(0);
    if (pix) {
        QPoint hotspot(pix->width() / 2, pix->height() / 2);
        drag->setPixmap(*pix, hotspot);
    }
#endif

    return true;
}

void KonqSidebarTreeTopLevelItem::middleButtonClicked()
{
    if (!m_bTopLevelGroup) {
        emit tree()->createNewWindow(m_externalURL);
    }
    // Do nothing for toplevel groups
}

void KonqSidebarTreeTopLevelItem::rightButtonPressed()
{
    QUrl url;
    url.setPath(m_path);
    // We don't show "edit file type" (useless here) and "properties" (shows the wrong name,
    // i.e. the filename instead of the Name field). There's the Rename item for that.
    // Only missing thing is changing the URL of a link. Hmm...

    if (!module() || !module()->handleTopLevelContextMenu(this, QCursor::pos())) {
        tree()->showToplevelContextMenu();
    }
}

void KonqSidebarTreeTopLevelItem::trash()
{
    delOperation(KonqOperations::TRASH);
}

void KonqSidebarTreeTopLevelItem::del()
{
    delOperation(KonqOperations::DEL);
}

void KonqSidebarTreeTopLevelItem::delOperation(KonqOperations::Operation method)
{
    QUrl url(m_path);
    QList<QUrl> lst;
    lst.append(url);

    KonqOperations::del(tree(), method, lst);
}

void KonqSidebarTreeTopLevelItem::paste()
{
    // move or not move ?
    bool move = false;
    const QMimeData *data = QApplication::clipboard()->mimeData();
    if (data->hasFormat("application/x-kde-cutselection")) {
        move = KonqMimeData::decodeIsCutSelection(data);
        qCDebug(SIDEBAR_LOG) << "move (from clipboard data) = " << move;
    }

    QUrl destURL;
    if (m_bTopLevelGroup) {
        destURL.setPath(m_path);
    } else {
        destURL = m_externalURL;
    }

    KIO::pasteClipboard(destURL, 0L, move);
}

void KonqSidebarTreeTopLevelItem::rename()
{
    tree()->rename(this, 0);
}

void KonqSidebarTreeTopLevelItem::rename(const QString &name)
{
    QUrl url(m_path);

    // Adjust the Name field of the .directory or desktop file
    QString desktopFile = m_path;
    if (isTopLevelGroup()) {
        desktopFile += "/.directory";
    }
    KDesktopFile cfg(desktopFile);
    cfg.desktopGroup().writeEntry("Name", name);
    cfg.sync();

    // Notify about the change
    QList<QUrl> lst;
    lst.append(url);
    OrgKdeKDirNotifyInterface::emitFilesChanged(lst.toStringList());
}

QString KonqSidebarTreeTopLevelItem::toolTipText() const
{
    return m_comment;
}

