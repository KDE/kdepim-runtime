/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GOOGLE_CALENDAR_SETTINGSDIALOG_H
#define GOOGLE_CALENDAR_SETTINGSDIALOG_H

#include <KDialog>
#include <KJob>
#include <Akonadi/ResourceBase>

#include <libkgapi/common.h>

namespace Ui {
  class SettingsDialog;
}

namespace KGAPI {
  class Reply;
  class AccessManager;

namespace Objects {
  class Calendar;
  class TaskList;
}
}

class QListWidgetItem;

using namespace KGAPI;

class SettingsDialog : public KDialog
{
  Q_OBJECT
  public:
    explicit SettingsDialog( WId windowId, QWidget *parent = 0 );
    ~SettingsDialog();

  private Q_SLOTS:
    void reloadAccounts();
    void addAccountClicked();
    void removeAccountClicked();
    void accountChanged();
    void addCalendarClicked();
    void editCalendarClicked();
    void removeCalendarClicked();
    void reloadCalendarsClicked();
    void addTaskListClicked();
    void editTaskListClicked();
    void removeTaskListClicked();
    void reloadTaskListsClicked();

    void gam_objectsListReceived( KGAPI::Reply *reply );
    void gam_objectCreated( KGAPI::Reply *reply );
    void gam_objectModified( KGAPI::Reply *reply );

    void addCalendar( KGAPI::Objects::Calendar *calendar );
    void editCalendar( KGAPI::Objects::Calendar *calendar );

    void addTaskList( KGAPI::Objects::TaskList *taskList );
    void editTaskList( KGAPI::Objects::TaskList *taskList );

    void saveSettings();

    void error( KGAPI::Error code, const QString &msg );

  private:
    Ui::SettingsDialog *m_ui;
    WId m_windowId;
    AccessManager *m_gam;
};

#endif // SETTINGSDIALOG_H
