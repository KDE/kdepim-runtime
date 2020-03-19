/*
   Copyright (C) 2013 Daniel Vrátil <dvratil@redhat.com>
                 2020 Igor Poboiko <igor.poboiko@gmail.com>

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

#include "googlesettingsdialog.h"
#include "ui_googlesettingsdialog.h"
#include "googlesettings.h"
#include "googleaccountmanager.h"
#include "googleresource.h"
#include "googleresource_debug.h"

#include <QDialogButtonBox>

#include <KGAPI/AuthJob>
#include <KGAPI/Calendar/Calendar>
#include <KGAPI/Calendar/CalendarFetchJob>
#include <KGAPI/Tasks/TaskList>
#include <KGAPI/Tasks/TaskListFetchJob>
#include <KMessageBox>
#include <KWindowSystem>

using namespace KGAPI2;

GoogleSettingsDialog::GoogleSettingsDialog(GoogleResource *resource, WId wId)
    : QDialog()
    , m_resource(resource)
{
    if (wId) {
        setAttribute(Qt::WA_NativeWindow, true);
        KWindowSystem::setMainWindow(windowHandle(), wId);
    }
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QWidget *mainWidget = new QWidget(this);
    mainLayout->addWidget(mainWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    mainLayout->addWidget(buttonBox);

    m_ui = new Ui::GoogleSettingsDialog;
    m_ui->setupUi(mainWidget);
    
    m_ui->refreshSpinBox->setSuffix(ki18np(" minute", " minutes"));
    m_ui->enableRefresh->setChecked(GoogleSettings::self()->enableIntervalCheck());
    m_ui->refreshSpinBox->setEnabled(GoogleSettings::self()->enableIntervalCheck());
    if (GoogleSettings::self()->enableIntervalCheck()) {
        m_ui->refreshSpinBox->setValue(GoogleSettings::self()->intervalCheckTime());
    } else {
        m_ui->refreshSpinBox->setValue(30);
    }

    m_ui->eventsLimitCombo->setMaximumDate(QDate::currentDate());
    m_ui->eventsLimitCombo->setMinimumDate(QDate::fromString(QStringLiteral("2000-01-01"), Qt::ISODate));
    m_ui->eventsLimitCombo->setOptions(KDateComboBox::EditDate | KDateComboBox::SelectDate
                                   |KDateComboBox::DatePicker | KDateComboBox::WarnOnInvalid);
    if (GoogleSettings::self()->eventsSince().isEmpty()) {
        const QString ds = QStringLiteral("%1-01-01").arg(QString::number(QDate::currentDate().year() - 3));
        m_ui->eventsLimitCombo->setDate(QDate::fromString(ds, Qt::ISODate));
    } else {
        m_ui->eventsLimitCombo->setDate(QDate::fromString(GoogleSettings::self()->eventsSince(), Qt::ISODate));
    }

    connect(buttonBox, &QDialogButtonBox::accepted, this, &GoogleSettingsDialog::slotSaveSettings);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &GoogleSettingsDialog::reject);
    connect(m_ui->reloadCalendarsBtn, &QPushButton::clicked, this, &GoogleSettingsDialog::slotReloadCalendars);
    connect(m_ui->reloadTaskListsBtn, &QPushButton::clicked, this, &GoogleSettingsDialog::slotReloadTaskLists);
    connect(m_ui->configureBtn, &QPushButton::clicked, this, &GoogleSettingsDialog::slotConfigure);
    if (!GoogleSettings::self()->account().isEmpty()) {
        m_account = m_resource->accountManager()->findAccount(GoogleSettings::self()->account());
    }
    QMetaObject::invokeMethod(this, &GoogleSettingsDialog::accountChanged, Qt::QueuedConnection);
}

GoogleSettingsDialog::~GoogleSettingsDialog()
{
}

bool GoogleSettingsDialog::handleError(Job *job)
{
    if ((job->error() == KGAPI2::NoError) || (job->error() == KGAPI2::OK)) {
        return true;
    }

    if (job->error() == KGAPI2::Unauthorized) {
        qCDebug(GOOGLE_LOG) << job << job->errorString();
        const QList<QUrl> resourceScopes = m_resource->scopes();
        for (const QUrl &scope : resourceScopes) {
            if (!m_account->scopes().contains(scope)) {
                m_account->addScope(scope);
            }
        }

        AuthJob *authJob = new AuthJob(m_account, GoogleSettings::self()->clientId(),
                                       GoogleSettings::self()->clientSecret(), this);
        authJob->setProperty(JOB_PROPERTY, QVariant::fromValue(job));
        connect(authJob, &AuthJob::finished, this, &GoogleSettingsDialog::slotAuthJobFinished);

        return false;
    }

    KMessageBox::sorry(this, job->errorString());
    return false;
}

void GoogleSettingsDialog::accountChanged()
{
    if (!m_account) {
        m_ui->accountLabel->setText(i18n("<b>not configured</b>"));
        m_ui->calendarsBox->setDisabled(true);
        m_ui->calendarsList->clear();
        m_ui->taskListsBox->setDisabled(true);
        m_ui->taskListsList->clear();
        return;
    }
    m_ui->accountLabel->setText(QStringLiteral("<b>%1</b>").arg(m_account->accountName()));
    slotReloadCalendars();
    slotReloadTaskLists();
}

void GoogleSettingsDialog::slotConfigure()
{
    m_account = AccountPtr(new Account());
    const QList<QUrl> resourceScopes = m_resource->scopes();
    for (const QUrl &scope : resourceScopes) {
        if (!m_account->scopes().contains(scope)) {
            m_account->addScope(scope);
        }
    }
    AuthJob *authJob = new AuthJob(m_account,
                                   GoogleSettings::self()->clientId(),
                                   GoogleSettings::self()->clientSecret());
    connect(authJob, &AuthJob::finished, this, &GoogleSettingsDialog::slotAuthJobFinished);
}

void GoogleSettingsDialog::slotAuthJobFinished(Job* job)
{
    auto authJob = qobject_cast<AuthJob *>(job);
    m_account = authJob->account();
    if (authJob->error() != KGAPI2::NoError) {
        KMessageBox::sorry(this, authJob->errorString());
        return;
    }
    if (!m_resource->accountManager()->storeAccount(m_account)) {
        qCWarning(GOOGLE_LOG) << "Failed to add account to KWallet";
        return;
    }
    accountChanged();

    auto otherJob = job->property(JOB_PROPERTY).value<KGAPI2::Job *>();
    if (otherJob) {
        otherJob->setAccount(m_account);
        otherJob->restart();
    }
}

void GoogleSettingsDialog::slotSaveSettings()
{
    if (!m_account) {
        GoogleSettings::self()->setAccount(QString());
        GoogleSettings::self()->setEnableIntervalCheck(m_ui->enableRefresh->isChecked());
        GoogleSettings::self()->setIntervalCheckTime(m_ui->refreshSpinBox->value());
        GoogleSettings::self()->setCalendars({});
        GoogleSettings::self()->setTaskLists({});
        GoogleSettings::self()->setEventsSince(QString());
        return;
    }
    GoogleSettings::self()->setAccount(m_account->accountName());
    GoogleSettings::self()->setEnableIntervalCheck(m_ui->enableRefresh->isChecked());
    GoogleSettings::self()->setIntervalCheckTime(m_ui->refreshSpinBox->value());

    QStringList calendars;
    for (int i = 0; i < m_ui->calendarsList->count(); i++) {
        QListWidgetItem *item = m_ui->calendarsList->item(i);

        if (item->checkState() == Qt::Checked) {
            calendars.append(item->data(Qt::UserRole).toString());
        }
    }
    GoogleSettings::self()->setCalendars(calendars);

    if (m_ui->eventsLimitCombo->isValid()) {
        GoogleSettings::self()->setEventsSince(m_ui->eventsLimitCombo->date().toString(Qt::ISODate));
    }

    QStringList taskLists;
    for (int i = 0; i < m_ui->taskListsList->count(); i++) {
        QListWidgetItem *item = m_ui->taskListsList->item(i);

        if (item->checkState() == Qt::Checked) {
            taskLists.append(item->data(Qt::UserRole).toString());
        }
    }
    GoogleSettings::self()->setTaskLists(taskLists);

    accept();
}

void GoogleSettingsDialog::slotReloadCalendars()
{
    m_ui->calendarsBox->setDisabled(true);
    m_ui->calendarsList->clear();

    if (!m_account) {
        return;
    }

    auto fetchJob = new CalendarFetchJob(m_account, this);
    connect(fetchJob, &CalendarFetchJob::finished, [this](Job* job){
                if (!handleError(job) || !m_account) {
                    m_ui->calendarsBox->setEnabled(false);
                    return;
                }

                const ObjectsList objects = qobject_cast<FetchJob *>(job)->items();

                QStringList activeCalendars;
                if (m_account->accountName() == GoogleSettings::self()->account()) {
                    activeCalendars = GoogleSettings::self()->calendars();
                }
                m_ui->calendarsList->clear();
                for (const ObjectPtr &object : objects) {
                    const CalendarPtr calendar = object.dynamicCast<Calendar>();

                    QListWidgetItem *item = new QListWidgetItem(calendar->title());
                    item->setData(Qt::UserRole, calendar->uid());
                    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
                    item->setCheckState((activeCalendars.isEmpty() || activeCalendars.contains(calendar->uid())) ? Qt::Checked : Qt::Unchecked);
                    m_ui->calendarsList->addItem(item);
                }

                m_ui->calendarsBox->setEnabled(true);
            });
}

void GoogleSettingsDialog::slotReloadTaskLists()
{
    if (!m_account) {
        return;
    }

    m_ui->taskListsBox->setDisabled(true);
    m_ui->taskListsList->clear();

    auto job = new TaskListFetchJob(m_account, this);
    connect(job, &TaskListFetchJob::finished, [this](KGAPI2::Job *job){
                if (!handleError(job) || !m_account) {
                    m_ui->taskListsBox->setDisabled(true);
                    return;
                }

                const ObjectsList objects = qobject_cast<FetchJob *>(job)->items();

                QStringList activeTaskLists;
                if (m_account->accountName() == GoogleSettings::self()->account()) {
                    activeTaskLists = GoogleSettings::self()->taskLists();
                }
                m_ui->taskListsList->clear();
                for (const ObjectPtr &object : objects) {
                    const TaskListPtr taskList = object.dynamicCast<TaskList>();

                    QListWidgetItem *item = new QListWidgetItem(taskList->title());
                    item->setData(Qt::UserRole, taskList->uid());
                    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
                    item->setCheckState((activeTaskLists.isEmpty() || activeTaskLists.contains(taskList->uid())) ? Qt::Checked : Qt::Unchecked);
                    m_ui->taskListsList->addItem(item);
                }

                m_ui->taskListsBox->setEnabled(true);

            });
}
