/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "newmailnotifiershowmessagejob.h"
#include "newmailnotifier_debug.h"
#include <KLocalizedString>

#include <KToolInvocation>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>

NewMailNotifierShowMessageJob::NewMailNotifierShowMessageJob(Akonadi::Item::Id id, QObject *parent)
    : KJob(parent)
    , mId(id)
{
}

NewMailNotifierShowMessageJob::~NewMailNotifierShowMessageJob()
{
}

void NewMailNotifierShowMessageJob::start()
{
    if (mId < 0) {
        emitResult();
        return;
    }
    const QString kmailInterface = QStringLiteral("org.kde.kmail");
    QDBusReply<bool> reply = QDBusConnection::sessionBus().interface()->isServiceRegistered(kmailInterface);
    if (!reply.isValid() || !reply.value()) {
        // Program is not already running, so start it
        QString errmsg;
        if (KToolInvocation::startServiceByDesktopName(QStringLiteral("org.kde.kmail2"), QString(), &errmsg)) {
            qCDebug(NEWMAILNOTIFIER_LOG) << " Can not start kmail" << errmsg;
            setError(UserDefinedError);
            setErrorText(i18n("Unable to start KMail application."));
            emitResult();
            return;
        }
    }
    QDBusInterface kmail(kmailInterface, QStringLiteral("/KMail"), QStringLiteral("org.kde.kmail.kmail"));
    if (kmail.isValid()) {
        kmail.call(QStringLiteral("showMail"), mId);
    } else {
        qCWarning(NEWMAILNOTIFIER_LOG) << "Impossible to access to DBus interface";
    }
    emitResult();
}
