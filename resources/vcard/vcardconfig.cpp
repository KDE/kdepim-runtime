/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "config-kdepim-runtime.h"
#include "settings.h"
#include "singlefileresourceconfigbase.h"
#if HAVE_ACTIVITY_SUPPORT
#include <PimCommonActivities/ConfigureActivitiesWidget>
#endif
class VCardConfigBase : public SingleFileResourceConfigBase<SETTINGS_NAMESPACE::Settings>
{
public:
    VCardConfigBase(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &list)
        : SingleFileResourceConfigBase(config, parent, list)
#if HAVE_ACTIVITY_SUPPORT
        , mConfigureActivitiesWidget(new PimCommonActivities::ConfigureActivitiesWidget(parent))
#endif
    {
        mWidget->setFilter(QStringLiteral("%1 (*.vcf)").arg(i18nc("Filedialog filter for *.vcf", "vCard Address Book File")));
#if HAVE_ACTIVITY_SUPPORT
        mWidget->addPage(i18n("Activities"), mConfigureActivitiesWidget);
#endif
    }

private:
#if HAVE_ACTIVITY_SUPPORT
    PimCommonActivities::ConfigureActivitiesWidget *const mConfigureActivitiesWidget;
#endif
};

class VCardConfig : public VCardConfigBase
{
    Q_OBJECT
public:
    ~VCardConfig() override = default;

    using VCardConfigBase::VCardConfigBase;
};

AKONADI_AGENTCONFIG_FACTORY(VCardConfigFactory, "vcardconfig.json", VCardConfig)

#include "vcardconfig.moc"
