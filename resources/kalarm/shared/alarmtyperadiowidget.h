/*
 *  alarmtyperadiowidget.h  -  KAlarm alarm type exclusive selection widget
 *  Program:  kalarm
 *  SPDX-FileCopyrightText: 2011 David Jarvie <djarvie@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef ALARMTYPERADIOWIDGET_H
#define ALARMTYPERADIOWIDGET_H

#include "singlefileresourceconfigwidget.h"
#include "ui_alarmtyperadiowidget.h"

#include <kalarmcal/kacalendar.h>

using namespace KAlarmCal;

class QButtonGroup;

class AlarmTypeRadioWidget : public Akonadi::SingleFileValidatingWidget
{
    Q_OBJECT
public:
    explicit AlarmTypeRadioWidget(QWidget *parent = nullptr);
    void setAlarmType(CalEvent::Type);
    CalEvent::Type alarmType() const;
    bool validate() const override;

private:
    Ui::AlarmTypeRadioWidget ui;
    QButtonGroup *mButtonGroup = nullptr;
};

#endif // ALARMTYPERADIOWIDGET_H
