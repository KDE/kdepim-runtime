/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "compactchangehelper.h"
#include "mixedmaildirresource_debug.h"
#include "filestore/entitycompactchangeattribute.h"

#include <AkonadiCore/collection.h>
#include <AkonadiCore/collectionmodifyjob.h>
#include <AkonadiCore/item.h>
#include <AkonadiCore/itemfetchjob.h>
#include <AkonadiCore/itemmodifyjob.h>
#include <AkonadiCore/session.h>

#include <QMap>
#include <QQueue>

using namespace Akonadi;

typedef QMap<QString, Item> OldIdItemMap;
typedef QMap<qint64, OldIdItemMap> RevisionChangeMap;
typedef QMap<Collection::Id, RevisionChangeMap> CollectionRevisionMap;

struct UpdateBatch {
    QQueue<Item> items;
    Collection collection;
};

class CompactChangeHelper::Private
{
    CompactChangeHelper *const q;

public:
    explicit Private(CompactChangeHelper *parent) : q(parent)
    {
    }

public:
    Session *mSession = nullptr;
    CollectionRevisionMap mChangesByCollection;
    QQueue<UpdateBatch> mPendingUpdates;
    UpdateBatch mCurrentUpdate;

public: // slots
    void processNextBatch();
    void processNextItem();
    void itemFetchResult(KJob *job);
};

void CompactChangeHelper::Private::processNextBatch()
{
    //qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "pendingUpdates.count=" << mPendingUpdates.count();
    if (mPendingUpdates.isEmpty()) {
        return;
    }

    mCurrentUpdate = mPendingUpdates.dequeue();

    processNextItem();
}

void CompactChangeHelper::Private::processNextItem()
{
    //qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "mCurrentUpdate.items.count=" << mCurrentUpdate.items.count();
    if (mCurrentUpdate.items.isEmpty()) {
        CollectionModifyJob *job = new CollectionModifyJob(mCurrentUpdate.collection, mSession);
        QObject::connect(job, &CollectionModifyJob::result, q, [this]() {
            processNextBatch();
        });
        return;
    }

    const Item nextItem = mCurrentUpdate.items.dequeue();

    Item item;
    item.setRemoteId(nextItem.remoteId());

    ItemFetchJob *job = new ItemFetchJob(item);
    job->setProperty("oldRemoteId", item.remoteId());
    job->setProperty("newRemoteId", nextItem.attribute<FileStore::EntityCompactChangeAttribute>()->remoteId());
    QObject::connect(job, &ItemFetchJob::result, q, [this](KJob *job) {
        itemFetchResult(job);
    });
}

void CompactChangeHelper::Private::itemFetchResult(KJob *job)
{
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);
    Q_ASSERT(fetchJob != nullptr);

    const QString oldRemoteId = fetchJob->property("oldRemoteId").toString();
    Q_ASSERT(!oldRemoteId.isEmpty());

    const QString newRemoteId = fetchJob->property("newRemoteId").toString();
    Q_ASSERT(!newRemoteId.isEmpty());

    if (fetchJob->error() != 0) {
        //qCCritical(MIXEDMAILDIRRESOURCE_LOG) << "Item fetch for remoteId=" << oldRemoteId
        //         << "new remoteId=" << newRemoteId << "failed:" << fetchJob->errorString();
        processNextItem();
        return;
    }

    // since we only need the item to modify its remote ID, we don't care
    // if it does not exist (anymore)
    if (fetchJob->items().isEmpty()) {
        processNextItem();
        return;
    }

    const Item item = fetchJob->items().at(0);

    Item updatedItem(item);
    updatedItem.setRemoteId(newRemoteId);

    ItemModifyJob *modifyJob = new ItemModifyJob(updatedItem);
    QObject::connect(modifyJob, &ItemModifyJob::result, q, [this]() {
        processNextItem();
    });
}

CompactChangeHelper::CompactChangeHelper(const QByteArray &sessionId, QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
    d->mSession = new Session(sessionId, this);
}

CompactChangeHelper::~CompactChangeHelper()
{
    delete d;
}

void CompactChangeHelper::addChangedItems(const Item::List &items)
{
    if (items.isEmpty()) {
        return;
    }

    qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "items.count=" << items.count()
                                      << "pendingUpdates.count=" << d->mPendingUpdates.count();
    UpdateBatch updateBatch;

    for (const Item &item : items) {
        const Collection collection = item.parentCollection();
        const qint64 revision = collection.remoteRevision().toLongLong();

        RevisionChangeMap &changesByRevision = d->mChangesByCollection[ collection.id() ];
        OldIdItemMap &changes = changesByRevision[ revision ];
        changes.insert(item.remoteId(), item);

        if (!updateBatch.collection.isValid()) {
            updateBatch.collection = collection;
        } else if (updateBatch.collection != collection) {
            d->mPendingUpdates << updateBatch;
            updateBatch.items.clear();
            updateBatch.collection = collection;
        }

        updateBatch.items << item;
    }

    if (updateBatch.collection.isValid()) {
        d->mPendingUpdates << updateBatch;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    QMetaObject::invokeMethod(this, [this]() {
        d->processNextBatch();
    }, Qt::QueuedConnection);
#else
    QMetaObject::invokeMethod(this, "processNextBatch", Qt::QueuedConnection);
#endif
}

QString CompactChangeHelper::currentRemoteId(const Item &item) const
{
    const Collection collection = item.parentCollection();
    const qint64 revision = collection.remoteRevision().toLongLong();

    QString remoteId = item.remoteId();

    const CollectionRevisionMap::const_iterator colIt = d->mChangesByCollection.constFind(collection.id());
    if (colIt != d->mChangesByCollection.constEnd()) {
        // find revision and iterate until the highest available one
        RevisionChangeMap::const_iterator revIt = colIt->constFind(revision);
        for (; revIt != colIt->constEnd(); ++revIt) {
            const OldIdItemMap::const_iterator idIt = revIt->constFind(remoteId);
            if (idIt != revIt->constEnd()) {
                remoteId = idIt.value().attribute<FileStore::EntityCompactChangeAttribute>()->remoteId();
            } else {
                break;
            }
        }
    }

    if (item.remoteId() != remoteId) {
        qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "item (id=" << item.id() << "remoteId=" << item.remoteId()
                                          << "), col(id=" << collection.id() << ", name=" << collection.name()
                                          << ", revision=" << revision << ") in compact change set (revisions="
                                          << colIt->keys() << ": current remoteId=" << remoteId;
    }

    return remoteId;
}

void CompactChangeHelper::checkCollectionChanged(const Collection &collection)
{
    const qint64 revision = collection.remoteRevision().toLongLong();
    //qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "col.id=" << collection.id() << ", remoteId=" << collection.remoteId()
    //         << "revision=" << revision;

    const CollectionRevisionMap::iterator colIt = d->mChangesByCollection.find(collection.id());
    if (colIt != d->mChangesByCollection.end()) {
        qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "matching change map found with" << colIt->count() << "entries";
        // remove all revisions until the seen one appears
        RevisionChangeMap::iterator revIt = colIt->begin();
        while (revIt != colIt->end() && revIt.key() <= revision) {
            qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "removing entry for revision" << revIt.key();
            revIt = colIt->erase(revIt);
        }

        if (revIt == colIt->end()) {
            qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "all change maps gone";
            d->mChangesByCollection.erase(colIt);
        }
    }
}

#include "moc_compactchangehelper.cpp"
