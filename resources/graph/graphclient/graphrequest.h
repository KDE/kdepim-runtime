/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Base KJob for a single Graph REST call. Centralises the three things every Graph
    call must handle and that EWS did *not* need:
      1. Bearer-token injection (Authorization: Bearer ...).
      2. 429 Too Many Requests  -> honour `Retry-After` and re-issue.
      3. Paging: transparently follow `@odata.nextLink` and aggregate results.
         (`@odata.deltaLink` is exposed to callers as the persisted sync state.)
*/

#pragma once

#include <KJob>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>

class GraphClient;

class GraphRequest : public KJob
{
    Q_OBJECT
public:
    enum class Method : uint8_t {
        Get,
        Post,
        Put,
        Patch,
        Delete
    };
    Q_ENUM(Method)

    GraphRequest(GraphClient &client, QObject *parent = nullptr);

    void setMethod(Method m);
    void setPath(const QString &path); // e.g. "/me/mailFolders/delta" (relative to baseUrl)
    void setAbsoluteUrl(const QUrl &url); // e.g. a stored nextLink/deltaLink (overrides path)
    void setBody(const QJsonObject &body);
    void setRawBody(const QByteArray &body, const QByteArray &contentType);
    // Requests ask for immutable item ids by default (IdType="ImmutableId"). Opt out
    // for listings whose returned ids serve as *collection* remoteIds: unlike
    // mailFolders, /me/calendars ids do change under IdType, and the stored
    // collection remoteIds are not migrated.
    void setUseImmutableIds(bool use);
    void addHeader(const QByteArray &name, const QByteArray &value); // e.g. Prefer

    void start() override;

    // Results (available in result slot):
    [[nodiscard]] QJsonObject responseObject() const; // single-object responses
    [[nodiscard]] QJsonArray aggregatedValue() const; // paged list responses ("value" concatenated)
    [[nodiscard]] QString deltaLink() const; // @odata.deltaLink from the final page (if any)
    [[nodiscard]] int httpStatus() const; // HTTP status of the (last) reply
    [[nodiscard]] QString graphErrorCode() const; // "error.code" from a Graph error body

private:
    void onReplyFinished();
    void issue(const QUrl &url);
    void scheduleRetry(int seconds, const QUrl &url); // 429 backoff

    GraphClient &mClient;
    Method mMethod = Method::Get;
    QString mPath;
    QUrl mAbsoluteUrl;
    QByteArray mBody;
    QByteArray mContentType;
    QList<QPair<QByteArray, QByteArray>> mHeaders;
    bool mUseImmutableIds = true;

    QJsonObject mResponseObject;
    QJsonArray mAggregated;
    QString mDeltaLink;
    QString mGraphErrorCode;
    int mRetryCount = 0;
    int mHttpStatus = 0;
};
