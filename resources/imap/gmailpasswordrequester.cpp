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

#include <KGAPI/Account>
#include <KGAPI/AuthJob>
#include <KGAPI/AccountManager>

#define GOOGLE_API_KEY QStringLiteral("554041944266.apps.googleusercontent.com")
#define GOOGLE_API_SECRET QStringLiteral("mdT1DjzohxN3npUUzkENT0gO")

GmailPasswordRequester::GmailPasswordRequester(ImapResourceBase *resource, QObject *parent)
    : PasswordRequesterInterface(parent)
    , mResource(resource)
{
}

GmailPasswordRequester::~GmailPasswordRequester()
{
}

void GmailPasswordRequester::requestPassword(RequestType request, const QString &serverError)
{
    Q_UNUSED(serverError); // we don't get anything useful from XOAUTH2 SASL

    if (request == WrongPasswordRequest) {
        auto promise = KGAPI2::AccountManager::instance()->findAccount(GOOGLE_API_KEY, mResource->settings()->userName());
        connect(promise, &KGAPI2::AccountPromise::finished,
                this, [this](KGAPI2::AccountPromise *promise) {
            if (promise->account()) {
                promise = KGAPI2::AccountManager::instance()->refreshTokens(
                    GOOGLE_API_KEY, GOOGLE_API_SECRET, mResource->settings()->userName());
            } else {
                promise = KGAPI2::AccountManager::instance()->getAccount(
                    GOOGLE_API_KEY, GOOGLE_API_SECRET, mResource->settings()->userName(),
                    { KGAPI2::Account::mailScopeUrl() });
            }
            connect(promise, &KGAPI2::AccountPromise::finished,
                    this, &GmailPasswordRequester::onTokenRequestFinished);
            mPendingPromise = promise;
        });
        mPendingPromise = promise;
    } else {
        auto promise = KGAPI2::AccountManager::instance()->getAccount(
            GOOGLE_API_KEY, GOOGLE_API_SECRET, mResource->settings()->userName(),
            { KGAPI2::Account::mailScopeUrl() });
        connect(promise, &KGAPI2::AccountPromise::finished,
                this, &GmailPasswordRequester::onTokenRequestFinished);
        mPendingPromise = promise;
    }
}

void GmailPasswordRequester::cancelPasswordRequests()
{
    if (mPendingPromise) {
        mPendingPromise->disconnect(this);
    }
}

void GmailPasswordRequester::onTokenRequestFinished(KGAPI2::AccountPromise *promise)
{
    mPendingPromise = nullptr;
    if (promise->hasError()) {
        Q_EMIT done(UserRejected, promise->errorText());
    } else {
        Q_EMIT done(PasswordRetrieved, promise->account()->accessToken());
    }
}
