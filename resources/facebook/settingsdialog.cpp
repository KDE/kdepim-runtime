/*
 *    Copyright (C) 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "settings.h"
#include "resource.h"
#include "tokenjobs.h"

#include <KLocalizedString>

SettingsDialog::SettingsDialog(FacebookResource *parent)
    : QDialog(nullptr)
    , mResource(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    auto settings = Settings::self();
    ui->attendingReminderChkBox->setChecked(settings->attendingReminders());
    ui->maybeAttendingReminderChkBox->setChecked(settings->maybeAttendingReminders());
    ui->notAttendingReminderChkBox->setChecked(settings->notAttendingReminders());
    ui->notRespondedReminderChkBox->setChecked(settings->notRespondedToReminders());
    ui->birthdayReminderChkBox->setChecked(settings->birthdayReminders());
    ui->eventReminderHoursSpinBox->setValue(settings->eventReminderHours());
    ui->birthdayReminderDaysSpinBox->setValue(settings->birthdayReminderDays());

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::save);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->loginBtn, &QPushButton::clicked, this, &SettingsDialog::login);
    connect(ui->logoutBtn, &QPushButton::clicked, this, &SettingsDialog::logout);

    checkToken();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::save()
{
    auto settings = Settings::self();
    const bool changed = ui->attendingReminderChkBox->isChecked() != settings->attendingReminders()
                         || ui->maybeAttendingReminderChkBox->isChecked() != settings->maybeAttendingReminders()
                         || ui->notAttendingReminderChkBox->isChecked() != settings->notAttendingReminders()
                         || ui->notRespondedReminderChkBox->isChecked() != settings->notRespondedToReminders()
                         || ui->birthdayReminderChkBox->isChecked() != settings->birthdayReminders()
                         || ui->eventReminderHoursSpinBox->value() != settings->eventReminderHours()
                         || ui->birthdayReminderDaysSpinBox->value() != settings->birthdayReminderDays();
    if (!changed) {
        reject();
    }

    settings->setAttendingReminders(ui->attendingReminderChkBox->isChecked());
    settings->setMaybeAttendingReminders(ui->maybeAttendingReminderChkBox->isChecked());
    settings->setNotAttendingReminders(ui->notAttendingReminderChkBox->isChecked());
    settings->setNotAttendingReminders(ui->notRespondedReminderChkBox->isChecked());
    settings->setBirthdayReminders(ui->birthdayReminderChkBox->isChecked());
    settings->setEventReminderHours(ui->eventReminderHoursSpinBox->value());
    settings->setBirthdayReminderDays(ui->birthdayReminderDaysSpinBox->value());

    accept();
}

void SettingsDialog::checkToken()
{
    ui->loginBtn->setVisible(false);
    ui->logoutBtn->setVisible(false);
    ui->loginStatusLbl->setText(i18n("Checking login status..."));

    auto job = new GetTokenJob(mResource);
    connect(job, &GetTokenJob::result,
            this, [this, job]() {
        if (job->error()) {
            ui->loginStatusLbl->setText(job->errorText());
        } else {
            const auto token = job->token();
            if (token.isEmpty()) {
                ui->loginStatusLbl->setText(i18n("Not logged in"));
                ui->logoutBtn->setVisible(false);
                ui->loginBtn->setVisible(true);
            } else {
                ui->loginStatusLbl->setText(i18n("Logged in as <b>%1</b>", job->userName()));
                ui->loginBtn->setVisible(false);
                ui->logoutBtn->setVisible(true);
            }
        }
    });
    job->start();
}

void SettingsDialog::login()
{
    ui->loginBtn->setEnabled(false);
    auto job = new LoginJob(mResource);
    connect(job, &LoginJob::result,
            this, [this](KJob *job) {
        if (job->error()) {
            ui->loginStatusLbl->setText(job->errorText());
        } else {
            checkToken();
        }
    });
    job->start();
}

void SettingsDialog::logout()
{
    ui->logoutBtn->setEnabled(false);
    auto job = new LogoutJob(mResource);
    connect(job, &LogoutJob::result,
            this, [this](KJob *job) {
        if (job->error()) {
            ui->loginStatusLbl->setText(job->errorText());
        } else {
            checkToken();
        }
    });
    job->start();
}
