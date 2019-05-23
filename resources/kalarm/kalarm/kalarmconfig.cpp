/*
 *  Copyright © 2018  Daniel Vrátil <dvratil@kde.org>
 *  Copyright © 2019  David Jarvie <djarvie@kde.org>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#include "singlefileresourceconfigbase.h"
#include "settings.h"
#include "alarmtyperadiowidget.h"

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
