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

#include "tomboyitemuploadjob.h"
#include "debug.h"
#include <klocalizedstring.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

TomboyItemUploadJob::TomboyItemUploadJob(const Akonadi::Item &item, JobType jobType, KIO::AccessManager *manager, QObject *parent)
    : TomboyJobBase(manager, parent)
    , mSourceItem(item)
    , mJobType(jobType)
{
    mSourceItem = Akonadi::Item(item);
    if (item.hasPayload<KMime::Message::Ptr>()) {
        mNoteContent = item.payload<KMime::Message::Ptr>();
    }

    mRemoteRevision = item.parentCollection().remoteRevision().toInt();

    // Create random remote id if adding new item
    if (jobType == JobType::AddItem) {
        mSourceItem.setRemoteId(QUuid::createUuid().toString());
    }
}

Akonadi::Item TomboyItemUploadJob::item() const
{
    return mSourceItem;
}

JobType TomboyItemUploadJob::jobType() const
{
    return mJobType;
}

void TomboyItemUploadJob::start()
{
    // Convert note to JSON
    QJsonObject jsonNote;
    jsonNote[QLatin1String("guid")] = mSourceItem.remoteId();
    switch (mJobType) {
    case JobType::DeleteItem:
        jsonNote[QLatin1String("command")] = QStringLiteral("delete");
        break;
    case JobType::AddItem:
        jsonNote[QLatin1String("create-date")] = getCurrentISOTime();
        // Missing break is intended
        Q_FALLTHROUGH();
    case JobType::ModifyItem:
        jsonNote[QLatin1String("title")] = mNoteContent->headerByType("subject")->asUnicodeString();
        jsonNote[QLatin1String("note-content")] = mNoteContent->mainBodyPart()->decodedText();
        jsonNote[QLatin1String("note-content-version")] = QStringLiteral("1");
        jsonNote[QLatin1String("last-change-date")] = getCurrentISOTime();
    }

    // Create the full JSON object
    QJsonArray noteChanges;
    noteChanges.append(jsonNote);
    QJsonObject postJson;
    postJson[QLatin1String("note-changes")] = noteChanges;
    postJson[QLatin1String("latest-sync-revision")] = QString::number(++mRemoteRevision);
    QJsonDocument postData;
    postData.setObject(postJson);

    // Network request
    QNetworkRequest request = QNetworkRequest(QUrl(mContentURL));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; boundary=7d44e178b0439"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, postData.toJson().length());
    mReply = mRequestor->put(request, QList<O0RequestParameter>(), postData.toJson());
    connect(mReply, &QNetworkReply::finished, this, &TomboyItemUploadJob::onRequestFinished);
    qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyItemUploadJob: Start network request";
}

void TomboyItemUploadJob::onRequestFinished()
{
    checkReplyError();
    if (error() != TomboyJobError::NoError) {
        setErrorText(mReply->errorString());
        emitResult();
        return;
    }
    qCDebug(TOMBOYNOTESRESOURCE_LOG) << "TomboyItemUploadJob: Network request finished. No error occured";

    // Parse received data as JSON
    const QJsonDocument document = QJsonDocument::fromJson(mReply->readAll(), nullptr);

    const QJsonObject jo = document.object();
    const QJsonArray notes = jo[QLatin1String("notes")].toArray();

    // Check if server status is as expected
    bool found = false;
    for (const auto &note : notes) {
        found = (note.toObject()[QLatin1String("guid")].toString() == mSourceItem.remoteId());
        if (found) {
            break;
        }
    }
    if (mJobType == JobType::DeleteItem && found) {
        setError(TomboyJobError::PermanentError);
        setErrorText(i18n("Sync error. Server status not as expected."));
        emitResult();
        return;
    }
    if (mJobType != JobType::DeleteItem && !found) {
        setError(TomboyJobError::PermanentError);
        setErrorText(i18n("Sync error. Server status not as expected."));
        emitResult();
        return;
    }

    setError(TomboyJobError::NoError);
    emitResult();
}

QString TomboyItemUploadJob::getCurrentISOTime() const
{
    QDateTime local = QDateTime::currentDateTime();
    QDateTime utc = local.toUTC();
    utc.setTimeSpec(Qt::LocalTime);
    int utcOffset = utc.secsTo(local);
    local.setUtcOffset(utcOffset);

    return local.toString(Qt::ISODate);
}
