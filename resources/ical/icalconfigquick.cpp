// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "icalconfigquick.h"
#include "settings.h"
#include <KLocalizedString>

ICalConfigQuick::ICalConfigQuick(const KSharedConfigPtr &config, const QString &identifier, QObject *parent)
    : SingleFileResourceConfigQuickBase(config,
                                        identifier,
                                        new Akonadi_ICal_Resource::Settings(config),
                                        {i18nc("@item:inlistbox", "iCalendar File (*.ics)")},
                                        parent)
{
}

AKONADI_AGENTCONFIG_QUICK_FACTORY(ICalConfigQuickFactory, "icalconfigquick.json", ICalConfigQuick, "org.kde.akonadi.icalconfig", "ConfigurationPage")

#include "icalconfigquick.moc"
