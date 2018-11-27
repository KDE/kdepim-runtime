/*
    Copyright (c) 2016 Stefan Stäglich  <sstaeglich@kdemail.net>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "tomboycollectionsdownloadjob.h"
#include "debug.h"
#include <Akonadi/Notes/NoteUtils>
#include <AkonadiCore/CachePolicy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

TomboyCollectionsDownloadJob::TomboyCollectionsDownloadJob(const QString &collectionName, KIO::AccessManager *manager, int refreshInterval, QObject *parent)
    : TomboyJobBase(manager, parent)
    , mCollectionName(collectionName)
{
    mRefreshInterval = refreshInterval;
}

Akonadi::Collection::List TomboyCollectionsDownloadJob::collections() const
{
    return mResultCollections;
}

void TomboyCollectionsDownloadJob::start()
{
    // Get user information
    QNetworkRequest request = QNetworkRequest(QUrl(mContentURL));
    mReply = mRequestor->get(request, QList<O0RequestParameter>());

    connect(mReply, &QNetworkReply::finished, this, &TomboyCollectionsDownloadJob::onRequestFinished);
    qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyCollectionsDownloadJob: Start network request";
}

void TomboyCollectionsDownloadJob::onRequestFinished()
{
    qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyCollectionsDownloadJob: Network request finished";
    checkReplyError();
    if (error() != TomboyJobError::NoError) {
        setErrorText(mReply->errorString());
        emitResult();
        return;
    }

    // Parse received data as JSON
    const QJsonDocument document = QJsonDocument::fromJson(mReply->readAll(), nullptr);

    const QJsonObject jo = document.object();
    qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyCollectionsDownloadJob: " << jo;
    const QJsonValue collectionRevision = jo[QLatin1String("latest-sync-revision")];
    qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyCollectionsDownloadJob: " << collectionRevision;

    Akonadi::Collection c;
    c.setParentCollection(Akonadi::Collection::root());
    c.setRemoteId(mContentURL);
    c.setName(mCollectionName);
    c.setRemoteRevision(QString::number(collectionRevision.toInt()));
    qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyCollectionsDownloadJob: Sync revision " << collectionRevision.toString();

    Akonadi::CachePolicy cachePolicy;
    cachePolicy.setInheritFromParent(false);
    cachePolicy.setSyncOnDemand(false);
    cachePolicy.setCacheTimeout(-1);
    cachePolicy.setIntervalCheckTime(mRefreshInterval);
    cachePolicy.setLocalParts(QStringList() << QStringLiteral("ALL"));
    c.setCachePolicy(cachePolicy);

    c.setContentMimeTypes({ Akonadi::NoteUtils::noteMimeType() });

    mResultCollections << c;

    emitResult();
}
