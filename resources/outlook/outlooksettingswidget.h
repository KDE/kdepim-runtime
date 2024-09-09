/*
    SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
    SPDX-FileCopyrightText: 2020 Igor Poboiko <igor.poboiko@gmail.com>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "outlooksettings.h"
#include "ui_outlooksettingswidget.h"

class OutlookSettingsWidget : public QWidget, private Ui::OutlookSettingsWidget
{
    Q_OBJECT
public:
    explicit OutlookSettingsWidget(OutlookSettings &settings, const QString &identifier, QWidget *parent);
    ~OutlookSettingsWidget() override;

    void loadSettings();
    void saveSettings();

    OutlookSettings &m_settings;
};
