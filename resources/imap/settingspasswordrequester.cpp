/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Kevin Ottens <kevin@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "settingspasswordrequester.h"

#include <KLocalizedString>
#include <KNotification>

#include "imapresource_debug.h"
#include "imapresourcebase.h"
#include "settings.h"
#include <QDialog>
#include <kwindowsystem.h>
#include <mailtransport/transportbase.h>

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
        connect(m_resource->settings(), &Settings::passwordRequestCompleted, this, &SettingsPasswordRequester::onPasswordRequestCompleted);
        m_resource->settings()->requestPassword();
    }
}

void SettingsPasswordRequester::askUserInput(const QString &serverError)
{
    auto notification = new KNotification(QStringLiteral("imapAuthFailed"), KNotification::Persistent);
    notification->setComponentName(QStringLiteral("akonadi_imap_resource"));
    notification->setIconName(QStringLiteral("network-server"));
    notification->setTitle(i18nc("@title", "An IMAP e-mail account needs your attention."));

    const auto accountName = m_resource->name();
    const auto message = accountName.isEmpty()
            ? i18n("The IMAP server refused the supplied username and password.\n"
                   "Do you want to try again, or open the settings?\n\n"
                   "%1", serverError)
            : i18n("The IMAP server for account %1 refused the supplied username and password.\n"
                   "Do you want to try again, or open the settings?\n\n"
                   "%2", accountName, serverError);
    notification->setText(message);

    auto tryAgainAction = notification->addAction(i18nc("@action:button", "Try again"));
    connect(tryAgainAction, &KNotificationAction::activated, this, [this, notification]() {
        disconnect(notification, &KNotification::closed, nullptr, nullptr);
        slotNoClicked();
    });

    auto openSettingsAction = notification->addAction(i18nc("@action:button", "Open settings"));
    connect(openSettingsAction, &KNotificationAction::activated, this, [this, notification]() {
        disconnect(notification, &KNotification::closed, nullptr, nullptr);
        slotYesClicked();
    });

    connect(notification, &KNotification::closed, this, [this] {
        Q_EMIT done(UserRejected);
    });

    notification->sendEvent();
}

void SettingsPasswordRequester::slotNoClicked()
{
    connect(m_resource->settings(), &Settings::passwordRequestCompleted, this, &SettingsPasswordRequester::onPasswordRequestCompleted);
    m_resource->settings()->requestPassword();
}

void SettingsPasswordRequester::slotYesClicked()
{
    if (!m_settingsDialog) {
        QDialog *dialog = m_resource->createConfigureDialog(m_resource->winIdForDialogs());
        connect(dialog, &QDialog::finished, this, &SettingsPasswordRequester::onSettingsDialogFinished);
        m_settingsDialog = dialog;
        dialog->show();
    }
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
}

void SettingsPasswordRequester::onPasswordRequestCompleted(const QString &password, bool userRejected)
{
    disconnect(m_resource->settings(), &Settings::passwordRequestCompleted, this, &SettingsPasswordRequester::onPasswordRequestCompleted);

    QString pwd = password;
    if (userRejected || pwd.isEmpty()) {
        pwd = m_resource->settings()->password();
    }

    if (userRejected) {
        Q_EMIT done(UserRejected);
    } else if (password.isEmpty() && (m_resource->settings()->authentication() != MailTransport::Transport::EnumAuthenticationType::GSSAPI)) {
        Q_EMIT done(EmptyPasswordEntered);
    } else {
        Q_EMIT done(PasswordRetrieved, password);
    }
}

#include "moc_settingspasswordrequester.cpp"
