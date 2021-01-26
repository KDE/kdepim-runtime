/*
 *  SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>
 *  SPDX-FileCopyrightText: 2019 David Jarvie <djarvie@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "alarmtyperadiowidget.h"
#include "settings.h"
#include "singlefileresourceconfigbase.h"

class KAlarmConfigBase : public SingleFileResourceConfigBase<SETTINGS_NAMESPACE::Settings>
{
public:
    KAlarmConfigBase(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &params)
        : SingleFileResourceConfigBase<SETTINGS_NAMESPACE::Settings>(config, parent, params)
    {
        mTypeSelector.reset(new AlarmTypeRadioWidget(parent));
        const QStringList types = mSettings->alarmTypes();
        CalEvent::Type alarmType = CalEvent::ACTIVE;
        if (!types.isEmpty()) {
            alarmType = CalEvent::type(types[0]);
        }
        mTypeSelector->setAlarmType(alarmType);
        mWidget->appendWidget(mTypeSelector.data());
        mWidget->setMonitorEnabled(false);
    }

    bool save() const override
    {
        mSettings->setAlarmTypes(CalEvent::mimeTypes(mTypeSelector->alarmType()));
        return SingleFileResourceConfigBase<SETTINGS_NAMESPACE::Settings>::save();
    }

private:
    QScopedPointer<AlarmTypeRadioWidget> mTypeSelector;
};

class KAlarmConfig : public KAlarmConfigBase
{
    Q_OBJECT
public:
    using KAlarmConfigBase::KAlarmConfigBase;
};

AKONADI_AGENTCONFIG_FACTORY(KAlarmConfigFactory, "kalarmconfig.json", KAlarmConfig)

#include "kalarmconfig.moc"
