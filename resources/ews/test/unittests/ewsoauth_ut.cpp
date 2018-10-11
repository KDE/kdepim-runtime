/*
    Copyright (C) 2018 Krzysztof Nowicki <krissn@op.pl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <functional>
#include <QtTest>

#include "ewsoauth.h"

#include "ewsoauth_ut_mock.h"

static const QString testEmail = QStringLiteral("joe.bloggs@unknown.com");
static const QString testClientId = QStringLiteral("b43c59cd-dd1c-41fd-bb9a-b0a1d5696a93");
static const QString testReturnUri = QStringLiteral("urn:ietf:wg:oauth:2.0:oob");
static const QString testReturnUriPercent = QUrl::toPercentEncoding(testReturnUri);
static const QString testState = QStringLiteral("joidsiuhq");
static const QString resource = QStringLiteral("https://outlook.office365.com/");
static const QString resourcePercent = QUrl::toPercentEncoding(resource);
static const QString authUrl = QStringLiteral("https://login.microsoftonline.com/common/oauth2/authorize");
static const QString tokenUrl = QStringLiteral("https://login.microsoftonline.com/common/oauth2/token");

static const QString accessToken1 = QStringLiteral("IERbOTo5NSdtY5HMntWTH1wgrRt98KmbF7nNloIdZ4SSYOU7pziJJakpHy8r6kxQi+7T9w36mWv9IWLrvEwTsA");
static const QString refreshToken1 = QStringLiteral("YW7lJFWcEISynbraq4NiLLke3rOieFdvoJEDxpjCXorJblIGM56OJSu1PZXMCQL5W3KLxS9ydxqLHxRTSdw");
static const QString idToken1 = QStringLiteral("gz7l0chu9xIi1MMgPkpHGQTmo3W7L1rQbmWAxEL5VSKHeqdIJ7E3K7vmMYTl/C1fWihB5XiLjD2GSVQoOzTfCw");


class UtEwsOAuth : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initialAuthSuccessful();
private:
    static QString formatJsonSorted(const QVariantMap &map);
};

void UtEwsOAuth::initialAuthSuccessful()
{
    EwsOAuth oAuth(nullptr, testEmail, testClientId, testReturnUri);

    QVERIFY(Mock::QWebEngineView::instance);
    QVERIFY(Mock::QOAuth2AuthorizationCodeFlow::instance);

    QEventLoop loop;
    bool browserRequest = false;
    QStringList events;
    
    connect(&oAuth, &EwsOAuth::granted, this, [&loop]() {
            qDebug() << "granted";
            loop.exit(0);
        });
    connect(&oAuth, &EwsOAuth::error, this, [&loop](const QString &msg, const QString &descr, const QUrl &url) {
            qDebug() << "error" << msg << descr << url;
            loop.exit(1);
        });
    QTimer timer;
    connect(&timer, &QTimer::timeout, this, [&loop]() {
            qDebug() << "timeout";
            loop.exit(1);            
        });
    connect(&oAuth, &EwsOAuth::browserDisplayRequest, this, [&]() {
            events.append("BrowserDisplayRequest");
            browserRequest = true;
            oAuth.browserDisplayReply(true);
        });
    connect(Mock::QWebEngineView::instance.data(), &Mock::QWebEngineView::logEvent, this, [&](const QString &event) {
            events.append(event);
        });
    connect(Mock::QOAuth2AuthorizationCodeFlow::instance.data(), &Mock::QOAuth2AuthorizationCodeFlow::logEvent, this,
            [&](const QString &event) {
                events.append(event);
            });

    Mock::QWebEngineView::instance->setRedirectUri(Mock::QOAuth2AuthorizationCodeFlow::instance->redirectUri());
    auto time = QDateTime::currentSecsSinceEpoch();

    constexpr unsigned int tokenLifetime = 86399;
    constexpr unsigned int extTokenLifetime = 345599;
    QString tokenReplyData;

    Mock::QWebEngineView::instance->setAuthFunction([&](const QUrl &, QVariantMap &map){
            map[QStringLiteral("code")] = QUrl::toPercentEncoding(refreshToken1);
        });
    Mock::QOAuth2AuthorizationCodeFlow::instance->setTokenFunction(
        [&](QString &data, QMap<Mock::QNetworkRequest::KnownHeaders, QVariant> &headers) {
            QVariantMap map;
            map[QStringLiteral("token_type")] = QStringLiteral("Bearer");
            map[QStringLiteral("scope")] = QStringLiteral("ReadWrite.All");
            map[QStringLiteral("expires_in")] = QString::number(tokenLifetime);
            map[QStringLiteral("ext_expires_in")] = QString::number(extTokenLifetime);
            map[QStringLiteral("expires_on")] = QString::number(time + tokenLifetime);
            map[QStringLiteral("not_before")] = QString::number(time);
            map[QStringLiteral("resource")] = resource;
            map[QStringLiteral("access_token")] = accessToken1;
            map[QStringLiteral("refresh_token")] = refreshToken1;
            map[QStringLiteral("foci")] = QStringLiteral("1");
            map[QStringLiteral("id_token")] = idToken1;
            tokenReplyData = formatJsonSorted(map);
            data = tokenReplyData;
            headers[Mock::QNetworkRequest::ContentTypeHeader] = QStringLiteral("application/json; charset=utf-8");
            
            return Mock::QNetworkReply::NoError;
        });
    Mock::QOAuth2AuthorizationCodeFlow::instance->setState(testState);

    timer.setSingleShot(true);
    timer.start(1000);

    oAuth.authenticate();
    const auto code = loop.exec();

    const auto authUrlString = Mock::authUrlString(authUrl, testClientId, testReturnUri, testEmail, resource, testState);
    const QStringList expectedEvents = {
        Mock::browserDisplayRequestString(),
        Mock::modifyParamsAuthString(testClientId, testReturnUri, testState),
        Mock::authorizeWithBrowserString(authUrlString),
        Mock::loadWebPageString(authUrlString),
        Mock::interceptRequestString(authUrlString),
        Mock::interceptRequestBlockedString(false),
        Mock::interceptRequestString(testReturnUri + "?code=" + QUrl::toPercentEncoding(refreshToken1)),
        Mock::interceptRequestBlockedString(true),
        Mock::authorizationCallbackReceivedString(refreshToken1),
        Mock::modifyParamsTokenString(testClientId, testReturnUri, refreshToken1),
        Mock::networkReplyFinishedString(tokenReplyData),
        Mock::replyDataCallbackString(tokenReplyData),
        Mock::tokenCallbackString(accessToken1, refreshToken1, idToken1, time, tokenLifetime, extTokenLifetime, resource)
    };
    for (const auto event : events) {
        qDebug() << "Got event:" << event;
    }

    QVERIFY(code == 0);
    QVERIFY(events == expectedEvents);
}

QString UtEwsOAuth::formatJsonSorted(const QVariantMap &map)
{
    QStringList keys = map.keys();
    keys.sort();
    QStringList elems;
    for (const auto key : keys) {
        QString val = map[key].toString();
        val.replace('"', QStringLiteral("\\\""));
        elems.append(QStringLiteral("\"%1\":\"%2\"").arg(key, val));
    }
    return QStringLiteral("{") + elems.join(',') + QStringLiteral("}");
}

QTEST_MAIN(UtEwsOAuth)

#include "ewsoauth_ut.moc"
