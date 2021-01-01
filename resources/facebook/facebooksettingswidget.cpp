/*
 *    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
 *    SPDX-FileCopyrightText: 2018-2021 Laurent Montel <montel@kde.org>
 *
 *    SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "facebooksettingswidget.h"
#include "ui_facebookagentsettingswidget.h"
#include "settings.h"
#include "resource.h"
#include "tokenjobs.h"

#include <KLocalizedString>

FacebookSettingsWidget::FacebookSettingsWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
    , ui(new Ui::FacebookAgentSettingsWidget)
{
    Settings::instance(config);

    QWidget *mainWidget = new QWidget(parent);

    ui->setupUi(mainWidget);
    parent->layout()->addWidget(mainWidget);

    auto settings = Settings::self();
    ui->attendingReminderChkBox->setChecked(settings->attendingReminders());
    ui->maybeAttendingReminderChkBox->setChecked(settings->maybeAttendingReminders());
    ui->notAttendingReminderChkBox->setChecked(settings->notAttendingReminders());
    ui->notRespondedReminderChkBox->setChecked(settings->notRespondedToReminders());
    ui->birthdayReminderChkBox->setChecked(settings->birthdayReminders());
    ui->eventReminderHoursSpinBox->setValue(settings->eventReminderHours());
    ui->birthdayReminderDaysSpinBox->setValue(settings->birthdayReminderDays());

    connect(ui->loginBtn, &QPushButton::clicked, this, &FacebookSettingsWidget::login);
    connect(ui->logoutBtn, &QPushButton::clicked, this, &FacebookSettingsWidget::logout);
}

FacebookSettingsWidget::~FacebookSettingsWidget()
{
}

void FacebookSettingsWidget::checkToken()
{
    ui->loginBtn->setVisible(false);
    ui->logoutBtn->setVisible(false);
    ui->loginStatusLbl->setText(i18n("Checking login status..."));

    auto job = new GetTokenJob(identifier(), this);
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

void FacebookSettingsWidget::login()
{
    ui->loginBtn->setEnabled(false);
    auto job = new LoginJob(identifier(), this);
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

void FacebookSettingsWidget::logout()
{
    ui->logoutBtn->setEnabled(false);
    auto job = new LogoutJob(identifier(), this);
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

void FacebookSettingsWidget::load()
{
    checkToken();
}

bool FacebookSettingsWidget::save() const
{
    auto settings = Settings::self();
    const bool changed = ui->attendingReminderChkBox->isChecked() != settings->attendingReminders()
                         || ui->maybeAttendingReminderChkBox->isChecked() != settings->maybeAttendingReminders()
                         || ui->notAttendingReminderChkBox->isChecked() != settings->notAttendingReminders()
                         || ui->notRespondedReminderChkBox->isChecked() != settings->notRespondedToReminders()
                         || ui->birthdayReminderChkBox->isChecked() != settings->birthdayReminders()
                         || ui->eventReminderHoursSpinBox->value() != settings->eventReminderHours()
                         || ui->birthdayReminderDaysSpinBox->value() != settings->birthdayReminderDays();
    if (changed) {
        settings->setAttendingReminders(ui->attendingReminderChkBox->isChecked());
        settings->setMaybeAttendingReminders(ui->maybeAttendingReminderChkBox->isChecked());
        settings->setNotAttendingReminders(ui->notAttendingReminderChkBox->isChecked());
        settings->setNotAttendingReminders(ui->notRespondedReminderChkBox->isChecked());
        settings->setBirthdayReminders(ui->birthdayReminderChkBox->isChecked());
        settings->setEventReminderHours(ui->eventReminderHoursSpinBox->value());
        settings->setBirthdayReminderDays(ui->birthdayReminderDaysSpinBox->value());
    }
    return true;
}
