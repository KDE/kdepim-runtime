/*
 *  alarmtypewidget.h  -  KAlarm Akonadi configuration alarm type selection widget
 *  Program:  kalarm
 *  SPDX-FileCopyrightText: 2011 David Jarvie <djarvie@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "ui_alarmtypewidget.h"

#include <KAlarmCal/KACalendar>

using namespace KAlarmCal;

class AlarmTypeWidget : public QWidget
{
    Q_OBJECT
public:
    AlarmTypeWidget(QWidget *parent, QLayout *layout);
    void setAlarmTypes(CalEvent::Types);
    CalEvent::Types alarmTypes() const;

Q_SIGNALS:
    void changed();

private:
    Ui::AlarmTypeWidget ui;
};

