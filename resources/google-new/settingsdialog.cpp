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
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "settingsdialog.h"
#include "googlesettings.h"

#include <KLocalizedString>
#include <QListWidget>
#include <QPushButton>
#include <KDateComboBox>

#include <QIcon>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

#include <KGAPI/Account>
#include <KGAPI/AuthJob>
#include <KGAPI/Calendar/Calendar>
#include <KGAPI/Calendar/CalendarFetchJob>
#include <KGAPI/Tasks/TaskList>
#include <KGAPI/Tasks/TaskListFetchJob>

using namespace KGAPI2;

SettingsDialog::SettingsDialog(GoogleAccountManager *accountManager, WId windowId, GoogleResource *parent)
    : GoogleSettingsDialog(accountManager, windowId, parent)
{
    connect(this, &SettingsDialog::currentAccountChanged, this, &SettingsDialog::slotCurrentAccountChanged);

    m_calendarsBox = new QGroupBox(i18n("Calendars"), this);
    mainLayout()->addWidget(m_calendarsBox);

    QVBoxLayout *vbox = new QVBoxLayout(m_calendarsBox);

    m_calendarsList = new QListWidget(m_calendarsBox);
    vbox->addWidget(m_calendarsList, 1);

    m_reloadCalendarsBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("Reload"), m_calendarsBox);
    vbox->addWidget(m_reloadCalendarsBtn);
    connect(m_reloadCalendarsBtn, &QPushButton::clicked, this, &SettingsDialog::slotReloadCalendars);

    QHBoxLayout *hbox = new QHBoxLayout;
    vbox->addLayout(hbox);

    m_eventsLimitLabel = new QLabel(i18nc("Followed by a date picker widget", "&Fetch only new events since"), this);
    hbox->addWidget(m_eventsLimitLabel);

    m_eventsLimitCombo = new KDateComboBox(this);
    m_eventsLimitLabel->setBuddy(m_eventsLimitCombo);
    m_eventsLimitCombo->setMaximumDate(QDate::currentDate());
    m_eventsLimitCombo->setMinimumDate(QDate::fromString(QStringLiteral("2000-01-01"), Qt::ISODate));
    m_eventsLimitCombo->setOptions(KDateComboBox::EditDate | KDateComboBox::SelectDate
                                   |KDateComboBox::DatePicker | KDateComboBox::WarnOnInvalid);
    if (GoogleSettings::self()->eventsSince().isEmpty()) {
        const QString ds = QStringLiteral("%1-01-01").arg(QString::number(QDate::currentDate().year() - 3));
        m_eventsLimitCombo->setDate(QDate::fromString(ds, Qt::ISODate));
    } else {
        m_eventsLimitCombo->setDate(QDate::fromString(GoogleSettings::self()->eventsSince(), Qt::ISODate));
    }
    hbox->addWidget(m_eventsLimitCombo);

    m_taskListsBox = new QGroupBox(i18n("Tasklists"), this);
    mainLayout()->addWidget(m_taskListsBox);

    vbox = new QVBoxLayout(m_taskListsBox);

    m_taskListsList = new QListWidget(m_taskListsBox);
    vbox->addWidget(m_taskListsList, 1);

    m_reloadTaskListsBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("Reload"), m_taskListsBox);
    vbox->addWidget(m_reloadTaskListsBtn);
    connect(m_reloadTaskListsBtn, &QPushButton::clicked, this, &SettingsDialog::slotReloadTaskLists);
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::saveSettings()
{
    const AccountPtr account = currentAccount();
    if (!currentAccount()) {
        GoogleSettings::self()->setAccount(QString());
        GoogleSettings::self()->setCalendars(QStringList());
        GoogleSettings::self()->setTaskLists(QStringList());
        GoogleSettings::self()->setEventsSince(QString());
        GoogleSettings::self()->save();
        return;
    }

    GoogleSettings::self()->setAccount(account->accountName());

    QStringList calendars;
    for (int i = 0; i < m_calendarsList->count(); i++) {
        QListWidgetItem *item = m_calendarsList->item(i);

        if (item->checkState() == Qt::Checked) {
            calendars.append(item->data(Qt::UserRole).toString());
        }
    }
    GoogleSettings::self()->setCalendars(calendars);
    if (m_eventsLimitCombo->isValid()) {
        GoogleSettings::self()->setEventsSince(m_eventsLimitCombo->date().toString(Qt::ISODate));
    }

    QStringList taskLists;
    for (int i = 0; i < m_taskListsList->count(); i++) {
        QListWidgetItem *item = m_taskListsList->item(i);

        if (item->checkState() == Qt::Checked) {
            taskLists.append(item->data(Qt::UserRole).toString());
        }
    }
    GoogleSettings::self()->setTaskLists(taskLists);

    GoogleSettings::self()->save();
}

void SettingsDialog::slotCurrentAccountChanged(const QString &accountName)
{
    if (accountName.isEmpty()) {
        m_taskListsBox->setDisabled(true);
        m_taskListsList->clear();
        m_calendarsBox->setDisabled(true);
        m_calendarsList->clear();
        return;
    }

    slotReloadCalendars();
    slotReloadTaskLists();
}

void SettingsDialog::slotReloadCalendars()
{
    const AccountPtr account = currentAccount();
    if (!account) {
        return;
    }

    CalendarFetchJob *fetchJob = new CalendarFetchJob(account, this);
    connect(fetchJob, &CalendarFetchJob::finished, this, &SettingsDialog::slotCalendarsRetrieved);

    m_calendarsBox->setDisabled(true);
    m_calendarsList->clear();
}

void SettingsDialog::slotReloadTaskLists()
{
    const AccountPtr account = currentAccount();
    if (!account) {
        return;
    }

    TaskListFetchJob *fetchJob = new TaskListFetchJob(account, this);
    connect(fetchJob, &TaskListFetchJob::finished, this, &SettingsDialog::slotTaskListsRetrieved);

    m_taskListsBox->setDisabled(true);
    m_taskListsList->clear();
}

void SettingsDialog::slotCalendarsRetrieved(Job *job)
{
    if (!handleError(job) || !currentAccount()) {
        m_calendarsBox->setEnabled(true);
        return;
    }

    FetchJob *fetchJob = qobject_cast<FetchJob *>(job);
    const ObjectsList objects = fetchJob->items();

    QStringList activeCalendars;
    if (currentAccount()->accountName() == GoogleSettings::self()->account()) {
        activeCalendars = GoogleSettings::self()->calendars();
    }
    m_calendarsList->clear();
    for (const ObjectPtr &object : objects) {
        CalendarPtr calendar = object.dynamicCast<Calendar>();

        QListWidgetItem *item = new QListWidgetItem(calendar->title());
        item->setData(Qt::UserRole, calendar->uid());
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckState((activeCalendars.isEmpty() || activeCalendars.contains(calendar->uid())) ? Qt::Checked : Qt::Unchecked);
        m_calendarsList->addItem(item);
    }

    m_calendarsBox->setEnabled(true);
}

void SettingsDialog::slotTaskListsRetrieved(Job *job)
{
    if (!handleError(job) || !currentAccount()) {
        m_taskListsBox->setEnabled(true);
        return;
    }

    FetchJob *fetchJob = qobject_cast<FetchJob *>(job);
    const ObjectsList objects = fetchJob->items();

    QStringList activeTaskLists;
    if (currentAccount()->accountName() == GoogleSettings::self()->account()) {
        activeTaskLists = GoogleSettings::self()->taskLists();
    }
    m_taskListsList->clear();
    for (const ObjectPtr &object : objects) {
        TaskListPtr taskList = object.dynamicCast<TaskList>();

        QListWidgetItem *item = new QListWidgetItem(taskList->title());
        item->setData(Qt::UserRole, taskList->uid());
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckState((activeTaskLists.isEmpty() || activeTaskLists.contains(taskList->uid())) ? Qt::Checked : Qt::Unchecked);
        m_taskListsList->addItem(item);
    }

    m_taskListsBox->setEnabled(true);
}
