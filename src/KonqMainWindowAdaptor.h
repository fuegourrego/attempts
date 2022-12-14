/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2000, 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __KonqMainWindowAdaptor_h__
#define __KonqMainWindowAdaptor_h__

// !!! Don't regenerate this file, I don't want to lose the method documentation
// Use qdbuscpp2xml KonqMainWindowAdaptor.h > org.kde.Konqueror.MainWindow.xml
// if you change the API.

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QDBusConnection>

class KonqMainWindow;

/**
 * DBUS interface for a konqueror main window
 */
class KonqMainWindowAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Konqueror.MainWindow")

public:

    explicit KonqMainWindowAdaptor(KonqMainWindow *mainWindow);
    ~KonqMainWindowAdaptor() override;

public slots:

    /**
     * Open a url in this window
     * @param url the url to open
     * @param tempFile whether to delete the file after use, usually this is false
     */
    void openUrl(const QString &url, bool tempFile);
    /**
     * Open a url in a new tab in this window
     * @param url the url to open
     * @param tempFile whether to delete the file after use, usually this is false
     */
    void newTab(const QString &url, bool tempFile);

    void newTabASN(const QString &url, const QByteArray &startup_id, bool tempFile);

    void newTabASNWithMimeType(const QString &url, const QString &mimetype, const QByteArray &startup_id, bool tempFile);

    void splitViewHorizontally();
    void splitViewVertically();

    /**
     * Reloads the current view.
     */
    void reload();

    /**
     * @return reference to the current KonqView
     */
    QDBusObjectPath currentView();
    /**
     * @return reference to the current part
     */
    QDBusObjectPath currentPart();

    QDBusObjectPath view(int viewNumber);

    QDBusObjectPath part(int partNumber);

private:

    KonqMainWindow *m_pMainWindow;
};

#endif

