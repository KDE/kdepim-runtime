/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphfetchpimitemsjob.h"

#include "calendar/grapheventhandler.h"
#include "contact/graphcontacthandler.h"
#include "graphclient/graphrequest.h"
#include "todo/graphtodohandler.h"

#include <KCalendarCore/Event>
#include <KContacts/Picture>
#include <QJsonArray>
#include <QJsonObject>

using namespace Akonadi;

// Delta change tracking rejects $top; page size is expressed via this header instead.
static const QByteArray kMaxPageSize = QByteArrayLiteral("odata.maxpagesize=50");

GraphFetchPimItemsJob::GraphFetchPimItemsJob(GraphClient &client, const Collection &collection, Type type, const QString &deltaLink, QObject *parent)
    : KJob(parent)
    , mClient(client)
    , mCollection(collection)
    , mType(type)
    , mDeltaLink(deltaLink)
{
}

void GraphFetchPimItemsJob::start()
{
    if (!mDeltaLink.isEmpty()) {
        startDelta(mDeltaLink, true); // resume from stored deltaLink
        return;
    }
    switch (mType) {
    case Type::Events:
        // The events delta returns only a minimal default field set; $select pulls the
        // fields we map. The returned deltaLink preserves this selection.
        startDelta(QStringLiteral("/me/calendars/%1/events/delta"
                                  "?$select=id,iCalUId,subject,body,bodyPreview,isAllDay,start,end,location,"
                                  "organizer,attendees,categories,showAs,sensitivity,recurrence")
                       .arg(mCollection.remoteId()),
                   false);
        break;
    case Type::Todos:
        startDelta(QStringLiteral("/me/todo/lists/%1/tasks/delta").arg(mCollection.remoteId()), false);
        break;
    case Type::Contacts:
        resolveContactsFolderThenStart();
        break;
    }
}

void GraphFetchPimItemsJob::resolveContactsFolderThenStart()
{
    // The default contacts folder is not listed by /me/contactFolders; get its id from
    // any contact's parentFolderId, then start the folder-scoped delta.
    auto pre = new GraphRequest(mClient, this);
    pre->setPath(QStringLiteral("/me/contacts?$top=1&$select=id,parentFolderId"));
    connect(pre, &KJob::result, this, [this, pre](KJob *job) {
        if (job->error()) {
            setError(job->error());
            setErrorText(job->errorText());
            emitResult();
            return;
        }
        const auto values = pre->aggregatedValue();
        if (values.isEmpty()) {
            emitResult(); // empty contacts folder: nothing to sync, no deltaLink yet
            return;
        }
        const QString folderId = values.first().toObject().value(QLatin1String("parentFolderId")).toString();
        if (folderId.isEmpty()) {
            emitResult();
            return;
        }
        startDelta(QStringLiteral("/me/contactFolders/%1/contacts/delta").arg(folderId), false);
    });
    pre->start();
}

void GraphFetchPimItemsJob::startDelta(const QString &url, bool isAbsolute)
{
    auto req = new GraphRequest(mClient, this);
    if (isAbsolute) {
        req->setAbsoluteUrl(QUrl(url));
    } else {
        req->setPath(url);
    }
    req->addHeader("Prefer", kMaxPageSize);
    connect(req, &KJob::result, this, &GraphFetchPimItemsJob::onDeltaFinished);
    req->start();
}

void GraphFetchPimItemsJob::onDeltaFinished(KJob *job)
{
    auto *req = qobject_cast<GraphRequest *>(job);
    if (job->error()) {
        if ((req->httpStatus() == 410 || req->graphErrorCode() == QLatin1String("ErrorInvalidSyncStateData")) && !mDeltaLink.isEmpty()) {
            // The stored delta token expired (HTTP 410 Gone) or the server no longer
            // accepts its state (HTTP 400 ErrorInvalidSyncStateData) — drop it and
            // rerun the delta from scratch. Delivery is incremental, so this only re-lists.
            mDeltaLink.clear();
            mChanged.clear();
            mRemoved.clear();
            start();
            return;
        }
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }
    mDeltaLink = req->deltaLink();

    for (const auto &v : req->aggregatedValue()) {
        const QJsonObject obj = v.toObject();
        const QString id = obj.value(QLatin1String("id")).toString();
        if (id.isEmpty()) {
            continue;
        }
        if (obj.contains(QLatin1String("@removed"))) {
            Item item;
            item.setRemoteId(id);
            mRemoved.append(item);
            continue;
        }
        switch (mType) {
        case Type::Events: {
            auto event = GraphEventHandler::toEvent(obj);
            if (!event) {
                continue;
            }
            Item item(GraphEventHandler::mimeType());
            item.setRemoteId(id);
            item.setParentCollection(mCollection);
            item.setPayload<KCalendarCore::Incidence::Ptr>(event);
            mChanged.append(item);
            break;
        }
        case Type::Todos: {
            auto todo = GraphTodoHandler::toTodo(obj);
            if (!todo) {
                continue;
            }
            Item item(GraphTodoHandler::mimeType());
            item.setRemoteId(id);
            item.setParentCollection(mCollection);
            item.setPayload<KCalendarCore::Incidence::Ptr>(todo);
            mChanged.append(item);
            break;
        }
        case Type::Contacts: {
            Item item(GraphContactHandler::mimeType());
            item.setRemoteId(id);
            item.setParentCollection(mCollection);
            item.setPayload<KContacts::Addressee>(GraphContactHandler::toAddressee(obj));
            mChanged.append(item);
            break;
        }
        }
    }
    if (mType == Type::Contacts && !mChanged.isEmpty()) {
        fetchPhotos(); // photos are a separate endpoint, not part of the contact JSON
        return;
    }
    emitResult();
}

void GraphFetchPimItemsJob::fetchPhotos()
{
    // GET /me/contacts/{id}/photo/$value for each changed contact, bundled via /$batch
    // (20 sub-requests per call, like the mail payload path). 404 = contact has no photo.
    constexpr int batchSize = 20;
    const int first = mPhotoIndex;
    const int last = qMin(mPhotoIndex + batchSize, int(mChanged.size()));

    QJsonArray requests;
    for (int i = first; i < last; ++i) {
        QJsonObject sub;
        sub.insert(QStringLiteral("id"), QString::number(i));
        sub.insert(QStringLiteral("method"), QStringLiteral("GET"));
        sub.insert(QStringLiteral("url"), QStringLiteral("/me/contacts/%1/photo/$value").arg(mChanged.at(i).remoteId()));
        requests.append(sub);
    }
    QJsonObject body;
    body.insert(QStringLiteral("requests"), requests);

    auto req = new GraphRequest(mClient, this);
    req->setMethod(GraphRequest::Method::Post);
    req->setPath(QStringLiteral("/$batch"));
    req->setBody(body);
    connect(req, &KJob::result, this, [this, req, last](KJob *job) {
        if (job->error()) {
            emitResult(); // photos are decoration: deliver the contacts without them
            return;
        }
        const QJsonArray responses = req->responseObject().value(QLatin1String("responses")).toArray();
        for (const auto &r : responses) {
            const QJsonObject resp = r.toObject();
            const int idx = resp.value(QLatin1String("id")).toString().toInt();
            const int status = resp.value(QLatin1String("status")).toInt();
            if (idx < 0 || idx >= mChanged.size() || status != 200) {
                continue;
            }
            // /$value in a batch comes back as a base64-encoded string body.
            const QByteArray data = QByteArray::fromBase64(resp.value(QLatin1String("body")).toString().toLatin1());
            if (data.isEmpty()) {
                continue;
            }
            QString type = resp.value(QLatin1String("headers")).toObject().value(QLatin1String("Content-Type")).toString();
            type = type.startsWith(QLatin1String("image/")) ? type.mid(6) : QStringLiteral("jpeg");
            auto addressee = mChanged.at(idx).payload<KContacts::Addressee>();
            KContacts::Picture photo;
            photo.setRawData(data, type);
            addressee.setPhoto(photo);
            mChanged[idx].setPayload(addressee);
        }
        mPhotoIndex = last;
        if (mPhotoIndex < mChanged.size()) {
            fetchPhotos();
        } else {
            emitResult();
        }
    });
    req->start();
}

Collection GraphFetchPimItemsJob::collection() const
{
    return mCollection;
}

Item::List GraphFetchPimItemsJob::changedItems() const
{
    return mChanged;
}

Item::List GraphFetchPimItemsJob::removedItems() const
{
    return mRemoved;
}
QString GraphFetchPimItemsJob::deltaLink() const
{
    return mDeltaLink;
}

#include "moc_graphfetchpimitemsjob.cpp"
