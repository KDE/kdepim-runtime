/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                         a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#include "settingspasswordrequester.h"

#include <KMessageBox>
#include <KLocalizedString>
#include <QDialog>

#include <mailtransport/transportbase.h>
#include <kwindowsystem.h>
#include "imapresource_debug.h"
#include <QDialogButtonBox>
#include <QPushButton>
#include <KPasswordDialog>

#include "imapresourcebase.h"
#include "settings.h"

SettingsPasswordRequester::SettingsPasswordRequester(ImapResourceBase *resource, QObject *parent)
    : PasswordRequesterInterface(parent)
    , m_resource(resource)
{
}

SettingsPasswordRequester::~SettingsPasswordRequester()
{
    cancelPasswordRequests();
}

void SettingsPasswordRequester::requestPassword(RequestType request, const QString &serverError)
{
    if (request == WrongPasswordRequest) {
        QMetaObject::invokeMethod(this, "askUserInput", Qt::QueuedConnection, Q_ARG(QString, serverError));
    } else {
        connect(m_resource->settings(), &Settings::passwordRequestCompleted,
                this, &SettingsPasswordRequester::onPasswordRequestCompleted);
        m_resource->settings()->requestPassword();
    }
}

void SettingsPasswordRequester::askUserInput(const QString &serverError)
{
    // the credentials were not ok, allow to retry or change password
    if (m_requestDialog) {
        qCDebug(IMAPRESOURCE_LOG) << "Password request dialog is already open";
        return;
    }
    QWidget *parent = QWidget::find(m_resource->winIdForDialogs());
    QString text = i18n("The server for account \"%2\" refused the supplied username and password. "
                        "Do you want to go to the settings, have another attempt "
                        "at logging in, or do nothing?\n\n"
                        "%1", serverError, m_resource->name());
    QDialog *dialog = new QDialog(parent, Qt::Dialog);
    dialog->setWindowTitle(i18n("Could Not Authenticate"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::No | QDialogButtonBox::Yes, dialog);
    buttonBox->button(QDialogButtonBox::Yes)->setDefault(true);

    buttonBox->button(QDialogButtonBox::Yes)->setText(i18n("Account Settings"));
    buttonBox->button(QDialogButtonBox::No)->setText(i18nc("Input username/password manually and not store them", "Try Again"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(buttonBox->button(QDialogButtonBox::Yes), &QPushButton::clicked, this, &SettingsPasswordRequester::slotYesClicked);
    connect(buttonBox->button(QDialogButtonBox::No), &QPushButton::clicked, this, &SettingsPasswordRequester::slotNoClicked);
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &SettingsPasswordRequester::slotCancelClicked);

    connect(dialog, &QDialog::destroyed, this, &SettingsPasswordRequester::onDialogDestroyed);
    m_requestDialog = dialog;
    KWindowSystem::setMainWindow(dialog->windowHandle(), m_resource->winIdForDialogs());
    bool checkboxResult = false;
    KMessageBox::createKMessageBox(dialog, buttonBox, QMessageBox::Information,
                                   text, QStringList(),
                                   QString(),
                                   &checkboxResult, KMessageBox::NoExec);
    dialog->show();
}

void SettingsPasswordRequester::onDialogDestroyed()
{
    m_requestDialog = nullptr;
}

void SettingsPasswordRequester::slotNoClicked()
{
    connect(m_resource->settings(), &Settings::passwordRequestCompleted,
            this, &SettingsPasswordRequester::onPasswordRequestCompleted);
    requestManualAuth(nullptr);
    m_requestDialog = nullptr;
}

void SettingsPasswordRequester::slotYesClicked()
{
    if (!m_settingsDialog) {
        QDialog *dialog = m_resource->createConfigureDialog(m_resource->winIdForDialogs());
        connect(dialog, &QDialog::finished, this, &SettingsPasswordRequester::onSettingsDialogFinished);
        m_settingsDialog = dialog;
        dialog->show();
    }
    m_requestDialog = nullptr;
}

void SettingsPasswordRequester::slotCancelClicked()
{
    Q_EMIT done(UserRejected);
    m_requestDialog = nullptr;
}

void SettingsPasswordRequester::onSettingsDialogFinished(int result)
{
    m_settingsDialog = nullptr;
    if (result == QDialog::Accepted) {
        Q_EMIT done(ReconnectNeeded);
    } else {
        Q_EMIT done(UserRejected);
    }
}

void SettingsPasswordRequester::cancelPasswordRequests()
{
    if (m_requestDialog) {
        if (m_requestDialog->close()) {
            m_requestDialog = nullptr;
        }
    }
}

void SettingsPasswordRequester::onPasswordRequestCompleted(const QString &password, bool userRejected)
{
    disconnect(m_resource->settings(), &Settings::passwordRequestCompleted,
               this, &SettingsPasswordRequester::onPasswordRequestCompleted);

    QString pwd = password;
    if (userRejected || pwd.isEmpty()) {
        pwd = requestManualAuth(&userRejected);
    }

    if (userRejected) {
        Q_EMIT done(UserRejected);
    } else if (password.isEmpty() && (m_resource->settings()->authentication() != MailTransport::Transport::EnumAuthenticationType::GSSAPI)) {
        Q_EMIT done(EmptyPasswordEntered);
    } else {
        Q_EMIT done(PasswordRetrieved, password);
    }
}

QString SettingsPasswordRequester::requestManualAuth(bool *userRejected)
{
    QScopedPointer<KPasswordDialog> dlg(new KPasswordDialog(nullptr));
    dlg->setModal(true);
    dlg->setPrompt(i18n("Please enter password for user '%1' on IMAP server '%2'.",
                        m_resource->settings()->userName(), m_resource->settings()->imapServer()));
    dlg->setPassword(m_resource->settings()->password());
    if (dlg->exec()) {
        if (userRejected) {
            *userRejected = false;
        }
        m_resource->settings()->setPassword(dlg->password());
        return dlg->password();
    } else {
        if (userRejected) {
            *userRejected = true;
        }
        return QString();
    }
}
