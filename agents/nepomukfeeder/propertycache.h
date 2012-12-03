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


class PropertyCache
{
public:
    PropertyCache();
    void fillCache(const Nepomuk2::SimpleResourceGraph &graph, const QHash<QUrl, QUrl> &mappings);
    Nepomuk2::SimpleResourceGraph applyCache(const Nepomuk2::SimpleResourceGraph &) const;
    void setCachedTypes(const QList<QUrl> &);
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
