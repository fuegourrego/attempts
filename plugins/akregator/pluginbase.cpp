/*
    This file is part of Akregator.

    Copyright (C) 2004 Teemu Rytilahti <tpr@d5k.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include <kprocess.h>
#include <KLocalizedString>
#include <KMessageBox>

#include "feeddetector.h"
#include "pluginbase.h"
#include "akregatorplugindebug.h"

#include <qstringlist.h>
#include <qurl.h>
#include <qdbusconnection.h>
#include <qdbusconnectioninterface.h>
#include <qdbusinterface.h>
#include <qdbusreply.h>

using namespace Akregator;

PluginBase::PluginBase()
{}

PluginBase::~PluginBase()
{}

bool PluginBase::akregatorRunning()
{
    //Laurent if akregator is registered into dbus so akregator is running
    return QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.akregator"));
}

void PluginBase::addFeedsViaDBUS(const QStringList &urls)
{
    qCDebug(AKREGATORPLUGIN_LOG);
    QDBusInterface akregator(QStringLiteral("org.kde.akregator"), QStringLiteral("/Akregator"), QStringLiteral("org.kde.akregator.part"));
    QDBusReply<void> reply  = akregator.call(QStringLiteral("addFeedsToGroup"), urls, i18n("Imported Feeds"));
    if (!reply.isValid()) {
        KMessageBox::error(0, i18n("Akregator feed icon - DBus Call failed"),
                           i18nc("@title:window", "The DBus call addFeedToGroup failed"));
    }
}

void PluginBase::addFeedViaCmdLine(const QString &url)
{
    KProcess proc;
    proc << QStringLiteral("akregator") << QStringLiteral("-g") << i18n("Imported Feeds");
    proc << QStringLiteral("-a") << url;
    proc.startDetached();
}

// handle all the wild stuff that QUrl doesn't handle
QString PluginBase::fixRelativeURL(const QString &s, const QUrl &baseurl)
{
    QString s2 = s;
    QUrl u;
    if (QUrl(s2).isRelative()) {
        if (s2.startsWith(QLatin1String("//"))) {
            s2.prepend(baseurl.scheme() + ':');
            u.setUrl(s2);
        } else if (s2.startsWith(QLatin1String("/"))) {
            // delete path/query/fragment, so that only protocol://host remains
            QUrl b2(baseurl.adjusted(QUrl::RemovePath|QUrl::RemoveQuery|QUrl::RemoveFragment));
            u = b2.resolved(QUrl(s2.mid(1)));		// remove leading "/"
        } else {
            u = baseurl.resolved(QUrl(s2));
        }
    } else {
        u.setUrl(s2);
    }

    u = u.adjusted(QUrl::NormalizePathSegments);
    //qCDebug(AKREGATORPLUGIN_LOG) << "url=" << s << " baseurl=" << baseurl << "fixed=" << u;
    return u.url();
}
