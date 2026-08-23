/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    OAuth2 (Authorization Code + PKCE) against Azure AD v2.0 endpoint, Graph scopes.
    Adapted from resources/ews/ewsclient/auth/ewsoauth.{h,cpp} — the flow is identical,
    only the authority endpoint (v2.0) and the requested scopes change.

    Token lifecycle:
      authenticate() -> read refresh token from the keychain
        -> found:   silent refresh (no UI); on failure fall back to interactive
        -> missing: interactive browser login (loopback redirect)
      granted -> persist new refresh token, schedule a proactive renewal shortly
                 before the access token expires, emit ready() (first time only).
*/

#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <memory>

class QOAuth2AuthorizationCodeFlow;
class QOAuthHttpServerReplyHandler;
class QTimer;

class GraphOAuth : public QObject
{
    Q_OBJECT
public:
    // tenantId: "common" (multi-tenant) or a concrete tenant GUID.
    // clientId: your Azure app registration's Application (client) ID.
    // walletKey: keychain entry name for the refresh token (use the resource instance id).
    GraphOAuth(const QString &tenantId, const QString &clientId, const QString &walletKey, QObject *parent = nullptr);
    ~GraphOAuth() override;

    /// Try a silent refresh (stored refresh token via QtKeychain); fall back to interactive.
    void authenticate();

    /// Drop persisted tokens (reconfigure/logout).
    void forgetTokens();

    /// Current bearer token, or empty if not yet authenticated.
    [[nodiscard]] QString accessToken() const;

Q_SIGNALS:
    /// First successful token acquisition. Later silent renewals do not re-emit.
    void ready();
    void failed(const QString &error);

private:
    [[nodiscard]] static QSet<QByteArray> graphScopes();

    void setUpFlow();
    void startSilentRefresh(const QString &refreshToken);
    void startInteractive();
    void onGranted();
    void onRequestFailed();
    void persistRefreshToken();
    void scheduleProactiveRefresh();

    QString mTenantId;
    QString mClientId;
    QString mWalletKey;
    QString mEnvToken; // GRAPH_ACCESS_TOKEN test hook
    bool mWasReady = false; // ready() already emitted
    bool mInteractive = false; // current attempt is the interactive flow
    std::unique_ptr<QOAuth2AuthorizationCodeFlow> mFlow;
    QOAuthHttpServerReplyHandler *mReplyHandler = nullptr;
    QTimer *mRefreshTimer = nullptr;
};
