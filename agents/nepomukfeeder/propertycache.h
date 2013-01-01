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

#ifndef PROPERTYCACHE_H
#define PROPERTYCACHE_H
#include <QUrl>
#include <qcache.h>
#include <nepomuk2/simpleresource.h>
#include <nepomuk2/simpleresourcegraph.h>

/**
 * A cache for nepomuk resources which are being stored using a SimpleResourceGraph.
 * The basic idea is to avoid repeatedly storing the same subresources of resources,
 * such as i.e. the email headers which are part of nearly every email.
 * To achieve this, all stored resouces are hashed, and the cache maintains hash-resourceUri pairs.
 * If a new graph is passed to the cache, equivalent resources can be identified based the hash,
 * which allows to insert the final resourceUri right away and avoids storing the same resource again.
 */
class PropertyCache
{
public:
    PropertyCache();
    
    /**
     * Fill the cache using the stored graph (which contains temporary uris), and the mapping to the final uris
     * which is returned by Nepomuk2::StoreResourceJob.
     * This will fill the cache with resource of the cached types.
     */
    void fillCache(const Nepomuk2::SimpleResourceGraph &graph, const QHash<QUrl, QUrl> &mappings);
    
    /**
     * Apply the cache to the graph, and return a graph with all already cached resources removed and the corresponding references
     * replaced by a final resource-uri.
     */
    Nepomuk2::SimpleResourceGraph applyCache(const Nepomuk2::SimpleResourceGraph &) const;
    
    /**
     * Sets the RDF:Types which should be cached.
     * Nothin is cached by default.
     */
    void setCachedTypes(const QList<QUrl> &);
    
    /**
     * Sets the maximum cache size.
     * The default is 100 items
     */
    void setSize(int);

private:
    QMap<uint, QUrl> hashResources(const Nepomuk2::SimpleResourceGraph &graph) const;
    QHash<QUrl, uint> findCachedResources(const Nepomuk2::SimpleResourceGraph &graph) const;
    void applyCache(Nepomuk2::SimpleResource &res, const QHash<QUrl, uint> &tempHashMap) const;
    bool cachingEnabled(const Nepomuk2::SimpleResource &res) const;
    uint hashResource(Nepomuk2::SimpleResource &res, QList<Nepomuk2::SimpleResource> &list) const;
    uint getHashOfProperty(const QUrl &url, QList<Nepomuk2::SimpleResource> &list) const;
    uint hashProperties(const Nepomuk2::PropertyHash &properties) const;
    QCache<uint, QUrl> mCache;
    QList<QUrl> mCachedTypes;
//     mutable uint mTotal;
//     mutable uint mHits;
};

#endif // PROPERTYCACHE_H
