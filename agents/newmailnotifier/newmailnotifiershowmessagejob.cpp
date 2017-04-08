/*
   Copyright (C) 2014-2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "newmailnotifiershowmessagejob.h"
#include "newmailnotifier_debug.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusConnectionInterface>
#include <ktoolinvocation.h>

NewMailNotifierShowMessageJob::NewMailNotifierShowMessageJob(Akonadi::Item::Id id, QObject *parent)
    : KJob(parent),
      mId(id)
{
}

NewMailNotifierShowMessageJob::~NewMailNotifierShowMessageJob()
{
}

void NewMailNotifierShowMessageJob::start()
{
    if (mId < 0) {
        Q_EMIT emitResult();
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
            Q_EMIT emitResult();
            return;
        }
    }
    QDBusInterface kmail(kmailInterface, QStringLiteral("/KMail"), QStringLiteral("org.kde.kmail.kmail"));
    if (kmail.isValid()) {
        kmail.call(QStringLiteral("showMail"), mId);
    }
    Q_EMIT emitResult();
}
