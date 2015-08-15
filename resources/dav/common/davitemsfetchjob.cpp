/*
    Copyright (c) 2010 Grégory Oestreicher <greg@kamago.net>
    Based on DavItemsListJob which is copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "davitemsfetchjob.h"
#include "davmanager.h"
#include "davmultigetprotocol.h"
#include <AkonadiCore/VectorHelper>
#include <kio/davjob.h>
#include <kio/job.h>
#include <KLocalizedString>

DavItemsFetchJob::DavItemsFetchJob(const DavUtils::DavUrl &collectionUrl, const QStringList &urls, QObject *parent)
    : KJob(parent), mCollectionUrl(collectionUrl), mUrls(urls)
{
}

void DavItemsFetchJob::start()
{
    const DavMultigetProtocol *protocol =
        dynamic_cast<const DavMultigetProtocol *>(DavManager::self()->davProtocol(mCollectionUrl.protocol()));
    if (!protocol) {
        setError(UserDefinedError);
        setErrorText(i18n("Protocol for the collection does not support MULTIGET"));
        emitResult();
        return;
    }

    const QDomDocument report = protocol->itemsReportQuery(mUrls)->buildQuery();
    KIO::DavJob *job = DavManager::self()->createReportJob(mCollectionUrl.url(), report, QStringLiteral("0"));
    job->addMetaData(QStringLiteral("PropagateHttpHeader"), QStringLiteral("true"));
    connect(job, &KIO::DavJob::result, this, &DavItemsFetchJob::davJobFinished);
}

DavItem::List DavItemsFetchJob::items() const
{
    return Akonadi::valuesToVector(mItems);
}

DavItem DavItemsFetchJob::item(const QString &url) const
{
    return mItems.value(url);
}

void DavItemsFetchJob::davJobFinished(KJob *job)
{
    KIO::DavJob *davJob = qobject_cast<KIO::DavJob *>(job);
    const int responseCode = davJob->queryMetaData(QStringLiteral("responsecode")).isEmpty() ?
                             0 :
                             davJob->queryMetaData(QStringLiteral("responsecode")).toInt();

    // KIO::DavJob does not set error() even if the HTTP status code is a 4xx or a 5xx
    if (davJob->error() || (responseCode >= 400 && responseCode < 600)) {
        QString err;
        if (davJob->error() && davJob->error() != KIO::ERR_SLAVE_DEFINED) {
            err = KIO::buildErrorString(davJob->error(), davJob->errorText());
        } else {
            err = davJob->errorText();
        }

        setError(UserDefinedError + responseCode);
        setErrorText(i18n("There was a problem with the request.\n"
                          "%1 (%2).", err, responseCode));

        emitResult();
        return;
    }

    const DavMultigetProtocol *protocol =
        static_cast<const DavMultigetProtocol *>(DavManager::self()->davProtocol(mCollectionUrl.protocol()));

    const QDomDocument document = davJob->response();
    const QDomElement documentElement = document.documentElement();

    QDomElement responseElement = DavUtils::firstChildElementNS(documentElement, QStringLiteral("DAV:"), QStringLiteral("response"));

    while (!responseElement.isNull()) {
        QDomElement propstatElement = DavUtils::firstChildElementNS(responseElement, QStringLiteral("DAV:"), QStringLiteral("propstat"));

        if (propstatElement.isNull()) {
            responseElement = DavUtils::nextSiblingElementNS(responseElement, QStringLiteral("DAV:"), QStringLiteral("response"));
            continue;
        }

        // Check for errors
        const QDomElement statusElement = DavUtils::firstChildElementNS(propstatElement, QStringLiteral("DAV:"), QStringLiteral("status"));
        if (!statusElement.text().contains(QLatin1String("200"))) {
            responseElement = DavUtils::nextSiblingElementNS(responseElement, QStringLiteral("DAV:"), QStringLiteral("response"));
            continue;
        }

        const QDomElement propElement = DavUtils::firstChildElementNS(propstatElement, QStringLiteral("DAV:"), QStringLiteral("prop"));

        DavItem item;

        // extract path
        const QDomElement hrefElement = DavUtils::firstChildElementNS(responseElement, QStringLiteral("DAV:"), QStringLiteral("href"));
        const QString href = hrefElement.text();

        QUrl url = davJob->url();
        url.setUserInfo(QString());
        if (href.startsWith(QLatin1Char('/'))) {
            // href is only a path, use request url to complete
            url.setPath(href.toLatin1(), QUrl::TolerantMode);
        } else {
            // href is a complete url
            url = QUrl::fromUserInput(href);
        }

        item.setUrl(url.toDisplayString());

        // extract etag
        const QDomElement getetagElement = DavUtils::firstChildElementNS(propElement, QStringLiteral("DAV:"), QStringLiteral("getetag"));
        item.setEtag(getetagElement.text());

        // extract content
        const QDomElement dataElement = DavUtils::firstChildElementNS(propElement,
                                        protocol->responseNamespace(),
                                        protocol->dataTagName());

        if (dataElement.isNull()) {
            responseElement = DavUtils::nextSiblingElementNS(responseElement, QStringLiteral("DAV:"), QStringLiteral("response"));
            continue;
        }

        const QByteArray data = dataElement.firstChild().toText().data().toUtf8();
        if (data.isEmpty()) {
            responseElement = DavUtils::nextSiblingElementNS(responseElement, QStringLiteral("DAV:"), QStringLiteral("response"));
            continue;
        }

        item.setData(data);

        mItems.insert(item.url(), item);
        responseElement = DavUtils::nextSiblingElementNS(responseElement, QStringLiteral("DAV:"), QStringLiteral("response"));
    }

    emitResult();
}
