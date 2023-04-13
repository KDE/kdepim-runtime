/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/AgentWidgetConfigurationBase>

#include "akonadi-singlefileresource_export.h"
#include "singlefileresourceconfigwidget.h"

template<typename Settings>
class SingleFileResourceConfigBase : public Akonadi::AgentWidgetConfigurationBase
{
public:
    explicit SingleFileResourceConfigBase(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &list)
        : Akonadi::AgentWidgetConfigurationBase(config, parent, list)
        , mSettings(new Settings(config))
        , mWidget(new Akonadi::SingleFileResourceConfigWidget<Settings>(parent, mSettings.data()))
    {
    }

    ~SingleFileResourceConfigBase() override = default;

    void load() override
    {
        mWidget->load();
        Akonadi::AgentWidgetConfigurationBase::load();
    }

    bool save() const override
    {
        if (!mWidget->save()) {
            return false;
        }
        return Akonadi::AgentWidgetConfigurationBase::save();
    }

protected:
    QScopedPointer<Settings> mSettings;
    QScopedPointer<Akonadi::SingleFileResourceConfigWidget<Settings>> mWidget;
};
