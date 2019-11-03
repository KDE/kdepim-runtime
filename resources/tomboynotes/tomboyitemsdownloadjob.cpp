/*
    Copyright (c) 2016 Stefan Stäglich <sstaeglich@kdemail.net>

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

#include "tomboyitemsdownloadjob.h"
#include "debug.h"
#include <Akonadi/Notes/NoteUtils>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

TomboyItemsDownloadJob::TomboyItemsDownloadJob(Akonadi::Collection::Id id, KIO::AccessManager *manager, QObject *parent)
    : TomboyJobBase(manager, parent)
    , mCollectionId(id)
{
}

Akonadi::Item::List TomboyItemsDownloadJob::items() const
{
    return mResultItems;
}

void TomboyItemsDownloadJob::start()
{
    // Get all notes
    QNetworkRequest request = QNetworkRequest(QUrl(mContentURL));
    mReply = mRequestor->get(request, QList<O0RequestParameter>());

    connect(mReply, &QNetworkReply::finished, this, &TomboyItemsDownloadJob::onRequestFinished);
    qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyItemsDownloadJob: Start network request";
}

void TomboyItemsDownloadJob::onRequestFinished()
{
    qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyItemsDownloadJob: Network request finished";
    checkReplyError();
    if (error() != TomboyJobError::NoError) {
        setErrorText(mReply->errorString());
        emitResult();
        return;
    }

    // Parse received data as JSON
    const QJsonDocument document = QJsonDocument::fromJson(mReply->readAll(), nullptr);

    const QJsonObject jo = document.object();
    const QJsonArray notes = jo[QLatin1String("notes")].toArray();

    for (const auto &note : notes) {
        Akonadi::Item item(Akonadi::NoteUtils::noteMimeType());
        item.setRemoteId(note.toObject()[QLatin1String("guid")].toString());
        mResultItems << item;
        qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyItemsDownloadJob: Retrieving note with id" << item.remoteId();
    }

    setError(TomboyJobError::NoError);
    emitResult();
}
