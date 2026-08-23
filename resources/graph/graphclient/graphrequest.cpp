/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphrequest.h"

#include "auth/graphoauth.h"
#include "graphclient.h"

#include <KLocalizedString>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

static constexpr int MaxRetries = 5;

GraphRequest::GraphRequest(GraphClient &client, QObject *parent)
    : KJob(parent)
    , mClient(client)
{
}

void GraphRequest::setMethod(Method m)
{
    mMethod = m;
}

void GraphRequest::setPath(const QString &path)
{
    mPath = path;
}

void GraphRequest::setAbsoluteUrl(const QUrl &url)
{
    mAbsoluteUrl = url;
}

void GraphRequest::setUseImmutableIds(bool use)
{
    mUseImmutableIds = use;
}

void GraphRequest::addHeader(const QByteArray &name, const QByteArray &value)
{
    mHeaders.append({name, value});
}

void GraphRequest::setBody(const QJsonObject &body)
{
    mBody = QJsonDocument(body).toJson(QJsonDocument::Compact);
    mContentType = "application/json";
}

void GraphRequest::setRawBody(const QByteArray &body, const QByteArray &contentType)
{
    mBody = body;
    mContentType = contentType;
}

void GraphRequest::start()
{
    // Requests can be scheduled before the OAuth handshake has produced a token —
    // e.g. a sync or send triggered while the interactive login is still open.
    if (!mClient.auth()) {
        setError(KJob::UserDefinedError);
        setErrorText(i18n("Not authenticated with Microsoft 365 yet"));
        QTimer::singleShot(0, this, [this] {
            emitResult();
        });
        return;
    }
    const QUrl url = mAbsoluteUrl.isValid() ? mAbsoluteUrl : QUrl(mClient.baseUrl() + mPath);
    issue(url);
}

void GraphRequest::issue(const QUrl &url)
{
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", "Bearer " + mClient.auth()->accessToken().toUtf8());
    if (!mContentType.isEmpty()) {
        req.setHeader(QNetworkRequest::ContentTypeHeader, mContentType);
    }
    // Ask for immutable ids so stored remoteIds survive moves and never flip between
    // Graph's two mutable encodings of the same message ("AAMk…" vs "AQMk…", which
    // broke delta tombstone matching). Preference values must share one header line,
    // so fold any caller-supplied Prefer entries (timezone, maxpagesize) into it.
    // Microsoft To Do has a separate id space that IdType must not switch.
    QByteArray prefer;
    if (mUseImmutableIds && !url.path().contains(QLatin1String("/me/todo"))) {
        prefer = "IdType=\"ImmutableId\"";
    }
    for (const auto &[name, value] : std::as_const(mHeaders)) {
        if (name == "Prefer") {
            prefer += (prefer.isEmpty() ? "" : ", ") + value;
        } else {
            req.setRawHeader(name, value);
        }
    }
    if (!prefer.isEmpty()) {
        req.setRawHeader("Prefer", prefer);
    }

    QNetworkAccessManager *nam = mClient.networkAccessManager();
    QNetworkReply *reply = nullptr;
    switch (mMethod) {
    case Method::Get:
        reply = nam->get(req);
        break;
    case Method::Delete:
        reply = nam->deleteResource(req);
        break;
    case Method::Post:
        reply = nam->post(req, mBody);
        break;
    case Method::Put:
        reply = nam->put(req, mBody);
        break;
    case Method::Patch:
        reply = nam->sendCustomRequest(req, "PATCH", mBody);
        break;
    }
    connect(reply, &QNetworkReply::finished, this, &GraphRequest::onReplyFinished);
}

void GraphRequest::onReplyFinished()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    mHttpStatus = http;

    // --- 429 / 503 throttling: honour Retry-After and re-issue -----------------
    if ((http == 429 || http == 503) && mRetryCount < MaxRetries) {
        const int retryAfter = reply->rawHeader("Retry-After").toInt();
        scheduleRetry(retryAfter > 0 ? retryAfter : (1 << mRetryCount), reply->url());
        ++mRetryCount;
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        // Graph error bodies carry the useful message: { "error": { "code", "message" } }.
        const QJsonObject err = QJsonDocument::fromJson(reply->readAll()).object().value(QLatin1String("error")).toObject();
        mGraphErrorCode = err.value(QLatin1String("code")).toString();
        setError(KJob::UserDefinedError);
        if (!err.isEmpty()) {
            setErrorText(i18nc("%1 is the server error message, %2 the HTTP status code, %3 the server error code",
                               "%1 (HTTP %2, %3)",
                               err.value(QLatin1String("message")).toString(),
                               http,
                               err.value(QLatin1String("code")).toString()));
        } else {
            setErrorText(reply->errorString());
        }
        emitResult();
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonObject obj = QJsonDocument::fromJson(data).object();

    // --- list vs single object -------------------------------------------------
    if (obj.contains(QLatin1String("value"))) {
        for (const auto &v : obj.value(QLatin1String("value")).toArray()) {
            mAggregated.append(v);
        }
        // Delta/paging links.
        if (obj.contains(QLatin1String("@odata.nextLink"))) {
            issue(QUrl(obj.value(QLatin1String("@odata.nextLink")).toString()));
            return; // keep aggregating
        }
        if (obj.contains(QLatin1String("@odata.deltaLink"))) {
            mDeltaLink = obj.value(QLatin1String("@odata.deltaLink")).toString();
        }
    } else {
        mResponseObject = obj;
    }

    emitResult();
}

void GraphRequest::scheduleRetry(int seconds, const QUrl &url)
{
    QTimer::singleShot(seconds * 1000, this, [this, url] {
        issue(url);
    });
}

QJsonObject GraphRequest::responseObject() const
{
    return mResponseObject;
}

QJsonArray GraphRequest::aggregatedValue() const
{
    return mAggregated;
}

QString GraphRequest::deltaLink() const
{
    return mDeltaLink;
}

int GraphRequest::httpStatus() const
{
    return mHttpStatus;
}

QString GraphRequest::graphErrorCode() const
{
    return mGraphErrorCode;
}

#include "moc_graphrequest.cpp"
