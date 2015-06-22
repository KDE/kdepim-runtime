/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "davitemslistjob.h"

#include "davmanager.h"
#include "davprotocolbase.h"
#include "davutils.h"

#include <kio/davjob.h>
#include <kio/job.h>
#include <KLocalizedString>

#include <QtCore/QBuffer>

DavItemsListJob::DavItemsListJob(const DavUtils::DavUrl &url, QObject *parent)
    : KJob(parent), mUrl(url), mSubJobCount(0)
{
}

void DavItemsListJob::setContentMimeTypes(const QStringList &types)
{
    mMimeTypes = types;
}

void DavItemsListJob::start()
{
    const DavProtocolBase *protocol = DavManager::self()->davProtocol(mUrl.protocol());
    Q_ASSERT(protocol);
    QListIterator<QDomDocument> it(protocol->itemsQueries());
    int queryIndex = 0;

    while (it.hasNext()) {
        const QDomDocument props = it.next();
        const QString mimeType = protocol->mimeTypeForQuery(queryIndex);

        if (mMimeTypes.isEmpty() || mMimeTypes.contains(mimeType)) {
            ++mSubJobCount;
            if (protocol->useReport()) {
                KIO::DavJob *job = DavManager::self()->createReportJob(mUrl.url(), props);
                job->addMetaData(QStringLiteral("PropagateHttpHeader"), QStringLiteral("true"));
                job->setProperty("davType", QStringLiteral("report"));
                job->setProperty("itemsMimeType", mimeType);
                connect(job, &KIO::DavJob::result, this, &DavItemsListJob::davJobFinished);
            } else {
                KIO::DavJob *job = DavManager::self()->createPropFindJob(mUrl.url(), props);
                job->addMetaData(QStringLiteral("PropagateHttpHeader"), QStringLiteral("true"));
                job->setProperty("davType", QStringLiteral("propFind"));
                job->setProperty("itemsMimeType", mimeType);
                connect(job, &KIO::DavJob::result, this, &DavItemsListJob::davJobFinished);
            }
        }

        ++queryIndex;
    }
}

DavItem::List DavItemsListJob::items() const
{
    return mItems;
}

void DavItemsListJob::davJobFinished(KJob *job)
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
    } else {
        /*
         * Extract data from a document like the following:
         *
         * <multistatus xmlns="DAV:">
         *   <response xmlns="DAV:">
         *     <href xmlns="DAV:">/caldav.php/test1.user/home/KOrganizer-166749289.780.ics</href>
         *     <propstat xmlns="DAV:">
         *       <prop xmlns="DAV:">
         *         <getetag xmlns="DAV:">"b4bbea0278f4f63854c4167a7656024a"</getetag>
         *       </prop>
         *       <status xmlns="DAV:">HTTP/1.1 200 OK</status>
         *     </propstat>
         *   </response>
         *   <response xmlns="DAV:">
         *     <href xmlns="DAV:">/caldav.php/test1.user/home/KOrganizer-399416366.464.ics</href>
         *     <propstat xmlns="DAV:">
         *       <prop xmlns="DAV:">
         *         <getetag xmlns="DAV:">"52eb129018398a7da4f435b2bc4c6cd5"</getetag>
         *       </prop>
         *       <status xmlns="DAV:">HTTP/1.1 200 OK</status>
         *     </propstat>
         *   </response>
         * </multistatus>
         */

        const QString itemsMimeType = job->property("itemsMimeType").toString();
        const QDomDocument document = davJob->response();
        const QDomElement documentElement = document.documentElement();

        QDomElement responseElement = DavUtils::firstChildElementNS(documentElement, QStringLiteral("DAV:"), QStringLiteral("response"));
        while (!responseElement.isNull()) {

            QDomElement propstatElement;

            // check for the valid propstat, without giving up on first error
            {
                const QDomNodeList propstats = responseElement.elementsByTagNameNS(QStringLiteral("DAV:"), QStringLiteral("propstat"));
                for (int i = 0; i < propstats.length(); ++i) {
                    const QDomElement propstatCandidate = propstats.item(i).toElement();
                    const QDomElement statusElement = DavUtils::firstChildElementNS(propstatCandidate, QStringLiteral("DAV:"), QStringLiteral("status"));
                    if (statusElement.text().contains(QStringLiteral("200"))) {
                        propstatElement = propstatCandidate;
                    }
                }
            }

            if (propstatElement.isNull()) {
                responseElement = DavUtils::nextSiblingElementNS(responseElement, QStringLiteral("DAV:"), QStringLiteral("response"));
                continue;
            }

            const QDomElement propElement = DavUtils::firstChildElementNS(propstatElement, QStringLiteral("DAV:"), QStringLiteral("prop"));

            // check whether it is a dav collection ...
            const QDomElement resourcetypeElement = DavUtils::firstChildElementNS(propElement, QStringLiteral("DAV:"), QStringLiteral("resourcetype"));
            if (!responseElement.isNull()) {
                const QDomElement collectionElement = DavUtils::firstChildElementNS(resourcetypeElement, QStringLiteral("DAV:"), QStringLiteral("collection"));
                if (!collectionElement.isNull()) {
                    responseElement = DavUtils::nextSiblingElementNS(responseElement, QStringLiteral("DAV:"), QStringLiteral("response"));
                    continue;
                }
            }

            // ... if not it is an item
            DavItem item;
            item.setContentType(itemsMimeType);

            // extract path
            const QDomElement hrefElement = DavUtils::firstChildElementNS(responseElement, QStringLiteral("DAV:"), QStringLiteral("href"));
            const QString href = hrefElement.text();

            KUrl url = davJob->url();
            url.setUser(QString());
            if (href.startsWith(QLatin1Char('/'))) {
                // href is only a path, use request url to complete
                url.setEncodedPath(href.toLatin1());
            } else {
                // href is a complete url
                KUrl tmpUrl(href);
                url = tmpUrl;
            }

            QString itemUrl = url.prettyUrl();
            if (mSeenUrls.contains(itemUrl)) {
                responseElement = DavUtils::nextSiblingElementNS(responseElement, QStringLiteral("DAV:"), QStringLiteral("response"));
                continue;
            }

            mSeenUrls << itemUrl;
            item.setUrl(itemUrl);

            // extract etag
            const QDomElement getetagElement = DavUtils::firstChildElementNS(propElement, QStringLiteral("DAV:"), QStringLiteral("getetag"));

            item.setEtag(getetagElement.text());

            mItems << item;

            responseElement = DavUtils::nextSiblingElementNS(responseElement, QStringLiteral("DAV:"), QStringLiteral("response"));
        }
    }

    if (--mSubJobCount == 0) {
        emitResult();
    }
}

