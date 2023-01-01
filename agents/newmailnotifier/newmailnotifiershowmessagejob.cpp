/*
   SPDX-FileCopyrightText: 2014-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "newmailnotifiershowmessagejob.h"
#include "newmailnotifier_debug.h"
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>

#include <KService>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>

NewMailNotifierShowMessageJob::NewMailNotifierShowMessageJob(Akonadi::Item::Id id, QObject *parent)
    : KJob(parent)
    , mId(id)
{
}

NewMailNotifierShowMessageJob::~NewMailNotifierShowMessageJob() = default;

void NewMailNotifierShowMessageJob::start()
{
    if (mId < 0) {
        emitResult();
        return;
    }

    // start the kmail application if it isn't running yet
    const auto service = KService::serviceByDesktopName(QStringLiteral("org.kde.kmail2"));
    if (!service) {
        qCDebug(NEWMAILNOTIFIER_LOG) << " Can not start kmail";
        setError(UserDefinedError);
        setErrorText(i18n("Unable to start KMail application."));
        emitResult();
        return;
    }

    auto job = new KIO::ApplicationLauncherJob(service, this);
    connect(job, &KJob::finished, this, [this]() {
        const QString kmailInterface = QStringLiteral("org.kde.kmail");
        QDBusInterface kmail(kmailInterface, QStringLiteral("/KMail"), QStringLiteral("org.kde.kmail.kmail"));
        if (kmail.isValid()) {
            kmail.call(QStringLiteral("showMail"), mId);
        } else {
            qCWarning(NEWMAILNOTIFIER_LOG) << "Impossible to access to DBus interface";
        }
        emitResult();
    });
    job->start();
}
