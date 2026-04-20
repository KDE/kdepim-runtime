/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "passwordrequesterinterface.h"

#include <QPointer>

class ImapResource;

namespace KGAPI2
{
class AccountPromise;
}

class GmailPasswordRequester : public XOAuthPasswordRequester
{
    Q_OBJECT
public:
    explicit GmailPasswordRequester(ImapResource *resource, QObject *parent = nullptr);
    ~GmailPasswordRequester() override;

    void requestPassword(RequestType request, const QString &serverError) override;
    void cancelPasswordRequests() override;

private:
    void onTokenRequestFinished(KGAPI2::AccountPromise *promise);
    ImapResource *const mResource;
    QPointer<KGAPI2::AccountPromise> mPendingPromise;
};
