/*
 *    Copyright (C) 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "listjob.h"
#include "resource_debug.h"
#include "tokenjobs.h"
#include "graph.h"

#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QUrlQuery>

#include <KLocalizedString>

ListJob::ListJob(const QString &identifier, const Akonadi::Collection &col, QObject *parent)
    : KCompositeJob(parent)
    , mIdentifier(identifier)
    , mCollection(col)
{
}

ListJob::~ListJob()
{
}

Akonadi::Collection ListJob::collection() const
{
    return mCollection;
}

void ListJob::emitError(const QString &errorText)
{
    setError(KJob::UserDefinedError);
    setErrorText(errorText);
    emitResult();
}

void ListJob::setRequest(const QString &endpoint, const QStringList &fields, const QMap<QString, QString> &queries)
{
    mEndpoint = endpoint;
    mFields = fields;
    mQueries = queries;
}

void ListJob::start()
{
    auto job = new GetTokenJob(mIdentifier, parent());
    connect(job, &GetTokenJob::result, this, &ListJob::tokenJobResult);
    job->start();
}

void ListJob::tokenJobResult(KJob *job)
{
    auto tokenJob = qobject_cast<GetTokenJob *>(job);
    if (tokenJob->error()) {
        emitError(tokenJob->errorText());
        return;
    }

    sendRequest(Graph::url(mEndpoint, tokenJob->token(), mFields, mQueries));
}

void ListJob::sendRequest(const QUrl &url)
{
    auto job = Graph::job(url);
    connect(job, &KJob::result, this, &ListJob::onGraphResponseReceived);
    job->start();
}

void ListJob::onGraphResponseReceived(KJob *job)
{
    if (job->error()) {
        emitError(job->errorText());
        return;
    }

    auto tjob = qobject_cast<KIO::StoredTransferJob *>(job);
    Q_ASSERT(tjob);

    QJsonParseError error;
    auto json = QJsonDocument::fromJson(tjob->data(), &error);
    if (error.error) {
        qCWarning(FBRESOURCE_LOG) << "JSON parsing error" << error.error << ", offset" << error.offset;
        emitError(i18n("Invalid response from server: JSON parsing error"));
        return;
    }

    const auto obj = json.object();

    if (obj.contains(QLatin1String("error"))) {
        const auto err = obj.value(QLatin1String("error")).toObject();
        const auto code = err.value(QLatin1String("code")).toInt();
        if (code == 190) {
            // expired token
            auto tokenJob = new LoginJob(mIdentifier, parent());
            const QUrl url = tjob->url();
            connect(tokenJob, &LoginJob::result,
                    this, [this, tokenJob, url]() {
                if (tokenJob->error()) {
                    emitError(tokenJob->errorText());
                    return;
                }

                QUrl url_ = url;
                QUrlQuery query(url);

                query.removeQueryItem(QStringLiteral("access_token"));
                query.addQueryItem(QStringLiteral("access_token"), tokenJob->token());
                url_.setQuery(query);
                sendRequest(url_);
            });
            tokenJob->start();
            return;
        }

        emitError(err.value(QLatin1String("message")).toString());
        return;
    }

    const auto data = obj.value(QLatin1String("data")).toArray();
    Akonadi::Item::List items;
    items.reserve(data.size());
    for (auto it = data.constBegin(), end = data.constEnd(); it != end; ++it) {
        const auto item = handleResponse(it->toObject());
        if (item.hasPayload()) {
            items.push_back(item);
        }
    }

    Q_EMIT itemsAvailable(this, items, {});

    const auto paging = obj.value(QLatin1String("paging")).toObject();
    const auto next = paging.constFind(QLatin1String("next"));
    if (next != paging.constEnd()) {
        sendRequest(QUrl(next->toString()));
    } else {
        emitResult();
    }
}
