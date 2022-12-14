/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2003 Alexander Kellett <lypanov@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/


#include "kbookmarkmenu.h"
#include "kbookmarkimporter.h"
#include "kbookmarkactioninterface.h"

#include <kactionmenu.h>
#include <QStack>

class QString;
class QMenu;
class KBookmark;
class KBookmarkOwner;

namespace Konqueror { // to avoid clashes with KF5::Bookmarks classes

class KImportedBookmarkMenu : public KBookmarkMenu
{
    friend class KBookmarkMenuImporter;
    Q_OBJECT
public:
    //TODO simplfy
    KImportedBookmarkMenu(KBookmarkManager *mgr,
                          KBookmarkOwner *owner, QMenu *parentMenu,
                          const QString &type, const QString &location);
    KImportedBookmarkMenu(KBookmarkManager *mgr,
                          KBookmarkOwner *owner, QMenu *parentMenu);
    ~KImportedBookmarkMenu() override;
    void clear() override;
    void refill() override;
protected Q_SLOTS:
    void slotNSLoad();
private:
    QString m_type;
    QString m_location;
};


/**
 * A class connected to KNSBookmarkImporter, to fill KActionMenus.
 */
class KBookmarkMenuImporter : public QObject
{
    Q_OBJECT
public:
    KBookmarkMenuImporter(KBookmarkManager *mgr, KImportedBookmarkMenu *menu) :
        m_menu(menu), m_pManager(mgr) {}

    void openBookmarks(const QString &location, const QString &type);
    void connectToImporter(const QObject &importer);

protected Q_SLOTS:
    void newBookmark(const QString &text, const QString &url, const QString &);
    void newFolder(const QString &text, bool, const QString &);
    void newSeparator();
    void endFolder();

protected:
    QStack<KImportedBookmarkMenu *> mstack;
    KImportedBookmarkMenu *m_menu;
    KBookmarkManager *m_pManager;
};

class KImportedBookmarkActionMenu : public KActionMenu, public KBookmarkActionInterface
{
    Q_OBJECT
public:
    KImportedBookmarkActionMenu(const QIcon &icon, const QString &text, QObject *parent)
        : KActionMenu(icon, text, parent),
          KBookmarkActionInterface(KBookmark())
    {
    }
    ~KImportedBookmarkActionMenu() override
    {}
};

}
