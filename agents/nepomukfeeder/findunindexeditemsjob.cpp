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
#include "nepomukfeeder-config.h"
#include "pluginloader.h"
#include "nepomukhelpers.h"

#include <aneo.h>

#include <Akonadi/ItemFetchScope>
#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ItemFetchJob>

#include <Nepomuk2/ResourceManager>
#include <Nepomuk2/Vocabulary/NIE>

#include <Soprano/Util/AsyncQuery>
#include <Soprano/Node>

#include <QTime>
#include <QStringList>

#ifdef HAVE_MALLOC_H
    #include <malloc.h>
#endif


FindUnindexedItemsJob::FindUnindexedItemsJob(int compatLevel, QObject* parent)
: KJob(parent),
  mCompatLevel(compatLevel),
  mTotalNumberOfItems(0),
  m_killed(false)
{

}

FindUnindexedItemsJob::~FindUnindexedItemsJob()
{
    //Free the memory we used
    mAkonadiItems.clear();
    mStaleUris.clear();
#ifdef HAVE_MALLOC_TRIM
    malloc_trim(0);
#endif
}

void FindUnindexedItemsJob::setIndexedCollections(const Akonadi::Collection::List &collections)
{
    mIndexedCollections = collections;
}

void FindUnindexedItemsJob::start()
{
    Akonadi::CollectionFetchJob* collectionFetchJob = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(),
                                                            Akonadi::CollectionFetchJob::Recursive,
                                                            this);
    connect(collectionFetchJob, SIGNAL(finished(KJob*)), this, SLOT(slotCollectionListReceived(KJob*)));
    collectionFetchJob->start();
}

void FindUnindexedItemsJob::slotCollectionListReceived(KJob* job)
{
    if (job->error()) {
        kWarning() << "Failed to fetch collections";
        return;
    }

    Akonadi::CollectionFetchJob* collectionFetchJob = static_cast<Akonadi::CollectionFetchJob*>(job);
    Akonadi::Collection::List allCollections = collectionFetchJob->collections();

    Akonadi::Collection::List indexedCollections;
    foreach (const Akonadi::Collection &col, allCollections) {
        if (!indexingDisabled(col)) {
            indexedCollections << col;
        }
    }
    setIndexedCollections( indexedCollections );

    mTime.start();
    fetchItemsFromCollection();
}

void FindUnindexedItemsJob::fetchItemsFromCollection()
{
    if( m_killed )
        return;

    if (mIndexedCollections.isEmpty()) {
        kDebug() << "Akonadi Query took(ms): " << mTime.elapsed();
        QMetaObject::invokeMethod(this, "retrieveIndexedNepomukResources", Qt::QueuedConnection);
        return;
    }
    const Akonadi::Collection col = mIndexedCollections.takeLast();
    Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(col, this);
    fetchJob->fetchScope().fetchAllAttributes(false);
    fetchJob->fetchScope().fetchFullPayload(false);
    fetchJob->fetchScope().setFetchModificationTime(true);
    connect(fetchJob, SIGNAL(itemsReceived(Akonadi::Item::List)), this, SLOT(itemsReceived(Akonadi::Item::List)));
    connect(fetchJob, SIGNAL(result(KJob*)), this, SLOT(jobDone(KJob*)));
}

void FindUnindexedItemsJob::itemsReceived(const Akonadi::Item::List &items)
{
    if( m_killed )
        return;

    mTotalNumberOfItems += items.size();
    foreach (const Akonadi::Item &item, items) {
        mAkonadiItems.insert(item.id(), qMakePair(item.modificationTime(), item.mimeType()));
    }
}

void FindUnindexedItemsJob::jobDone(KJob *job)
{
    if (job->error()) {
        mAkonadiItems.clear();
        setError(KJob::UserDefinedError);
        setErrorText("Retrieving items failed");
        emitResult();
        return;
    }

    fetchItemsFromCollection();
}

void FindUnindexedItemsJob::retrieveIndexedNepomukResources()
{
    if( m_killed )
        return;

#ifdef HAVE_MALLOC_TRIM
    malloc_trim(0);
#endif

    kDebug();
    mTime.start();
    Q_ASSERT(Nepomuk2::ResourceManager::instance()->initialized());

    QString query = QString::fromLatin1("SELECT ?r ?id ?lastMod WHERE { ?r %1 ?id. ?r %2 ?lastMod }")
                    .arg( Soprano::Node::resourceToN3(Vocabulary::ANEO::akonadiItemId()),
                          Soprano::Node::resourceToN3(Nepomuk2::Vocabulary::NIE::lastModified()) );

    Soprano::Model* model = Nepomuk2::ResourceManager::instance()->mainModel();

    Soprano::Util::AsyncQuery* mQuery = Soprano::Util::AsyncQuery::executeQuery(model,
                                                        query, Soprano::Query::QueryLanguageSparql );

    connect(mQuery, SIGNAL(nextReady(Soprano::Util::AsyncQuery*)),
            this, SLOT(processResult(Soprano::Util::AsyncQuery*)));
    connect(mQuery, SIGNAL(finished(Soprano::Util::AsyncQuery*)),
            this, SLOT(queryFinished(Soprano::Util::AsyncQuery*)));
}

void FindUnindexedItemsJob::processResult(Soprano::Util::AsyncQuery *query)
{
    if( m_killed ) {
        query->close();
        return;
    }

    const Akonadi::Item::Id &id = query->binding("id").literal().toInt64();
    ItemHash::iterator it = mAkonadiItems.find(id);
    if (it == mAkonadiItems.end()) { //Not found in akonadi, stale
        mStaleUris << query->binding("r").uri();
    } else if (query->binding("lastMod").literal().toDateTime() == it->first) { //Found and up-to-date
        mAkonadiItems.erase(it);
    }
    query->next();
}

void FindUnindexedItemsJob::queryFinished(Soprano::Util::AsyncQuery *query)
{
    if (query->lastError()) {
        mAkonadiItems.clear();
        mStaleUris.clear();
        kWarning() << query->lastError();
        setError(KJob::UserDefinedError);
        setErrorText("Nepomuk query failed");
        emitResult();
        return;
    }
    kDebug() << "Nepomuk Query took(ms): " << mTime.elapsed();
    kDebug() << "Found " << getUnindexed().size() << " unindexed items.";
    kDebug() << "Found " << mStaleUris.size() << " items which can be removed from nepomuk.";
    kDebug() << "out of " << mTotalNumberOfItems << " items.";
    emitResult();
}

const FindUnindexedItemsJob::ItemHash &FindUnindexedItemsJob::getUnindexed() const
{
    return mAkonadiItems;
}

const QList<QUrl> &FindUnindexedItemsJob::staleUris() const
{
    return mStaleUris;
}

int FindUnindexedItemsJob::indexedCount() const
{
    return (mTotalNumberOfItems - getUnindexed().size());
}

int FindUnindexedItemsJob::totalCount() const
{
    return mTotalNumberOfItems;
}

bool FindUnindexedItemsJob::doKill()
{
    m_killed = true;
    return true;
}


#include "findunindexeditemsjob.moc"
