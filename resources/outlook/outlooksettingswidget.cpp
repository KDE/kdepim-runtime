/*
    SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
    SPDX-FileCopyrightText: 2020 Igor Poboiko <igor.poboiko@gmail.com>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "outlooksettingswidget.h"

#include <QDialogButtonBox>

OutlookSettingsWidget::OutlookSettingsWidget(OutlookSettings &settings, QWidget *parent)
    : QWidget(parent)
    , m_settings(settings)
{
    auto mainLayout = new QVBoxLayout(this);

    auto mainWidget = new QWidget(this);
    mainLayout->addWidget(mainWidget);
    setupUi(mainWidget);
}

OutlookSettingsWidget::~OutlookSettingsWidget() = default;

void OutlookSettingsWidget::loadSettings()
{
    accessTokenEdit->setText(m_settings.accessToken());
}

void OutlookSettingsWidget::saveSettings()
{
    m_settings.setAccessToken(accessTokenEdit);
}

#include "moc_outlooksettingswidget.cpp"
