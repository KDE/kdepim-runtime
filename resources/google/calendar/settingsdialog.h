/*
    Copyright (C) 2011-2013  Daniel Vrátil <dvratil@redhat.com>

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

#include "common/googlesettingsdialog.h"

class KListWidget;
class QLabel;
class KDateComboBox;

class SettingsDialog : public GoogleSettingsDialog
{
  Q_OBJECT
  public:
    explicit SettingsDialog( GoogleAccountManager *accountManager, WId windowId, GoogleResource *parent );
    ~SettingsDialog();

  private Q_SLOTS:
    void slotReloadCalendars();
    void slotReloadTaskLists();
    void slotCurrentAccountChanged( const QString &accountName );

    void slotTaskListsRetrieved( KGAPI2::Job *job );
    void slotCalendarsRetrieved( KGAPI2::Job *job );

    void saveSettings();

  private:
    QGroupBox *m_calendarsBox;
    KListWidget *m_calendarsList;
    KPushButton *m_reloadCalendarsBtn;
    QLabel *m_eventsLimitLabel;
    KDateComboBox *m_eventsLimitCombo;

    QGroupBox *m_taskListsBox;
    KListWidget *m_taskListsList;
    KPushButton *m_reloadTaskListsBtn;

};

#endif // SETTINGSDIALOG_H
