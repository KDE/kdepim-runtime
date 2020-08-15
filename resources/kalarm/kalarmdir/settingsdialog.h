/*
 *  settingsdialog.h  -  Akonadi KAlarm directory resource configuration dialog
 *  Program:  kalarm
 *  SPDX-FileCopyrightText: 2011 David Jarvie <djarvie@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KALARMDIR_SETTINGSDIALOG_H
#define KALARMDIR_SETTINGSDIALOG_H

#include "ui_settingsdialog.h"
#include "ui_alarmtypewidget.h"

#include <kalarmcal/kacalendar.h>

#include <QDialog>
class QPushButton;
using namespace KAlarmCal;

class KConfigDialogManager;
class AlarmTypeWidget;

namespace Akonadi_KAlarm_Dir_Resource {
class Settings;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    SettingsDialog(WId windowId, Settings *);
    void setAlarmTypes(CalEvent::Types);
    CalEvent::Types alarmTypes() const;

private Q_SLOTS:
    void save();
    void validate();
    void textChanged();
    void readOnlyClicked(bool);

private:
    Ui::SettingsDialog ui;
    AlarmTypeWidget *mTypeSelector = nullptr;
    QPushButton *mOkButton = nullptr;
    KConfigDialogManager *mManager = nullptr;
    Akonadi_KAlarm_Dir_Resource::Settings *mSettings = nullptr;
    bool mReadOnlySelected = false;                    // read-only was set by user (not by validate())
};
}

#endif // KALARMDIR_SETTINGSDIALOG_H
