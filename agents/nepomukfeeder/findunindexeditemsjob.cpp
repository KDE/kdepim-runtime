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
#include <Soprano/QueryResultIterator>
#include <Soprano/Model>
#include <QTime>


FindUnindexedItemsJob::FindUnindexedItemsJob(QObject* parent)
: KJob(parent)
{

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
    itemFetchJob->fetchScope().setFetchModificationTime(false);
    itemFetchJob->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    connect(itemFetchJob, SIGNAL(result(KJob*)), this, SLOT(itemsRetrieved(KJob*)));
    itemFetchJob->start();
}

void FindUnindexedItemsJob::itemsRetrieved(KJob *job)
{
    kDebug() << "Akonadi Query took(ms): " << mTime.elapsed();
    mTime.start();
    Akonadi::RecursiveItemFetchJob *itemFetchJob = static_cast<Akonadi::RecursiveItemFetchJob*>(job);
    Q_ASSERT(job);
    foreach (const Akonadi::Item &item, itemFetchJob->items()) {
        if (item.mimeType() != QLatin1String("message/rfc822"))
            continue;
        mAkonadiItems.insert(item.id(), item);
    }
    kDebug() << "copy took(ms): " << mTime.elapsed();
    retrieveIndexedNepomukResources();
}

void FindUnindexedItemsJob::retrieveIndexedNepomukResources()
{
    kDebug();
    mTime.start();
    int compatLevel = 3;
    Soprano::QueryResultIterator result = Nepomuk2::ResourceManager::instance()->mainModel()->executeQuery(
        QString::fromLatin1("SELECT ?id WHERE { ?r %2 %3 . ?r %4 ?id }")
            .arg(
            Soprano::Node::resourceToN3(Vocabulary::ANEO::akonadiIndexCompatLevel()),
            Soprano::Node::literalToN3(compatLevel),
            Soprano::Node::resourceToN3(Vocabulary::ANEO::akonadiItemId())
            ),
            Soprano::Query::QueryLanguageSparql);
    while (result.next()) {
        mNepomukItems.append(Akonadi::Item(result[0].literal().toInt64()));
    }
    kDebug() << "Nepomuk Query took(ms): " << mTime.elapsed();
    findUnindexed();
}

void FindUnindexedItemsJob::findUnindexed()
{
    mTime.start();
    foreach(const Akonadi::Item &item, mNepomukItems) {
        mAkonadiItems.remove(item.id());
    }
    kDebug() << "Finding Unindexed took(ms): " << mTime.elapsed();
    kDebug() << "Found " << getUnindexed().size() << " unindexed items.";
    emitResult();
}

Akonadi::Item::List FindUnindexedItemsJob::getUnindexed() const
{
    return mAkonadiItems.values();
}

#include "findunindexeditemsjob.moc"
