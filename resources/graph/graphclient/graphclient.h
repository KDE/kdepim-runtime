/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Thin Graph REST client: owns the QNetworkAccessManager and the bearer token source.
    Equivalent role to EwsClient, but REST/JSON instead of SOAP/XML.
*/

#pragma once

#include <QString>
#include <memory>

class QNetworkAccessManager;
class GraphOAuth;

class GraphClient
{
public:
    GraphClient();
    ~GraphClient();

    void setBaseUrl(const QString &baseUrl); // https://graph.microsoft.com/v1.0
    void setAuth(GraphOAuth *auth); // token provider (not owned)

    [[nodiscard]] QString baseUrl() const;
    [[nodiscard]] GraphOAuth *auth() const;
    [[nodiscard]] QNetworkAccessManager *networkAccessManager() const;

private:
    QString mBaseUrl;
    GraphOAuth *mAuth = nullptr; // not owned
    std::unique_ptr<QNetworkAccessManager> mNam;
};
