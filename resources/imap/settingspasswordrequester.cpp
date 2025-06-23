/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Kevin Ottens <kevin@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "settingspasswordrequester.h"

#include <KLocalizedString>
#include <KNotification>

#include <qt6keychain/keychain.h>

#include "imapresource_debug.h"
#include "imapresourcebase.h"
#include "settings.h"
#include <QDialog>
#include <kwindowsystem.h>
#include <mailtransport/transportbase.h>

using namespace QKeychain;

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
        if (m_resource->settings()->mustFetchPassword()) {
            auto job = m_resource->settings()->requestPassword();
            connect(job, &ReadPasswordJob::finished, this, [this, job](auto) {
                auto password = job->textData();
                if (job->error() == NoError) {
                    if (password.isEmpty()) {
                        Q_EMIT done(EmptyPasswordEntered);
                    } else {
                        Q_EMIT done(PasswordRetrieved, password);
                    }
                } else if (job->error() == AccessDeniedByUser) {
                    Q_EMIT done(UserRejected);
                } else {
                    Q_EMIT done(EmptyPasswordEntered);
                }
                m_readPasswordJobs.removeAll(job);
            });
            m_readPasswordJobs << job;
        } else {
            auto password = m_resource->settings()->password();
            if (password.isEmpty()) {
                Q_EMIT done(EmptyPasswordEntered);
            } else {
                Q_EMIT done(PasswordRetrieved, password);
            }
        }
    }
}

void SettingsPasswordRequester::askUserInput(const QString &serverError)
{
    auto notification = new KNotification(QStringLiteral("imapAuthFailed"), KNotification::Persistent);
    notification->setComponentName(QStringLiteral("akonadi_imap_resource"));
    notification->setIconName(QStringLiteral("network-server"));
    notification->setTitle(i18nc("@title", "An IMAP e-mail account needs your attention."));

    const auto accountName = m_resource->name();
    const auto message = accountName.isEmpty() ? i18n(
                                                     "The IMAP server refused the supplied username and password.\n"
                                                     "Do you want to try again, or open the settings?\n\n"
                                                     "%1",
                                                     serverError)
                                               : i18n(
                                                     "The IMAP server for account %1 refused the supplied username and password.\n"
                                                     "Do you want to try again, or open the settings?\n\n"
                                                     "%2",
                                                     accountName,
                                                     serverError);
    notification->setText(message);

    auto tryAgainAction = notification->addAction(i18nc("@action:button", "Try again"));
    connect(tryAgainAction, &KNotificationAction::activated, this, [this, notification]() {
        disconnect(notification, &KNotification::closed, nullptr, nullptr);
        slotTryAgainClicked();
    });

    auto openSettingsAction = notification->addAction(i18nc("@action:button", "Open settings"));
    connect(openSettingsAction, &KNotificationAction::activated, this, [this, notification]() {
        disconnect(notification, &KNotification::closed, nullptr, nullptr);
        slotOpenSettingsClicked();
    });

    connect(notification, &KNotification::closed, this, [this] {
        Q_EMIT done(UserRejected);
    });

    notification->sendEvent();
}

void SettingsPasswordRequester::slotTryAgainClicked()
{
    requestPassword();
}

void SettingsPasswordRequester::slotOpenSettingsClicked()
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
    for (const auto job : std::as_const(m_readPasswordJobs)) {
        m_readPasswordJobs.removeAll(job);
        disconnect(job, &ReadPasswordJob::finished, this, nullptr);
    }
}

#include "moc_settingspasswordrequester.cpp"
