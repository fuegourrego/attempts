/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "konqhistorysettings.h"

// KDE
#include <kconfig.h>
#include <ksharedconfig.h>
#include <kconfiggroup.h>

class KonqHistorySettingsSingleton {
public:
    KonqHistorySettings self;
};

Q_GLOBAL_STATIC(KonqHistorySettingsSingleton, globalHistorySettings)
KonqHistorySettings *KonqHistorySettings::self()
{
    return &globalHistorySettings->self;
}

KonqHistorySettings::KonqHistorySettings()
    : QObject(nullptr)
{
    m_fontOlderThan.setItalic(true);   // default

    new KonqHistorySettingsAdaptor(this);
    const QString dbusPath = QStringLiteral("/KonqHistorySettings");
    const QString dbusInterface = QStringLiteral("org.kde.Konqueror.SidebarHistorySettings");
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(dbusPath, this);
    dbus.connect(QString(), dbusPath, dbusInterface, QStringLiteral("notifySettingsChanged"), this, SLOT(slotSettingsChanged()));

    readSettings(false);
}

KonqHistorySettings::~KonqHistorySettings()
{
}

void KonqHistorySettings::readSettings(bool reparse)
{
    KSharedConfigPtr config = KSharedConfig::openConfig(); // read konquerorrc

    if (reparse) {
        config->reparseConfiguration();
    }

    const KConfigGroup cg(config, "HistorySettings");

    int defaultAction = cg.readEntry("Default Action", 0);
    m_defaultAction = defaultAction <= static_cast<int>(Action::OpenNewWindow) ? static_cast<Action>(defaultAction) : Action::Auto;

    m_valueYoungerThan = cg.readEntry("Value youngerThan", 1);
    m_valueOlderThan = cg.readEntry("Value olderThan", 2);

    const QString days = QStringLiteral("days");
    const QString metricY = cg.readEntry("Metric youngerThan", days);
    m_metricYoungerThan = (metricY == days) ? DAYS : MINUTES;
    const QString metricO = cg.readEntry("Metric olderThan", days);
    m_metricOlderThan = (metricO == days) ? DAYS : MINUTES;

    m_fontYoungerThan = cg.readEntry("Font youngerThan", m_fontYoungerThan);
    m_fontOlderThan   = cg.readEntry("Font olderThan", m_fontOlderThan);

    m_detailedTips = cg.readEntry("Detailed Tooltips", true);
    m_sortsByName = cg.readEntry("SortHistory", "byDate") == QLatin1String("byName");
}

void KonqHistorySettings::applySettings()
{
    KConfigGroup config(KSharedConfig::openConfig(), "HistorySettings"); // write to konquerorrc

    config.writeEntry("Default Action", static_cast<int>(m_defaultAction));

    config.writeEntry("Value youngerThan", m_valueYoungerThan);
    config.writeEntry("Value olderThan", m_valueOlderThan);

    const QString minutes = QStringLiteral("minutes");
    const QString days = QStringLiteral("days");
    config.writeEntry("Metric youngerThan", m_metricYoungerThan == DAYS ?  days : minutes);
    config.writeEntry("Metric olderThan", m_metricOlderThan == DAYS ?  days : minutes);

    config.writeEntry("Font youngerThan", m_fontYoungerThan);
    config.writeEntry("Font olderThan", m_fontOlderThan);

    config.writeEntry("Detailed Tooltips", m_detailedTips);
    config.writeEntry("SortHistory", m_sortsByName ? "byName" : "byDate");

    // notify konqueror instances about the new configuration
    emit notifySettingsChanged();
}

void KonqHistorySettings::slotSettingsChanged()
{
    readSettings(true /*reparse*/);
    emit settingsChanged();
}

