/*
 *   Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "findunindexeditemsjob.h"
#include <aneo.h>
#include <Akonadi/RecursiveItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Collection>
#include <Nepomuk2/ResourceManager>
#include <Soprano/Util/AsyncQuery>
#include <Soprano/Node>
#include <Nepomuk2/Vocabulary/NIE>
#include <QTime>
#include <QStringList>


FindUnindexedItemsJob::FindUnindexedItemsJob(int compatLevel, QObject* parent)
: KJob(parent),
  mCompatLevel(compatLevel)
{

}

FindUnindexedItemsJob::~FindUnindexedItemsJob()
{
    //Free the memory we used
    mAkonadiItems.clear();
    mStaleItems.clear();
}

void FindUnindexedItemsJob::setIndexedCollections(const Akonadi::Collection::List &collections)
{
    mIndexedCollections = collections;
}

void FindUnindexedItemsJob::start()
{
    retrieveAkonadiItems();
}

void FindUnindexedItemsJob::retrieveAkonadiItems()
{
    kDebug();
    mTime.start();
    Akonadi::RecursiveItemFetchJob *itemFetchJob = new Akonadi::RecursiveItemFetchJob(Akonadi::Collection::root(), QStringList(), this);
    itemFetchJob->fetchScope().fetchAllAttributes(false);
    itemFetchJob->fetchScope().fetchFullPayload(false);
    itemFetchJob->fetchScope().setFetchModificationTime(true);
    itemFetchJob->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    itemFetchJob->setAutoDelete(false);
    connect(itemFetchJob, SIGNAL(result(KJob*)), this, SLOT(itemsRetrieved(KJob*)));
    itemFetchJob->start();
}

void FindUnindexedItemsJob::itemsRetrieved(KJob *job)
{
    kDebug() << "Akonadi Query took(ms): " << mTime.elapsed();
    if (job->error()) {
        setError(KJob::UserDefinedError);
        setErrorText("Retrieving items failed");
        emitResult();
        return;
    }
    mTime.start();
    Akonadi::RecursiveItemFetchJob *itemFetchJob = static_cast<Akonadi::RecursiveItemFetchJob*>(job);
    Q_ASSERT(job);
    mTotalNumberOfItems = itemFetchJob->items().size();
    foreach (const Akonadi::Item &item, itemFetchJob->items()) {
        if (!mIndexedCollections.isEmpty() && !mIndexedCollections.contains(item.parentCollection())) {
            continue;
        }
        mAkonadiItems.insert(item.id(), qMakePair(item.modificationTime(), item.mimeType()));
    }
    //Make sure the job is immediately deleted when entering the eventloop the next time.
    itemFetchJob->deleteLater();
    kDebug() << "copy took(ms): " << mTime.elapsed();
    //Allow the above job to delete itself
    QMetaObject::invokeMethod(this, "retrieveIndexedNepomukResources", Qt::QueuedConnection);
}

void FindUnindexedItemsJob::retrieveIndexedNepomukResources()
{
    kDebug();
    mTime.start();
    Q_ASSERT(Nepomuk2::ResourceManager::instance()->initialized());
    mQuery = QSharedPointer<Soprano::Util::AsyncQuery>(Soprano::Util::AsyncQuery::executeQuery(Nepomuk2::ResourceManager::instance()->mainModel(),
        QString::fromLatin1("SELECT ?id ?lastMod WHERE { ?r %1 ?id. ?r %2 ?lastMod }")
            .arg(Soprano::Node::resourceToN3(Vocabulary::ANEO::akonadiItemId()),
            Soprano::Node::resourceToN3(Nepomuk2::Vocabulary::NIE::lastModified())),
            Soprano::Query::QueryLanguageSparql));
    connect(mQuery.data(), SIGNAL(nextReady(Soprano::Util::AsyncQuery*)), this, SLOT(processResult(Soprano::Util::AsyncQuery*)));
    connect(mQuery.data(), SIGNAL(finished(Soprano::Util::AsyncQuery*)), this, SLOT(queryFinished(Soprano::Util::AsyncQuery*)));
}

void FindUnindexedItemsJob::processResult(Soprano::Util::AsyncQuery *query)
{
    const Akonadi::Item::Id &id = query->binding(0).literal().toInt64();
    ItemHash::iterator it = mAkonadiItems.find(id);
    if (it == mAkonadiItems.end()) { //Not found in akonadi, stale
        mStaleItems.append(id);
    } else if (query->binding(1).literal().toDateTime() == it->first) { //Found and up-to-date
        mAkonadiItems.erase(it);
    }
    query->next();
}

void FindUnindexedItemsJob::queryFinished(Soprano::Util::AsyncQuery *query)
{
    if (query->lastError()) {
        mAkonadiItems.clear();
        kWarning() << query->lastError();
        setError(KJob::UserDefinedError);
        setErrorText("Nepomuk query failed");
        emitResult();
        return;
    }
    kDebug() << "Nepomuk Query took(ms): " << mTime.elapsed();
    kDebug() << "Found " << getUnindexed().size() << " unindexed items.";
    kDebug() << "Found " << mStaleItems.size() << " items which can be removed from nepomuk.";
    kDebug() << "out of " << mTotalNumberOfItems << " items.";
    emitResult();
}

const FindUnindexedItemsJob::ItemHash &FindUnindexedItemsJob::getUnindexed() const
{
    return mAkonadiItems;
}

const QList<Akonadi::Item::Id> &FindUnindexedItemsJob::getItemsToRemove() const
{
    return mStaleItems;
}

#include "findunindexeditemsjob.moc"

