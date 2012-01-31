/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <KDE/KParts/BrowserExtension>
#include <KDE/KWebView>

#include <QtWebKit/QWebHitTestResult>

class KUrl;
class KWebKitPart;
class QWebHitTestResult;
class QWebInspector;

class WebView : public KWebView
{
    Q_OBJECT
public:
    WebView(KWebKitPart* part, QWidget* parent);
    ~WebView();   

    /**
     * Same as QWebPage::load, but with KParts style arguments instead.
     *
     * @see KParts::OpenUrlArguments, KParts::BrowserArguments.
     *
     * @param url     the url to load.
     * @param args    reference to a OpenUrlArguments object.
     * @param bargs   reference to a BrowserArguments object.
     */
    void loadUrl(const KUrl& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& bargs);

    QWebHitTestResult contextMenuResult() const;

protected:
    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWebView::dropEvent
     * @internal
     */
    virtual void dropEvent(QDropEvent*);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::contextMenuEvent
     * @internal
     */
    virtual void contextMenuEvent(QContextMenuEvent*);

    void editableContentActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void selectActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void linkActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void partActionPopupMenu(KParts::BrowserExtension::ActionGroupMap &);
    void multimediaActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);

private Q_SLOTS:
    void slotOpenSelection();

private:
    void addSearchActions(QList<QAction*>& selectActions, QWebView*);

    KActionCollection* m_actionCollection;
    QWebHitTestResult m_result;
    QWeakPointer<KWebKitPart> m_part;
    QWebInspector* m_webInspector;
};

#endif // WEBVIEW_H
