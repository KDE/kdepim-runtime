/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "gmailpasswordrequester.h"
#include "gmailsettings.h"
#include "gmailresource.h"

#include <libkgapi2/account.h>
#include <libkgapi2/authjob.h>

#include <qjson/parser.h>

#include <KDebug>

/**
 * See https://developers.google.com/gmail/xoauth2_protocol for protocol documentation
 */

GmailPasswordRequester::GmailPasswordRequester(GmailResource *resource, QObject *parent)
    : PasswordRequesterInterface(parent)
    , mResource(resource)
{
}

GmailPasswordRequester::~GmailPasswordRequester()
{
}

void GmailPasswordRequester::requestPassword(PasswordRequesterInterface::RequestType request, const QString &serverError)
{
    if (request == PasswordRequesterInterface::WrongPasswordRequest) {
        connect(mResource->settings(), SIGNAL(accountRequestCompleted(KGAPI2::AccountPtr,bool)),
                this, SLOT(onAuthFinished(KGAPI2::AccountPtr,bool)),
                Qt::UniqueConnection);
        static_cast<GmailSettings*>(mResource->settings())->requestAccount(true);
    } else {
        QMetaObject::invokeMethod(this, "done", Qt::QueuedConnection,
                                  Q_ARG(int, PasswordRetrieved),
                                  Q_ARG(QString, mResource->settings()->password()));
    }
}

bool GmailPasswordRequester::isTokenExpired(const QString &serverError)
{
    QJson::Parser parser;

    QString base64Error = serverError.mid(7, serverError.length() - 9);
    const QByteArray decoded = QByteArray::fromBase64(base64Error.toLatin1());
    bool ok = false;
    const QVariant json = parser.parse(decoded, &ok);
    if (!ok) {
        return false;
    }

    const QVariantMap map = json.toMap();
    if (map[QLatin1String("status")].toString().toInt() == KGAPI2::Unauthorized) {
        return true;
    }

    kDebug() << "Gmail Auth error:" << json;
    return false;
}

void GmailPasswordRequester::onAuthFinished(const KGAPI2::AccountPtr &account, bool userRejected)
{
    if (userRejected) {
        done(UserRejected);
        return;
    }

    if (!account) {
        // Really??
        done(ReconnectNeeded);
        return;
    }

    // Access Token is not really a password, but meh...
    done(PasswordRetrieved, account->accessToken());
}
