/*
    Copyright (c) 2010 Grégory Oestreicher <greg@kamago.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef ETAGCACHE_H
#define ETAGCACHE_H

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QStringList>

namespace Akonadi
{
class Collection;
}

class KJob;

/**
 * @short A helper class to cache etags.
 *
 * The EtagCache caches the remote ids and etags of all items
 * in a given collection. This cache is needed to find
 * out which items have been changed in the backend and have to
 * be refetched on the next call of ResourceBase::retrieveItems()
 */
class EtagCache : public QObject
{
    Q_OBJECT

public:
    /**
     * Creates a new etag cache and populates it with the ETags
     * of items found in @p collection.
     */
    explicit EtagCache(const Akonadi::Collection &collection, QObject *parent = Q_NULLPTR);

    /**
     * Sets the ETag for the remote ID. If the remote ID is marked as
     * changed (is contained in the return of changedRemoteIds), remove
     * it from the changed list.
     */
    void setEtag(const QString &remoteId, const QString &etag);

    /**
     * Checks if the given item is in the cache
     */
    bool contains(const QString &remoteId);

    /**
     * Check if the known ETag for the remote ID is equal to @p refEtag.
     */
    bool etagChanged(const QString &remoteId, const QString &refEtag) const;

    /**
     * Mark an item as changed in the backend.
     */
    void markAsChanged(const QString &remoteId);

    /**
     * Returns true if the remote ID is marked as changed (is contained in the
     * return of changedRemoteIds)
     */
    bool isOutOfDate(const QString &remoteId) const;

    /**
     * Removes the entry for item with remote ID @p remoteId.
     */
    void removeEtag(const QString &remoteId);

    /**
     * Returns the list of all items URLs.
     */
    QStringList urls() const;

    /**
     * Returns the list of remote ids of items that have been changed
     * in the backend.
     */
    QStringList changedRemoteIds() const;

private Q_SLOTS:
    void onItemFetchJobFinished(KJob *job);

private:
    QMap<QString, QString> mCache;
    QSet<QString> mChangedRemoteIds;
};

#endif
