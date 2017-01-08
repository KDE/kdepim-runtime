/*
    Copyright (c) 2016 Daniel Vrátil <dvratil@kde.org>

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

#include "gmailpasswordrequester.h"
#include "imapresourcebase.h"
#include "settings.h"
#include "imapresource_debug.h"

#include <KGAPI/kgapi/account.h>
#include <KGAPI/kgapi/authjob.h>

#define GOOGLE_API_KEY QStringLiteral("554041944266.apps.googleusercontent.com")
#define GOOGLE_API_SECRET QStringLiteral("mdT1DjzohxN3npUUzkENT0gO")

GmailPasswordRequester::GmailPasswordRequester(ImapResourceBase *resource, QObject *parent)
    : PasswordRequesterInterface(parent)
    , mResource(resource)
    , mRunningRequest(nullptr)
{
}

GmailPasswordRequester::~GmailPasswordRequester()
{
}

void GmailPasswordRequester::requestPassword(RequestType request, const QString &serverError)
{
    Q_UNUSED(serverError); // we don't get anything useful from XOAUTH2 SASL

    if (request == WrongPasswordRequest) {
        const QString tokens = mResource->settings()->password();
        if (tokens.isEmpty()) {
            requestToken();
        } else {
            const QString token = tokens.mid(tokens.indexOf(QLatin1Char('\001')) + 1);
            if (token.isEmpty() || token == tokens) {
                requestToken(token); // if token == tokens, assume it's account password
            } else {
                refreshToken(token);
            }
        }
    } else {
        connect(mResource->settings(), &Settings::passwordRequestCompleted,
                this, &GmailPasswordRequester::onPasswordRequestCompleted);
        mResource->settings()->requestPassword();
    }
}

void GmailPasswordRequester::cancelPasswordRequests()
{
    if (mRunningRequest) {
        mRunningRequest->disconnect(this);
        mRunningRequest->deleteLater();
    }
}

void GmailPasswordRequester::onPasswordRequestCompleted(const QString &password, bool userRejected)
{
    disconnect(mResource->settings(), &Settings::passwordRequestCompleted,
               this, &GmailPasswordRequester::onPasswordRequestCompleted);

    QString token = password;
    if (userRejected || token.isEmpty()) {
        requestToken();
        return;
    } else {
        token = password.left(password.indexOf(QLatin1Char('\001')));
        if (token.isEmpty() || token == password) {
            requestToken(password); // if token == password, assume it's account password
            return;
        }
    }

    if (userRejected) {
        Q_EMIT done(UserRejected);
    } else {
        Q_EMIT done(PasswordRetrieved, token);
    }
}

void GmailPasswordRequester::requestToken(const QString &password)
{
    Q_ASSERT(!mRunningRequest);
    auto acc = KGAPI2::AccountPtr::create(mResource->settings()->userName(),
                                          QString(), QString(),
                                          QList<QUrl>{ QUrl(QStringLiteral("https://mail.google.com/")) });

    auto authJob = new KGAPI2::AuthJob(acc, GOOGLE_API_KEY, GOOGLE_API_SECRET, this);
    authJob->setUsername(mResource->settings()->userName());
    authJob->setPassword(password);
    connect(authJob, &KGAPI2::Job::finished,
            this, &GmailPasswordRequester::onTokenRequestFinished);
    mRunningRequest = authJob;
}

void GmailPasswordRequester::onTokenRequestFinished(KGAPI2::Job *job)
{
    mRunningRequest.clear();

    auto authJob = qobject_cast<KGAPI2::AuthJob*>(job);
    if (authJob->error()) {
        qCWarning(IMAPRESOURCE_LOG) << "Error obtaining XOAUTH2 token:" << authJob->errorString();
        Q_EMIT done(UserRejected);
        return;
    }

    const auto account = authJob->account();
    const QString tokens = QStringLiteral("%1\001%2").arg(account->accessToken(),
                                                          account->refreshToken());
    mResource->settings()->setPassword(tokens);
    Q_EMIT done(PasswordRetrieved, account->accessToken());
}

void GmailPasswordRequester::refreshToken(const QString &refreshToken)
{
    Q_ASSERT(!mRunningRequest);
    auto acc = KGAPI2::AccountPtr::create(mResource->settings()->userName(),
                                          QString(), refreshToken,
                                          QList<QUrl>{ QUrl(QStringLiteral("https://mail.google.com/")) });
    auto authJob = new KGAPI2::AuthJob(acc, GOOGLE_API_KEY, GOOGLE_API_SECRET, this);
    authJob->setUsername(mResource->settings()->userName());
    connect(authJob, &KGAPI2::Job::finished,
            this, &GmailPasswordRequester::onTokenRequestFinished);
    mRunningRequest = authJob;
}
