/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "propertycache.h"

#include <nepomuk2/simpleresourcegraph.h>
#include <nepomuk2/datamanagement.h>
#include <nepomuk2/storeresourcesjob.h>
#include <nepomuk2/simpleresource.h>
#include <KDebug>
#include <soprano/rdf.h>


PropertyCache::PropertyCache()
// :   mTotal(1),
//     mHits(1)
{
}

void PropertyCache::setCachedTypes(const QList< QUrl > &cachedTypes)
{
    mCachedTypes = cachedTypes;
}

void PropertyCache::setSize(int size)
{
    mCache.setMaxCost(size);
}

/*
 * This hash function uses XOR to combine the individual hashes of each property.
 * Due to this:
 * * the order of the hashes is not preseved in the hash (XOR is commutative)
 * * XOR of two equal values results in 0, but that shouldn't be a problem in this specific case.
 */
uint PropertyCache::hashProperties(const Nepomuk2::PropertyHash &properties) const
{
    uint hash = 0;
    QHashIterator<QUrl, QVariant> it( properties );
    while( it.hasNext() ) {
        it.next();
        hash ^= qHash( it.key() ) ^ qHash( it.value().toString() );
    }
    return hash;
}

uint PropertyCache::getHashOfProperty(const QUrl &url, QList<Nepomuk2::SimpleResource> &list) const
{
    QList<Nepomuk2::SimpleResource>::iterator it = list.begin();
    for (;it != list.end(); it++) {
        if (it->uri() == url) {
            return hashResource(*it, list);
        }
    }
    //If we get a full graph, every temporary uri should also be desribed in the same graph
    kWarning() << "resource not found " << url;
    Q_ASSERT(0);
    return 0;
}

uint PropertyCache::hashResource(Nepomuk2::SimpleResource &res, QList<Nepomuk2::SimpleResource> &list) const
{
    const Nepomuk2::SimpleResource copy = res;
    Nepomuk2::PropertyHash::const_iterator it = copy.properties().constBegin();
    for (;it != copy.properties().constEnd(); it++) {
        if (it.value().canConvert<QUrl>() && it.value().toUrl().toString().startsWith("_:")) {
            const int hash = getHashOfProperty(it.value().toUrl(), list);
            res.setProperty(it.key(), QString::number(hash));
        }
    }
    return hashProperties(res.properties());
}

void PropertyCache::applyCache(Nepomuk2::SimpleResource &res, const QHash<QUrl, uint> &tempHashMap) const
{
    const Nepomuk2::SimpleResource copy = res;
    Nepomuk2::PropertyHash::const_iterator it = copy.properties().constBegin();
    for (;it != copy.properties().constEnd(); it++) {
        if (it.value().canConvert<QUrl>() && it.value().toUrl().toString().startsWith("_:")) {
            uint hash = tempHashMap.value(it.value().toUrl());
//             mTotal++;
            if (!mCache.contains(hash)) {
//                 kDebug() << "value not available in cache";
                continue;
            }
//             mHits++;
//             kDebug() << "Hit ratio: " << (double)mHits/(double)mTotal << " Hits: " << mHits << " Total: " << mTotal;
            res.setProperty(it.key(), *mCache.object(hash));
        }
    }
}

bool PropertyCache::cachingEnabled(const Nepomuk2::SimpleResource& res) const
{
//     kDebug() << "types: " << res.property(Soprano::Vocabulary::RDF::type());
    const QVariantList types = res.property(Soprano::Vocabulary::RDF::type());
    foreach (const QVariant &variant, types) {
        Q_ASSERT(variant.canConvert<QUrl>());
        if (mCachedTypes.contains(variant.toUrl())) {
            return true;
        }
    }
    return false;
}

QHash<QUrl, uint> PropertyCache::findCachedResources(const Nepomuk2::SimpleResourceGraph &graph) const
{
    QHash<QUrl, uint> tempHashMap;
    QList<Nepomuk2::SimpleResource> list = graph.toList();
    for (QList<Nepomuk2::SimpleResource>::iterator it = list.begin(); it != list.end(); it++) {
        if (!cachingEnabled(*it)) { //If the caching wasn't enabled before it likely still isn't (that saves us the hashing)
            continue;
        }

        uint hash = hashResource(*it, list);
        if (mCache.contains(hash)) {
            tempHashMap.insert(it->uri(), hash);
        }
    }
    return tempHashMap;
}

/*
 * Go through graph, and replace every temporary uri with the hash of the referenced resource (so the hashing process gives always the same result)
 * We cannot hash the temporary uri as it wouldn't be guaranteed to be always the same.
 * In the end return a mapping of the temporary uri to the corresponding resources hash.
 */
QMap<uint, QUrl> PropertyCache::hashResources(const Nepomuk2::SimpleResourceGraph& graph) const
{
    QMap<uint, QUrl> waitingForUri;
    QList<Nepomuk2::SimpleResource> list = graph.toList();
    for (QList<Nepomuk2::SimpleResource>::iterator it = list.begin(); it != list.end(); it++) {
        if (!cachingEnabled(*it)) {
            continue;
        }
        uint hash = hashResource(*it, list);
        if (!mCache.contains(hash)) {
            waitingForUri.insert(hash, it->uri()); //insert into cache after we have the map
        }
    }
    return waitingForUri;
}

/*
 * Fill the cache once we have the definitive mapping from the storeresourcesjob.
 * Note that we cannot replace the temporary uris right away, as the whole point of the cache is that we can identify equivalent resources before storage (so the cache results in less data to store).
 */
void PropertyCache::fillCache(const Nepomuk2::SimpleResourceGraph& graph, const QHash< QUrl, QUrl >& mappings)
{
    const QMap<uint, QUrl> waitingForUri = hashResources(graph);
    QMap<uint, QUrl>::const_iterator it = waitingForUri.constBegin();
    for (; it != waitingForUri.constEnd(); it++) {
        Q_ASSERT(mappings.contains(it.value()));
        mCache.insert(it.key(), new QUrl(mappings.value(it.value())));
    }
}


Nepomuk2::SimpleResourceGraph PropertyCache::applyCache(const Nepomuk2::SimpleResourceGraph &graph) const
{
    //find the resources which we have already cached
    QHash<QUrl, uint> tempHashMap = findCachedResources(graph);

    QList<Nepomuk2::SimpleResource> result = graph.toList();

    //Remove what we have already stored
    QMutableListIterator<Nepomuk2::SimpleResource> it(result);
    while (it.hasNext()) {
        const Nepomuk2::SimpleResource &res = it.next();
        if (tempHashMap.contains(res.uri()) && mCache.contains(tempHashMap.value(res.uri()))) {
//             kDebug() << "removing " << res.uri();
            it.remove();
        }
    }

    //Go through all properties and replace any temp uri with it's cache value based on tempHashMap and cache
    QList<Nepomuk2::SimpleResource>::iterator resIt = result.begin();
    for (;resIt != result.end(); resIt++) {
        applyCache(*resIt, tempHashMap);
    }

//     kDebug() << tempHashMap;
//     qDebug() << "------------";
//     qDebug() << result;
//     qDebug() << "------------";

    return Nepomuk2::SimpleResourceGraph(result);
}

