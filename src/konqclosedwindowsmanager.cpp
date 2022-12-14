/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2007 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqclosedwindowsmanager.h"
#include "konqsettingsxt.h"
#include "konqmisc.h"
#include "konqcloseditem.h"
#include "konqclosedwindowsmanageradaptor.h"
#include "konqclosedwindowsmanager_interface.h"
#include <kio/fileundomanager.h>
#include <QDirIterator>
#include <QMetaType>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <KLocalizedString>

#include <kconfig.h>
#include <QStandardPaths>
#include <KSharedConfig>

Q_DECLARE_METATYPE(QList<QVariant>)

class KonqClosedWindowsManagerPrivate
{
public:
    KonqClosedWindowsManager instance;
    int m_maxNumClosedItems;
};

static KonqClosedWindowsManagerPrivate *myKonqClosedWindowsManagerPrivate = nullptr;

KonqClosedWindowsManager::KonqClosedWindowsManager()
{
    new KonqClosedWindowsManagerAdaptor(this);

    const QString dbusPath = QStringLiteral("/KonqUndoManager");
    const QString dbusInterface = QStringLiteral("org.kde.Konqueror.UndoManager");

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(dbusPath, this);
    dbus.connect(QString(), dbusPath, dbusInterface, QStringLiteral("notifyClosedWindowItem"), this, SLOT(slotNotifyClosedWindowItem(QString,int,QString,QString,QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, QStringLiteral("notifyRemove"), this, SLOT(slotNotifyRemove(QString,QString,QDBusMessage)));

    QString filename = "closeditems/" + KonqMisc::encodeFilename(dbus.baseService());
    QString file = QDir::tempPath() + QLatin1Char('/') +  filename;
    QFile::remove(file);

    KConfigGroup configGroup(KSharedConfig::openConfig(), "Undo");
    m_numUndoClosedItems = configGroup.readEntry("Number of Closed Windows", 0);

    m_konqClosedItemsConfig = nullptr;
    m_blockClosedItems = false;
    m_konqClosedItemsStore = new KConfig(file, KConfig::SimpleConfig);
}

KonqClosedWindowsManager::~KonqClosedWindowsManager()
{
    // Do some file cleaning
    removeClosedItemsConfigFiles();

    qDeleteAll(m_closedWindowItemList); // must be done before deleting the kconfigs
    delete m_konqClosedItemsConfig;
    delete m_konqClosedItemsStore;
}

KConfig *KonqClosedWindowsManager::memoryStore()
{
    return m_konqClosedItemsStore;
}

KonqClosedWindowsManager *KonqClosedWindowsManager::self()
{
    if (!myKonqClosedWindowsManagerPrivate) {
        myKonqClosedWindowsManagerPrivate = new KonqClosedWindowsManagerPrivate;
    }
    return &myKonqClosedWindowsManagerPrivate->instance;
}

void KonqClosedWindowsManager::destroy()
{
    delete myKonqClosedWindowsManagerPrivate;
    myKonqClosedWindowsManagerPrivate = nullptr;
}

void KonqClosedWindowsManager::addClosedWindowItem(KonqUndoManager
        *real_sender, KonqClosedWindowItem *closedWindowItem, bool propagate)
{
    readConfig();
    // If we are off the limit, remove the last closed window item
    if (m_closedWindowItemList.size() >=
            KonqSettings::maxNumClosedItems()) {
        KonqClosedWindowItem *last = m_closedWindowItemList.last();

        emit removeWindowInOtherInstances(nullptr, last);
        emitNotifyRemove(last);

        m_closedWindowItemList.removeLast();
        delete last;
    }

    if (!m_blockClosedItems) {
        m_numUndoClosedItems++;
        emit addWindowInOtherInstances(real_sender, closedWindowItem);
    }

    // The prepend goes after emit addWindowInOtherInstances() because otherwise
    // the first time addWindowInOtherInstances() is emitted, KonqUndoManager
    // will catch it and it will call to its private populate() function which
    // will add to the undo list closedWindowItem, and then it will add it again
    // but we want it to be added only once.
    m_closedWindowItemList.prepend(closedWindowItem);

    if (propagate) {
        // if it needs to be propagated means that it's a local window and thus
        // we need to call to saveConfig() to keep updated the kconfig file, so
        // that new konqueror instances can read it correctly updated.
        saveConfig();

        // Once saved, tell to other konqi processes
        emitNotifyClosedWindowItem(closedWindowItem);
    }
}

void KonqClosedWindowsManager::removeClosedWindowItem(KonqUndoManager
        *real_sender, const KonqClosedWindowItem *closedWindowItem, bool propagate)
{
    readConfig();
    auto it = std::find(m_closedWindowItemList.begin(), m_closedWindowItemList.end(), closedWindowItem);

    // If the item was found, remove it from the list
    if (it != m_closedWindowItemList.end()) {
        m_closedWindowItemList.erase(it);
        m_numUndoClosedItems--;
    }
    emit removeWindowInOtherInstances(real_sender, closedWindowItem);

    if (propagate) {
        emitNotifyRemove(closedWindowItem);
    }
}

const QList<KonqClosedWindowItem *> &KonqClosedWindowsManager::closedWindowItemList()
{
    readConfig();
    return m_closedWindowItemList;
}

void KonqClosedWindowsManager::readSettings()
{
    readConfig();
}

static QString dbusService()
{
    return QDBusConnection::sessionBus().baseService();
}

/**
 * Returns whether the DBUS call we are handling was a call from us self
 */
bool isSenderOfSignal(const QDBusMessage &msg)
{
    return dbusService() == msg.service();
}

bool isSenderOfSignal(const QString &service)
{
    return dbusService() == service;
}

void KonqClosedWindowsManager::emitNotifyClosedWindowItem(
    const KonqClosedWindowItem *closedWindowItem)
{
    emit notifyClosedWindowItem(closedWindowItem->title(),
                                closedWindowItem->numTabs(),
                                m_konqClosedItemsStore->name(),
                                closedWindowItem->configGroup().name());
}

void KonqClosedWindowsManager::emitNotifyRemove(
    const KonqClosedWindowItem *closedWindowItem)
{
    const KonqClosedRemoteWindowItem *closedRemoteWindowItem =
        dynamic_cast<const KonqClosedRemoteWindowItem *>(closedWindowItem);

    // Here we do this because there's no need to call to configGroup() if it's
    // a remote window item, and it would be error prone to be so, because
    // it could give us a null pointer and konqueror would crash
    if (closedRemoteWindowItem)
        emit notifyRemove(closedRemoteWindowItem->remoteConfigFileName(),
                          closedRemoteWindowItem->remoteGroupName());
    else
        emit notifyRemove(closedWindowItem->configGroup().config()->name(),
                          closedWindowItem->configGroup().name());
}

void KonqClosedWindowsManager::slotNotifyClosedWindowItem(
    const QString &title, int numTabs, const QString &configFileName,
    const QString &configGroup, const QString &service)
{
    if (isSenderOfSignal(service)) {
        return;
    }

    // Create a new ClosedWindowItem and add it to the list
    KonqClosedWindowItem *closedWindowItem = new KonqClosedRemoteWindowItem(
        title, memoryStore(), configGroup, configFileName,
        KIO::FileUndoManager::self()->newCommandSerialNumber(), numTabs,
        service);

    // Add it to all the windows but don't propagate over dbus,
    // as it already comes from dbus)
    addClosedWindowItem(nullptr, closedWindowItem, false);
}

void KonqClosedWindowsManager::slotNotifyClosedWindowItem(
    const QString &title, int numTabs, const QString &configFileName,
    const QString &configGroup, const QDBusMessage &msg)
{
    slotNotifyClosedWindowItem(title, numTabs, configFileName, configGroup,
                               msg.service());
}

void KonqClosedWindowsManager::slotNotifyRemove(
    const QString &configFileName, const QString &configGroup,
    const QDBusMessage &msg)
{
    if (isSenderOfSignal(msg)) {
        return;
    }

    // Find the window item. It can be either remote or local
    KonqClosedWindowItem *closedWindowItem =
        findClosedRemoteWindowItem(configFileName, configGroup);
    if (!closedWindowItem) {
        closedWindowItem = findClosedLocalWindowItem(configFileName, configGroup);
        if (!closedWindowItem) {
            return;
        }
    }

    // Remove it in all the windows but don't propagate over dbus,
    // as it already comes from dbus)
    removeClosedWindowItem(nullptr, closedWindowItem, false);
}

KonqClosedRemoteWindowItem *KonqClosedWindowsManager::findClosedRemoteWindowItem(
    const QString &configFileName,
    const QString &configGroup)
{
    readConfig();

    KonqClosedRemoteWindowItem *closedRemoteWindowItem = nullptr;
    for (QList<KonqClosedWindowItem *>::const_iterator it = m_closedWindowItemList.constBegin();
            it != m_closedWindowItemList.constEnd(); ++it) {
        closedRemoteWindowItem = dynamic_cast<KonqClosedRemoteWindowItem *>(*it);

        if (closedRemoteWindowItem &&
                closedRemoteWindowItem->equalsTo(configFileName, configGroup)) {
            return closedRemoteWindowItem;
        }
    }

    return closedRemoteWindowItem;
}

KonqClosedWindowItem *KonqClosedWindowsManager::findClosedLocalWindowItem(
    const QString &configFileName,
    const QString &configGroup)
{
    readConfig();
    KonqClosedWindowItem *closedWindowItem = nullptr;
    for (QList<KonqClosedWindowItem *>::const_iterator it = m_closedWindowItemList.constBegin();
            it != m_closedWindowItemList.constEnd(); ++it) {
        closedWindowItem = *it;
        KonqClosedRemoteWindowItem *closedRemoteWindowItem =
            dynamic_cast<KonqClosedRemoteWindowItem *>(closedWindowItem);

        if (!closedRemoteWindowItem && closedWindowItem &&
                closedWindowItem->configGroup().config()->name() == configFileName &&
                closedWindowItem->configGroup().name() == configGroup) {
            return closedWindowItem;
        }
    }

    return closedWindowItem;
}

/**
 * @returns the number of konqueror processes by counting the number of
 * org.kde.konqueror services in dbus.
 *
 * If dbus fails it returns -1.
 */
static int numberOfKonquerorProcesses()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    QDBusReply<QStringList> reply = dbus.interface()->registeredServiceNames();
    if (!reply.isValid()) {
        return -1;
    }

    const QStringList allServices = reply;
    int count = 0; // count the number of running konqueror processes. Should be at least one, us.
    for (QStringList::const_iterator it = allServices.begin(), end = allServices.end(); it != end; ++it) {
        const QString service = *it;
        if (service.startsWith(QLatin1String("org.kde.konqueror"))) {
            count++;
        }
    }
    return count;
}

void KonqClosedWindowsManager::removeClosedItemsConfigFiles()
{
    // We'll only remove closed items config files if we are the only process
    // left or if dbus fails (just in case there is any other konqi process
    // but we couldn't see it).
    int count = numberOfKonquerorProcesses();
    if (count > 1 || count == -1) {
        return;
    }

    // We are the only instance of konqueror left and thus we can safely remove
    // all those temporary files.
    QString dir = QDir::tempPath() + QLatin1Char('/') +  "closeditems/";
    QDBusConnectionInterface *idbus = QDBusConnection::sessionBus().interface();
    QDirIterator it(dir, QDir::Writable | QDir::Files);
    while (it.hasNext()) {
        // Only remove the files for those konqueror instances not running anymore
        QString filename = it.next();
        if (!idbus->isServiceRegistered(KonqMisc::decodeFilename(it.fileName()))) {
            QFile::remove(filename);
        }
    }
}

void KonqClosedWindowsManager::saveConfig()
{
    readConfig();

    // Create / overwrite the saved closed windows list
    QString filename = QStringLiteral("closeditems_saved");
    QString file = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + filename;
    QFile::remove(file);

    KConfig *config = new KConfig(file, KConfig::SimpleConfig);

    // Populate the config file
    KonqClosedWindowItem *closedWindowItem = nullptr;
    uint counter = m_closedWindowItemList.size() - 1;
    for (QList<KonqClosedWindowItem *>::const_iterator it = m_closedWindowItemList.constBegin();
            it != m_closedWindowItemList.constEnd(); ++it, --counter) {
        closedWindowItem = *it;
        KConfigGroup configGroup(config, "Closed_Window" + QString::number(counter));
        configGroup.writeEntry("title", closedWindowItem->title());
        configGroup.writeEntry("numTabs", closedWindowItem->numTabs());
        closedWindowItem->configGroup().copyTo(&configGroup);
    }

    KConfigGroup configGroup(KSharedConfig::openConfig(), "Undo");
    configGroup.writeEntry("Number of Closed Windows", m_closedWindowItemList.size());
    configGroup.sync();

    // Finally the most important thing, which is to save the store config
    // so that other konqi processes can reopen windows closed in this process.
    m_konqClosedItemsStore->sync();

    delete config;
}

void KonqClosedWindowsManager ::readConfig()
{
    if (m_konqClosedItemsConfig) {
        return;
    }

    QString filename = QStringLiteral("closeditems_saved");
    QString file = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + filename;

    m_konqClosedItemsConfig = new KConfig(file, KConfig::SimpleConfig);

    // If the config file doesn't exists, there's nothing to read
    if (!QFile::exists(file)) {
        return;
    }

    m_blockClosedItems = true;
    for (int i = 0; i < m_numUndoClosedItems; i++) {
        // For each item, create a new ClosedWindowItem
        KConfigGroup configGroup(m_konqClosedItemsConfig, "Closed_Window" +
                                 QString::number(i));

        // The number of closed items was not correctly set, fix it and save the
        // correct number.
        if (!configGroup.exists()) {
            m_numUndoClosedItems = i;
            KConfigGroup configGroup(KSharedConfig::openConfig(), "Undo");
            configGroup.writeEntry("Number of Closed Windows",
                                   m_closedWindowItemList.size());
            configGroup.sync();
            break;
        }

        QString title = configGroup.readEntry("title", i18n("no name"));
        int numTabs = configGroup.readEntry("numTabs", 0);

        KonqClosedWindowItem *closedWindowItem = new KonqClosedWindowItem(
            title, memoryStore(), i, numTabs);
        configGroup.copyTo(&closedWindowItem->configGroup());
        configGroup.writeEntry("foo", 0);

        // Add the item only to this window
        addClosedWindowItem(nullptr, closedWindowItem, false);
    }

    m_blockClosedItems = false;
}

bool KonqClosedWindowsManager::undoAvailable() const
{
    return m_numUndoClosedItems > 0;
}
