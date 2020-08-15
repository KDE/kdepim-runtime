/*
 *  alarmtyperadiowidget.cpp  -  KAlarm alarm type exclusive selection widget
 *  Program:  kalarm
 *  SPDX-FileCopyrightText: 2011 David Jarvie <djarvie@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "alarmtyperadiowidget.h"

#include <QButtonGroup>

AlarmTypeRadioWidget::AlarmTypeRadioWidget(QWidget *parent)
    : Akonadi::SingleFileValidatingWidget(parent)
{
    ui.setupUi(this);
    ui.mainLayout->setContentsMargins(0, 0, 0, 0);
    mButtonGroup = new QButtonGroup(ui.groupBox);
    mButtonGroup->addButton(ui.activeRadio);
    mButtonGroup->addButton(ui.archivedRadio);
    mButtonGroup->addButton(ui.templateRadio);
    connect(ui.activeRadio, &QRadioButton::toggled, this, &AlarmTypeRadioWidget::changed);
    connect(ui.archivedRadio, &QRadioButton::toggled, this, &AlarmTypeRadioWidget::changed);
    connect(ui.templateRadio, &QRadioButton::toggled, this, &AlarmTypeRadioWidget::changed);
}

void AlarmTypeRadioWidget::setAlarmType(CalEvent::Type type)
{
    switch (type) {
    case CalEvent::ACTIVE:
        ui.activeRadio->setChecked(true);
        break;
    case CalEvent::ARCHIVED:
        ui.archivedRadio->setChecked(true);
        break;
    case CalEvent::TEMPLATE:
        ui.templateRadio->setChecked(true);
        break;
    default:
        break;
    }
}

CalEvent::Type AlarmTypeRadioWidget::alarmType() const
{
    if (ui.activeRadio->isChecked()) {
        return CalEvent::ACTIVE;
    }
    if (ui.archivedRadio->isChecked()) {
        return CalEvent::ARCHIVED;
    }
    if (ui.templateRadio->isChecked()) {
        return CalEvent::TEMPLATE;
    }
    return CalEvent::EMPTY;
}

bool AlarmTypeRadioWidget::validate() const
{
    return static_cast<bool>(mButtonGroup->checkedButton());
}
