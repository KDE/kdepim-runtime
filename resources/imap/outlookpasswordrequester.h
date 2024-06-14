/*
    SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "passwordrequesterinterface.h"

namespace MailTransport
{
class OutlookOAuthTokenRequester;
class TokenResult;
}

class ImapResourceBase;

class OutlookPasswordRequester : public XOAuthPasswordRequester
{
    Q_OBJECT
public:
    explicit OutlookPasswordRequester(ImapResourceBase *resource, QObject *parent = nullptr);
    ~OutlookPasswordRequester() override;

    void requestPassword(RequestType request, const QString &serverError) override;
    void cancelPasswordRequests() override;

private:
    void onTokenRequestFinished(const MailTransport::TokenResult &result);
    void storeResultToWallet(const MailTransport::TokenResult &result);

    ImapResourceBase *const mResource;
    std::unique_ptr<MailTransport::OutlookOAuthTokenRequester> mTokenRequester;
    bool mRequestInProgress = false;
};
