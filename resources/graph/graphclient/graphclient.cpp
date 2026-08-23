/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphclient.h"

#include <QNetworkAccessManager>

GraphClient::GraphClient()
    : mBaseUrl(QStringLiteral("https://graph.microsoft.com/v1.0"))
    , mNam(std::make_unique<QNetworkAccessManager>())
{
}

GraphClient::~GraphClient() = default;

void GraphClient::setBaseUrl(const QString &baseUrl)
{
    mBaseUrl = baseUrl;
}

void GraphClient::setAuth(GraphOAuth *auth)
{
    mAuth = auth;
}

QString GraphClient::baseUrl() const
{
    return mBaseUrl;
}

GraphOAuth *GraphClient::auth() const
{
    return mAuth;
}

QNetworkAccessManager *GraphClient::networkAccessManager() const
{
    return mNam.get();
}
