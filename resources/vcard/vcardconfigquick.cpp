// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "vcardconfigquick.h"
#include "settings.h"
#include <KLocalizedString>

VCardConfigQuick::VCardConfigQuick(const KSharedConfigPtr &config, const QString &identifier, QObject *parent)
    : SingleFileResourceConfigQuickBase(config,
                                        identifier,
                                        new Akonadi_VCard_Resource::Settings(config),
                                        {i18nc("@item:inlistbox", "vCard Address Book File (*.vcf)")},
                                        parent)
{
}

AKONADI_AGENTCONFIG_QUICK_FACTORY(VCardConfigQuickFactory, "vcardconfigquick.json", VCardConfigQuick, "org.kde.akonadi.vcardconfig", "ConfigurationPage")

#include "vcardconfigquick.moc"
