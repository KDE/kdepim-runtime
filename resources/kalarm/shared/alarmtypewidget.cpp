/*
 *  alarmtypewidget.cpp  -  KAlarm Akonadi configuration alarm type selection widget
 *  Program:  kalarm
 *  SPDX-FileCopyrightText: 2011 David Jarvie <djarvie@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "alarmtypewidget.h"

AlarmTypeWidget::AlarmTypeWidget(QWidget *parent, QLayout *layout)
    : QWidget()
{
    ui.setupUi(parent);
    layout->addWidget(ui.groupBox);
    connect(ui.activeCheckBox, &QCheckBox::toggled, this, &AlarmTypeWidget::changed);
    connect(ui.archivedCheckBox, &QCheckBox::toggled, this, &AlarmTypeWidget::changed);
    connect(ui.templateCheckBox, &QCheckBox::toggled, this, &AlarmTypeWidget::changed);
}

void AlarmTypeWidget::setAlarmTypes(CalEvent::Types types)
{
    if (types & CalEvent::ACTIVE) {
        ui.activeCheckBox->setChecked(true);
    }
    if (types & CalEvent::ARCHIVED) {
        ui.archivedCheckBox->setChecked(true);
    }
    if (types & CalEvent::TEMPLATE) {
        ui.templateCheckBox->setChecked(true);
    }
}

CalEvent::Types AlarmTypeWidget::alarmTypes() const
{
    CalEvent::Types types = CalEvent::EMPTY;
    if (ui.activeCheckBox->isChecked()) {
        types |= CalEvent::ACTIVE;
    }
    if (ui.archivedCheckBox->isChecked()) {
        types |= CalEvent::ARCHIVED;
    }
    if (ui.templateCheckBox->isChecked()) {
        types |= CalEvent::TEMPLATE;
    }
    return types;
}
