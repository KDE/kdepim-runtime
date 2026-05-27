/*
    SPDX-FileCopyrightText: 2010 Grégory Oestreicher <greg@kamago.net>
    SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

#include <qhash.h>

namespace Akonadi
{
class Collection;
}

namespace KDAV
{
class EtagCache;
}

class KJob;

class DavEtagCache;

/**
 * @short A helper class to cache DAV items.
 *
 * Will use KDAV::EtagCache internally to cache items.
 * The KDAV::EtagCache caches the remote ids and etags of all items
 * in a given collection. This cache is needed to find
 * out which items have been changed in the backend and have to
 * be refetched on the next call of ResourceBase::retrieveItems()
 */
class DavItemCache : public QObject
{
    Q_OBJECT

public:
    /**
     * Creates a new etag cache and populates it with the ETags
     * of items found in @p collection.
     */
    explicit DavItemCache(const Akonadi::Collection &collection, QObject *parent = nullptr);
    ~DavItemCache() override;

    /**
     * Don't use this accessor, expected to be used only to share the map with KDAV
     * methods like setEtag need to be called on DavItemCache directly.
     * @return KDAV::EtagCache to share with KDAV jobs
     */
    [[nodiscard]] std::shared_ptr<KDAV::EtagCache> eTagCache();

    /**
     * Sets the ETag for the remote ID. If the remote ID is marked as
     * changed (is contained in the return of changedRemoteIds), remove
     * it from the changed list.
     */
    void setEtag(const QString &remoteId, const QString &etag);

    /**
     * Removes the entry for item with remote ID \a remoteId.
     */
    void removeEtag(const QString &remoteId);

    /**
     * Returns the list of all items URLs.
     */
    [[nodiscard]] QStringList urls() const;
    /**
     * Mark an item as changed in the backend.
     */
    void markAsChanged(const QString &remoteId);
    /**
     * Returns true if the remote ID is marked as changed (is contained in the
     * return of changedRemoteIds)
     */
    [[nodiscard]] bool isOutOfDate(const QString &remoteId) const;

    /**
     * Returns the list of exception URLs for the given remote ID.
     */
    [[nodiscard]] QStringList exceptionUrls(const QString &remoteId) const;
    void removeException(const QString &remoteId);

private:
    void onItemFetchJobFinished(KJob *job);
    [[nodiscard]] bool isExceptionRemoteId(const QString &remoteId) const;
    [[nodiscard]] QString baseRemoteId(const QString &exceptionRemoteId) const;
    /**
     * cache the exception remoteId
     */
    void cacheException(const QString &remoteId);

    std::shared_ptr<DavEtagCache> mEtagCache;
    QMultiHash<QString, QString> mExceptionCache;
};
