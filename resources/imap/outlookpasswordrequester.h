/*
    SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "passwordrequesterinterface.h"

namespace KWallet
{
class Wallet;
} // namespace KWallet

class ImapResourceBase;
class OutlookOAuthTokenRequester;
class TokenResult;

class OutlookPasswordRequester : public XOAuthPasswordRequester
{
    Q_OBJECT
public:
    explicit OutlookPasswordRequester(ImapResourceBase *resource, QObject *parent = nullptr);
    ~OutlookPasswordRequester() override;

    void requestPassword(RequestType request, const QString &serverError) override;
    void cancelPasswordRequests() override;

private:
    void onTokenRequestFinished(KWallet::Wallet *wallet, const TokenResult &result);
    void storeResultToWallet(KWallet::Wallet *wallet, const TokenResult &result);
    QString loadTokenFromKWallet(KWallet::Wallet *wallet, const QString &tokenType);

    ImapResourceBase *const mResource;
    std::unique_ptr<OutlookOAuthTokenRequester> mTokenRequester;
};
