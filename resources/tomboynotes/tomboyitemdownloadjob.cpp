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

#include "tomboyitemdownloadjob.h"
#include "debug.h"
#include <KMime/Message>
#include <QJsonDocument>
#include <QJsonObject>

TomboyItemDownloadJob::TomboyItemDownloadJob(const Akonadi::Item &item, KIO::AccessManager *manager, QObject *parent)
    : TomboyJobBase(manager, parent)
    , mResultItem(item)
{
}

Akonadi::Item TomboyItemDownloadJob::item() const
{
    return mResultItem;
}

void TomboyItemDownloadJob::start()
{
    // Get the specific note
    mContentURL.chop(1);
    QNetworkRequest request(QUrl(QString(mContentURL + QLatin1String("/") + mResultItem.remoteId())));
    mReply = mRequestor->get(request, QList<O0RequestParameter>());

    connect(mReply, &QNetworkReply::finished, this, &TomboyItemDownloadJob::onRequestFinished);
    qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyItemDownloadJob: Start network request";
}

void TomboyItemDownloadJob::onRequestFinished()
{
    checkReplyError();
    if (error() != TomboyJobError::NoError) {
        setErrorText(mReply->errorString());
        emitResult();
        return;
    }
    qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyItemDownloadJob: Network request finished. No error occurred";

    // Parse received data as JSON
    const QJsonDocument document = QJsonDocument::fromJson(mReply->readAll(), nullptr);

    const QJsonObject jsonNote = document.object();

    qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyItemDownloadJob: JSON note: " << jsonNote;

    mResultItem.setRemoteRevision(QString::number(jsonNote[QLatin1String("last-sync-revision")].toInt()));
    qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyItemDownloadJob: Sync revision " << mResultItem.remoteRevision();

    // Set timestamp
    const QString timeStampJson = jsonNote[QLatin1String("last-change-date")].toString();
    const QDateTime modificationTime = QDateTime::fromString(timeStampJson, Qt::ISODate);
    mResultItem.setModificationTime(modificationTime);

    // Set note title
    auto akonadiNote = KMime::Message::Ptr::create();
    akonadiNote->subject(true)->fromUnicodeString(jsonNote[QLatin1String("title")].toString(), "utf-8");

    // Set note content
    akonadiNote->contentType()->setMimeType("text/html");
    akonadiNote->contentType()->setCharset("utf-8");
    akonadiNote->contentTransferEncoding(true)->setEncoding(KMime::Headers::CEquPr);
    akonadiNote->mainBodyPart()->fromUnicodeString(jsonNote[QLatin1String("note-content")].toString());

    // Add title and content to Akonadi::Item
    akonadiNote->assemble();
    mResultItem.setPayload<KMime::Message::Ptr>(akonadiNote);

    emitResult();
}
