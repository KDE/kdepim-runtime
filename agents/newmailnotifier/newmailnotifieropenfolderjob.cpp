/*
   SPDX-FileCopyrightText: 2023-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "newmailnotifieropenfolderjob.h"
#include "newmailnotifier_debug.h"

#include <KLocalizedString>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>

NewMailNotifierOpenFolderJob::NewMailNotifierOpenFolderJob(const QString &identifier, QObject *parent)
    : KJob{parent}
    , mIdentifer(identifier)
{
}

NewMailNotifierOpenFolderJob::~NewMailNotifierOpenFolderJob() = default;

void NewMailNotifierOpenFolderJob::start()
{
    const qint64 identifier = mIdentifer.toLong();
    if (identifier < 0) {
        emitResult();
        return;
    }

    const QString kmailInterface = QStringLiteral("org.kde.kmail");
    auto startReply = QDBusConnection::sessionBus().interface()->startService(kmailInterface);
    if (!startReply.isValid()) {
        qCDebug(NEWMAILNOTIFIER_LOG) << "Can not start kmail";
        setError(UserDefinedError);
        setErrorText(i18n("Unable to start KMail application."));
        emitResult();
        return;
    }

    QDBusInterface kmail(kmailInterface, QStringLiteral("/KMail"), QStringLiteral("org.kde.kmail.kmail"));
    if (kmail.isValid()) {
        kmail.call(QStringLiteral("selectFolder"), mIdentifer);
    } else {
        qCWarning(NEWMAILNOTIFIER_LOG) << "Impossible to access the DBus interface";
    }

    emitResult();
}

#include "moc_newmailnotifieropenfolderjob.cpp"
