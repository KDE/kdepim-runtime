/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "configwidget.h"

#include "oxa/connectiontestjob.h"
#include "settings.h"
#include "ui_configwidget.h"

#include <KConfigDialogManager>
#include <KMessageBox>

#include <KLocalizedString>
#include <QPushButton>

ConfigWidget::ConfigWidget(Settings *settings, QWidget *parent)
    : QWidget(parent)
{
    Ui::ConfigWidget ui;
    ui.setupUi(this);

    ui.kcfg_BaseUrl->setWhatsThis(i18n("Enter the http or https URL to your Open-Xchange installation here."));
    ui.kcfg_Username->setWhatsThis(i18n("Enter the username of your Open-Xchange account here."));
    ui.kcfg_Password->setWhatsThis(i18n("Enter the password of your Open-Xchange account here."));

    mServerEdit = ui.kcfg_BaseUrl;
    mUserEdit = ui.kcfg_Username;
    mPasswordEdit = ui.kcfg_Password;
    mCheckConnectionButton = ui.checkConnectionButton;

    mManager = new KConfigDialogManager(this, settings);

    connect(mServerEdit, &KLineEdit::textChanged, this, &ConfigWidget::updateButtonState);
    connect(mUserEdit, &KLineEdit::textChanged, this, &ConfigWidget::updateButtonState);
    connect(mCheckConnectionButton, &QPushButton::clicked, this, &ConfigWidget::checkConnection);

    resize(QSize(410, 200));
}

void ConfigWidget::load()
{
    mManager->updateWidgets();
}

void ConfigWidget::save() const
{
    mManager->updateSettings();
}

void ConfigWidget::updateButtonState()
{
    mCheckConnectionButton->setEnabled(!mServerEdit->text().isEmpty() && !mUserEdit->text().isEmpty());
}

void ConfigWidget::checkConnection()
{
    OXA::ConnectionTestJob *job = new OXA::ConnectionTestJob(mServerEdit->text(), mUserEdit->text(),
                                                             mPasswordEdit->text(), this);
    connect(job, &OXA::ConnectionTestJob::result, this, &ConfigWidget::checkConnectionJobFinished);
    job->start();

    QApplication::setOverrideCursor(Qt::WaitCursor);
}

void ConfigWidget::checkConnectionJobFinished(KJob *job)
{
    QApplication::restoreOverrideCursor();

    if (job->error()) {
        KMessageBox::error(this, i18n("Unable to connect: %1", job->errorText()), i18n("Connection error"));
    } else {
        KMessageBox::information(this, i18n("Tested connection successfully."), i18n("Connection success"));
    }
}
