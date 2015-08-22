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

#include "davitemcreatejob.h"

#include "davitemfetchjob.h"
#include "davmanager.h"

#include <kio/davjob.h>
#include <kio/job.h>
#include <KLocalizedString>

#include <QtCore/QDebug>

DavItemCreateJob::DavItemCreateJob(const DavUtils::DavUrl &url, const DavItem &item, QObject *parent)
    : DavJobBase(parent), mUrl(url), mItem(item), mRedirectCount(0)
{
}

void DavItemCreateJob::start()
{
    QString headers = QStringLiteral("Content-Type: ");
    headers += mItem.contentType();
    headers += QLatin1String("\r\n");
    headers += QLatin1String("If-None-Match: *");

    KIO::StoredTransferJob *job = KIO::storedPut(mItem.data(), mUrl.url(), -1, KIO::HideProgressInfo | KIO::DefaultFlags);
    job->addMetaData(QStringLiteral("PropagateHttpHeader"), QStringLiteral("true"));
    job->addMetaData(QStringLiteral("customHTTPHeader"), headers);
    job->addMetaData(QStringLiteral("cookies"), QStringLiteral("none"));
    job->addMetaData(QStringLiteral("no-auth-prompt"), QStringLiteral("true"));
    job->setRedirectionHandlingEnabled(false);

    connect(job, &KIO::StoredTransferJob::result, this, &DavItemCreateJob::davJobFinished);
}

DavItem DavItemCreateJob::item() const
{
    return mItem;
}

void DavItemCreateJob::davJobFinished(KJob *job)
{
    KIO::StoredTransferJob *storedJob = qobject_cast<KIO::StoredTransferJob *>(job);
    const int responseCode = storedJob->queryMetaData(QStringLiteral("responsecode")).isEmpty() ?
                             0 :
                             storedJob->queryMetaData(QStringLiteral("responsecode")).toInt();

    if (storedJob->error()) {
        QString err;
        if (storedJob->error() != KIO::ERR_SLAVE_DEFINED) {
            err = KIO::buildErrorString(storedJob->error(), storedJob->errorText());
        } else {
            err = storedJob->errorText();
        }

        setLatestResponseCode(responseCode);
        setError(UserDefinedError + responseCode);
        setErrorText(i18n("There was a problem with the request. The item has not been created on the server.\n"
                          "%1 (%2).", err, responseCode));

        emitResult();
        return;
    }

    // The 'Location:' HTTP header is used to indicate the new URL
    const QStringList allHeaders = storedJob->queryMetaData(QStringLiteral("HTTP-Headers")).split(QLatin1Char('\n'));
    QString location;
    foreach (const QString &header, allHeaders) {
        if (header.startsWith(QLatin1String("location:"), Qt::CaseInsensitive)) {
            location = header.section(QLatin1Char(' '), 1);
        }
    }

    QUrl url;
    if (location.isEmpty()) {
        url = storedJob->url();
    } else if (location.startsWith(QLatin1Char('/'))) {
        url = storedJob->url();
        url.setPath(location, QUrl::TolerantMode);
    } else {
        url = QUrl::fromUserInput(location);
    }

    if (responseCode == 301 || responseCode == 302 || responseCode == 307 || responseCode == 308) {
        if (mRedirectCount > 4) {
            setLatestResponseCode(responseCode);
            setError(UserDefinedError + responseCode);
            emitResult();
        } else {
            QUrl itemUrl(url);
            itemUrl.setUserName(mUrl.url().userName());
            itemUrl.setPassword(mUrl.url().password());
            mUrl.setUrl(itemUrl);

            ++mRedirectCount;
            start();
        }

        return;
    }

    url.setUserInfo(QString());
    mItem.setUrl(url.toDisplayString());

    DavItemFetchJob *fetchJob = new DavItemFetchJob(mUrl, mItem);
    connect(fetchJob, &DavItemFetchJob::result, this, &DavItemCreateJob::itemRefreshed);
    fetchJob->start();
}

void DavItemCreateJob::itemRefreshed(KJob *job)
{
    if (!job->error()) {
        DavItemFetchJob *fetchJob = qobject_cast<DavItemFetchJob *>(job);
        mItem.setEtag(fetchJob->item().etag());
    }
    emitResult();
}

