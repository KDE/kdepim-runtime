/*
    SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "outlookpasswordrequester.h"
#include "imapresource_debug.h"
#include "imapresourcebase.h"
#include "passwordrequesterinterface.h"
#include "settings.h"

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator64>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

#include <KLocalizedString>
#include <KWallet>
#include <memory>
#include <qcontainerfwd.h>
#include <qloggingcategory.h>
#include <qnetworkrequest.h>
#include <qstringliteral.h>

static const char16_t *tenantId = u"common";
static const char16_t *clientId = u"18da2bc3-146a-4581-8c92-27dc7b9954a0";
static const char16_t *scope = u"https://outlook.office.com/IMAP.AccessAsUser.All offline_access";

class TokenResult
{
public:
    TokenResult(int errorCode, const QString &errorText)
        : mErrorCode(errorCode)
        , mErrorText(errorText)
    {
    }

    TokenResult(const QString &accessToken, const QString &refreshToken)
        : mAccessToken(accessToken)
        , mRefreshToken(refreshToken)
    {
    }

    QString accessToken() const
    {
        return mAccessToken;
    }

    QString refreshToken() const
    {
        return mRefreshToken;
    }

    bool hasError() const
    {
        return mErrorCode != 0;
    }

    int errorCode() const
    {
        return mErrorCode;
    }

    QString errorText() const
    {
        return mErrorText;
    }

private:
    int mErrorCode;
    QString mErrorText;
    QString mAccessToken;
    QString mRefreshToken;
};

Q_DECLARE_METATYPE(TokenResult);

/// Helper class to generate PKCE verifier and challenge (RFC 7636)
class PKCE
{
public:
    explicit PKCE()
    {
        mVerifier = generateRandomString(128);
        mChallenge = generateChallenge(mVerifier);
    }

    QString challenge() const
    {
        return mChallenge;
    }

    QString verifier() const
    {
        return mVerifier;
    }

private:
    QString generateRandomString(std::size_t length)
    {
        static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-._`~";

        auto generator = QRandomGenerator::securelySeeded();
        QString result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            const int idx = generator.bounded(static_cast<int>(sizeof(charset) - 1));
            result.append(QChar::fromLatin1(charset[idx]));
        }
        return result;
    }

    QString generateChallenge(const QString &verifier)
    {
        const auto sha256 = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
        return QString::fromLatin1(sha256.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    }

private:
    QString mVerifier;
    QString mChallenge;
};

class OutlookOAuthTokenRequester : public QObject
{
    Q_OBJECT
public:
    explicit OutlookOAuthTokenRequester(ImapResourceBase *resource, QObject *parent = nullptr)
        : QObject(parent)
        , mResource(resource)
    {
    }

    void requestToken()
    {
        qCDebug(IMAPRESOURCE_LOG) << "Requesting new Outlook OAuth2 access token";

        auto redirectUri = startLocalHttpServer();
        if (!redirectUri.has_value()) {
            Q_EMIT finished({-1, i18nc("@status", "Failed to start local HTTP server to receive Outlook OAuth2 authorization code")});
        }
        mRedirectUri = *redirectUri;

        QUrl url(QStringLiteral("https://login.microsoftonline.com/%1/oauth2/v2.0/authorize").arg(QStringView{tenantId}));
        url.setQuery({{QStringLiteral("client_id"), QString::fromUtf16(clientId)},
                      {QStringLiteral("redirect_uri"), mRedirectUri.toString()},
                      {QStringLiteral("response_type"), QStringLiteral("code")},
                      {QStringLiteral("response_mode"), QStringLiteral("query")},
                      {QStringLiteral("scope"), QString::fromUtf16(scope)},
                      {QStringLiteral("login_hint"), mResource->settings()->userName()},
                      {QStringLiteral("code_challenge"), mPkce.challenge()},
                      {QStringLiteral("code_challenge_method"), QStringLiteral("S256")}});

        qCDebug(IMAPRESOURCE_LOG) << "Browser opened, waiting for Outlook OAuth2 authorization code...";
        QDesktopServices::openUrl(url);
    }

    void refreshToken(const QString &refreshToken)
    {
        qCDebug(IMAPRESOURCE_LOG) << "Refreshing Outlook OAuth2 access token";

        QUrl url(QStringLiteral("https://login.microsoftonline.com/%1/oauth2/v2.0/token").arg(QStringView{tenantId}));

        QNetworkRequest request{url};
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
        mNam = std::make_unique<QNetworkAccessManager>();
        auto *reply = mNam->post(request,
                                 QUrlQuery{{QStringLiteral("client_id"), QString::fromUtf16(clientId)},
                                           {QStringLiteral("grant_type"), QStringLiteral("refresh_token")},
                                           {QStringLiteral("scope"), QString::fromUtf16(scope)},
                                           {QStringLiteral("refresh_token"), refreshToken}}
                                     .toString(QUrl::FullyEncoded)
                                     .toUtf8());
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            handleTokenResponse(reply, true);
        });
    }

Q_SIGNALS:
    void finished(const TokenResult &result);

private:
    std::optional<QUrl> startLocalHttpServer()
    {
        mHttpServer = std::make_unique<QTcpServer>();
        connect(mHttpServer.get(), &QTcpServer::newConnection, this, &OutlookOAuthTokenRequester::handleNewConnection);
        if (!mHttpServer->listen(QHostAddress::LocalHost)) {
            return {};
        }
        qCDebug(IMAPRESOURCE_LOG) << "Local Outlook OAuth2 server listening on port" << mHttpServer->serverPort();

        return QUrl(QStringLiteral("http://localhost:%1").arg(mHttpServer->serverPort()));
    }

    void handleNewConnection()
    {
        qCDebug(IMAPRESOURCE_LOG) << "New incoming connection from Outlook OAuth2";
        mSocket = std::unique_ptr<QTcpSocket>(mHttpServer->nextPendingConnection());
        connect(mSocket.get(), &QTcpSocket::readyRead, this, &OutlookOAuthTokenRequester::handleSocketReadyRead);
    }

    void handleSocketReadyRead()
    {
        auto request = mSocket->readLine();
        mSocket->readAll(); // read the rest of data and discard it

        sendResponseToBrowserAndCloseSocket();

        if (!request.startsWith("GET /?") && !request.endsWith("HTTP/1.1")) {
            Q_EMIT finished({-1, i18nc("@status", "Invalid authorization response from server")});
            return;
        }

        // Remove verb and protocol from the request line
        request.remove(0, sizeof("GET ") - 1);
        request.truncate(request.size() - sizeof(" HTTP/1.1") - 1);
        // Prefix it with protocol and domain so that it's a full URL that we can parse
        request.prepend("http://localhost");

        // Try to parse the URL
        QUrl url(QString::fromUtf8(request));
        if (!url.isValid()) {
            qCWarning(IMAPRESOURCE_LOG) << "Failed to extract valid URL from initial HTTP request line from Outlook OAuth2:" << request;
            Q_EMIT finished({-1, i18nc("@status", "Invalid authorization response from server")});
            return;
        }

        // Extract code
        const QUrlQuery query(url);
        if (query.hasQueryItem(QStringLiteral("error"))) {
            const auto error = query.queryItemValue(QStringLiteral("error"));
            const auto errorDescription = query.queryItemValue(QStringLiteral("error_description"));
            qCWarning(IMAPRESOURCE_LOG) << "Authorization server returned error:" << error << errorDescription;
            Q_EMIT finished({-1, i18nc("@status", "Authorization server returned error: %1", errorDescription)});
            return;
        }

        const auto code = query.queryItemValue(QStringLiteral("code"));
        if (code.isEmpty()) {
            qCWarning(IMAPRESOURCE_LOG) << "Failed to extract authorization code from Outlook OAuth2 response:" << request;
            Q_EMIT finished({-1, i18nc("@status", "Invalid authorization response from server")});
            return;
        }

        qCDebug(IMAPRESOURCE_LOG) << "Extracted Outlook OAuth2 autorization token from response, requesting access token...";
        requestIdToken(code);
    }

    void requestIdToken(const QString &code)
    {
        QUrl url(QStringLiteral("https://login.microsoftonline.com/%1/oauth2/v2.0/token").arg(QStringView{tenantId}));

        QNetworkRequest request{url};
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
        mNam = std::make_unique<QNetworkAccessManager>();
        auto *reply = mNam->post(request,
                                 QUrlQuery{{QStringLiteral("client_id"), QString::fromUtf16(clientId)},
                                           {QStringLiteral("scope"), QString::fromUtf16(scope)},
                                           {QStringLiteral("code"), code},
                                           {QStringLiteral("redirect_uri"), mRedirectUri.toString()},
                                           {QStringLiteral("grant_type"), QStringLiteral("authorization_code")},
                                           {QStringLiteral("code_verifier"), mPkce.verifier()}}
                                     .toString(QUrl::FullyEncoded)
                                     .toUtf8());
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            handleTokenResponse(reply);
        });
        qCDebug(IMAPRESOURCE_LOG) << "Requested Outlook OAuth2 access token, waiting for response...";
    }

    void handleTokenResponse(QNetworkReply *reply, bool isTokenRefresh = false)
    {
        const auto responseData = reply->readAll();
        reply->deleteLater();

        const auto response = QJsonDocument::fromJson(responseData);
        if (!response.isObject()) {
            qCWarning(IMAPRESOURCE_LOG) << "Failed to parse token response:" << responseData;
            Q_EMIT finished({-1, i18nc("@status", "Failed to parse token response")});
            return;
        }

        if (response[QStringView{u"error"}].isString()) {
            const auto error = response[QStringView{u"error"}].toString();
            const auto errorDescription = response[QStringView{u"error_description"}].toString();
            qCWarning(IMAPRESOURCE_LOG) << "Outlook OAuth2 authorization server returned error:" << error << errorDescription;

            if (isTokenRefresh && error == u"invalid_grant") {
                qCDebug(IMAPRESOURCE_LOG) << "Outlook OAuth2 refresh token is invalid, requesting new token...";
                requestToken();
            } else {
                Q_EMIT finished({-1, i18nc("@status", "Authorization server returned error: %1", errorDescription)});
            }
            return;
        }

        const auto accessToken = response[QStringView{u"access_token"}].toString();
        const auto refreshToken = response[QStringView{u"refresh_token"}].toString();

        qCDebug(IMAPRESOURCE_LOG) << "Received Outlook OAuth2 access and refresh tokens";

        Q_EMIT finished({accessToken, refreshToken});
    }

    void sendResponseToBrowserAndCloseSocket()
    {
        const auto response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html>"
            "<head><title>KDE PIM Authorization</title></head>"
            "<body>"
            "<h1>You can close the browser window now and return to the application.</h1>"
            "</body>"
            "</html>\r\n\r\n";

        mSocket->write(response);
        mSocket->flush();
        mSocket->close();

        mSocket.release()->deleteLater();

        mHttpServer->close();
        mHttpServer.release()->deleteLater();

        qCDebug(IMAPRESOURCE_LOG) << "Sent HTTP OK response to browser and closed our local HTTP server.";
    }

private:
    PKCE mPkce;
    ImapResourceBase *mResource;
    QUrl mRedirectUri;
    std::unique_ptr<QTcpServer> mHttpServer;
    std::unique_ptr<QTcpSocket> mSocket;
    std::unique_ptr<QNetworkAccessManager> mNam;
};

OutlookPasswordRequester::OutlookPasswordRequester(ImapResourceBase *resource, QObject *parent)
    : XOAuthPasswordRequester(parent)
    , mResource(resource)
{
}

OutlookPasswordRequester::~OutlookPasswordRequester() = default;

QString OutlookPasswordRequester::loadTokenFromKWallet(KWallet::Wallet *wallet, const QString &tokenType)
{
    if (!wallet->hasFolder(QStringLiteral("imap"))) {
        return {};
    }

    wallet->setFolder(QStringLiteral("imap"));
    QMap<QString, QString> result;
    wallet->readMap(mResource->settings()->config()->name(), result);
    auto refreshToken = result.constFind(tokenType);
    if (refreshToken == result.cend()) {
        return {};
    }

    return *refreshToken;
}

void OutlookPasswordRequester::requestPassword(RequestType request, const QString &serverError)
{
    Q_UNUSED(serverError) // we don't get anything useful from XOAUTH2 SASL

    auto *wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Asynchronous);
    connect(wallet, &KWallet::Wallet::walletOpened, this, [this, wallet, request](bool success) {
        if (!success) {
            Q_EMIT done(UserRejected, i18nc("@status", "Failed to open KWallet"));
            return;
        }

        mTokenRequester = std::make_unique<OutlookOAuthTokenRequester>(mResource);
        connect(mTokenRequester.get(), &OutlookOAuthTokenRequester::finished, this, [this, wallet](const auto &result) {
            onTokenRequestFinished(wallet, result);
        });

        if (request == WrongPasswordRequest) {
            const auto refreshToken = loadTokenFromKWallet(wallet, QStringLiteral("refreshToken"));
            if (!refreshToken.isEmpty()) {
                qCDebug(IMAPRESOURCE_LOG) << "Found an Outlook OAuth2 refresh token in KWallet, refreshing access token...";
                mTokenRequester->refreshToken(refreshToken);
            } else {
                qCDebug(IMAPRESOURCE_LOG) << "No Outlook OAuth2 refresh token found in KWallet, requesting new token...";
                mTokenRequester->requestToken();
            }
        } else {
            const auto accessToken = loadTokenFromKWallet(wallet, QStringLiteral("accessToken"));
            if (accessToken.isEmpty()) {
                qCDebug(IMAPRESOURCE_LOG) << "No Outlook OAuth2 access token found in KWallet, requesting new token...";
                mTokenRequester->requestToken();
            } else {
                qCDebug(IMAPRESOURCE_LOG) << "Found an Outlook OAuth2 access token in KWallet, using it...";
                Q_EMIT done(PasswordRetrieved, accessToken);
            }
        }
    });
}

void OutlookPasswordRequester::cancelPasswordRequests()
{
    if (mTokenRequester) {
        mTokenRequester->disconnect(this);
        mTokenRequester.release()->deleteLater();
        qCDebug(IMAPRESOURCE_LOG) << "Canceled Outlook OAuth2 token request";
    }
}

void OutlookPasswordRequester::storeResultToWallet(KWallet::Wallet *wallet, const TokenResult &result)
{
    const auto name = mResource->settings()->config()->name();
    qCDebug(IMAPRESOURCE_LOG).nospace().noquote() << "Storing Outlook OAuth2 token to KWallet (imap/" << name << ")";
    if (!wallet->hasFolder(QStringLiteral("imap"))) {
        wallet->createFolder(QStringLiteral("imap"));
    }
    wallet->setFolder(QStringLiteral("imap"));
    const int ok = wallet->writeMap(name, {{QStringLiteral("accessToken"), result.accessToken()}, {QStringLiteral("refreshToken"), result.refreshToken()}});
    if (ok != 0) {
        qCWarning(IMAPRESOURCE_LOG) << "Failed to store Outlook OAuth2 token to KWallet:" << ok;
    }
}

void OutlookPasswordRequester::onTokenRequestFinished(KWallet::Wallet *wallet, const TokenResult &result)
{
    mTokenRequester.release()->deleteLater();
    if (result.hasError()) {
        Q_EMIT done(UserRejected, result.errorText());
    } else {
        storeResultToWallet(wallet, result);
        Q_EMIT done(PasswordRetrieved, result.accessToken());
    }
}

#include "outlookpasswordrequester.moc"
