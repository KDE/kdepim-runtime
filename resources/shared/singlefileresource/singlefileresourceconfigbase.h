/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef SINGLEFILERESOURCCONFIGBASE_H_
#define SINGLEFILERESOURCCONFIGBASE_H_

#include <AkonadiCore/AgentConfigurationBase>

#include <QVBoxLayout>

#include "akonadi-singlefileresource_export.h"
#include "singlefileresourceconfigwidget.h"

template<typename Settings>
class AKONADI_SINGLEFILERESOURCE_EXPORT SingleFileResourceConfigBase : public Akonadi::AgentConfigurationBase
{
public:
    explicit SingleFileResourceConfigBase(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &list)
        : Akonadi::AgentConfigurationBase(config, parent, list)
        , mSettings(new Settings(config))
        , mWidget(new Akonadi::SingleFileResourceConfigWidget<Settings>(parent, mSettings.data()))
    {
    }

    ~SingleFileResourceConfigBase()
    {
    }

    void load() override
    {
        mWidget->load();
        Akonadi::AgentConfigurationBase::load();
    }

    bool save() const override
    {
        if (!mWidget->save()) {
            return false;
        }
        return Akonadi::AgentConfigurationBase::save();
    }

protected:
    QScopedPointer<Settings> mSettings;
    QScopedPointer<Akonadi::SingleFileResourceConfigWidget<Settings> > mWidget;
};

#endif
