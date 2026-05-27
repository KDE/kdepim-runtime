/*
    SPDX-FileCopyrightText: 2010 Grégory Oestreicher <greg@kamago.net>
    SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "davitemcache.h"

#include "davetagcache.h"

#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <davresource_debug.h>

using namespace KDAV;

DavItemCache::DavItemCache(const Akonadi::Collection &collection, QObject *parent)
    : QObject(parent)
    , mEtagCache(std::make_shared<DavEtagCache>())
{
    auto job = new Akonadi::ItemFetchJob(collection);
    job->fetchScope().fetchFullPayload(false); // We only need the remote id and the revision
    connect(job, &Akonadi::ItemFetchJob::result, this, &DavItemCache::onItemFetchJobFinished);
    job->start();
}

DavItemCache::~DavItemCache() = default;

std::shared_ptr<KDAV::EtagCache> DavItemCache::eTagCache()
{
    return mEtagCache;
}

void DavItemCache::onItemFetchJobFinished(KJob *job)
{
    if (job->error()) {
        return;
    }

    const Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    const Akonadi::Item::List items = fetchJob->items();

    for (const Akonadi::Item &item : items) {
        if (isExceptionRemoteId(item.remoteId())) {
            cacheException(item.remoteId());
        } else if (!mEtagCache->contains(item.remoteId())) {
            mEtagCache->setEtagInternal(item.remoteId(), item.remoteRevision());
        }
    }
}

bool DavItemCache::isExceptionRemoteId(const QString &remoteId) const
{
    return remoteId.contains(u"#");
}

QString DavItemCache::baseRemoteId(const QString &exceptionRemoteId) const
{
    Q_ASSERT(!exceptionRemoteId.isEmpty() && isExceptionRemoteId(exceptionRemoteId));
    const auto separatorPosition = exceptionRemoteId.indexOf(u'#');
    Q_ASSERT(separatorPosition != -1);
    return exceptionRemoteId.mid(0, separatorPosition);
}

void DavItemCache::cacheException(const QString &remoteId)
{
    const QString mainItemRemoteId = baseRemoteId(remoteId);
    if (!mExceptionCache.contains(mainItemRemoteId, remoteId)) {
        mExceptionCache.insert(mainItemRemoteId, remoteId);
    }
}

QStringList DavItemCache::exceptionUrls(const QString &remoteId) const
{
    return mExceptionCache.values(remoteId);
}

void DavItemCache::removeException(const QString &remoteId)
{
    const QString mainItemRemoteId = baseRemoteId(remoteId);

    mExceptionCache.remove(mainItemRemoteId, remoteId);
}

void DavItemCache::setEtag(const QString &remoteId, const QString &etag)
{
    if (isExceptionRemoteId(remoteId)) {
        cacheException(remoteId);
    } else {
        mEtagCache->setEtag(remoteId, etag);
    }
}

QStringList DavItemCache::urls() const
{
    return mEtagCache->urls();
}

void DavItemCache::removeEtag(const QString &remoteId)
{
    if (isExceptionRemoteId(remoteId)) {
        removeException(remoteId);
    } else {
        mEtagCache->removeEtag(remoteId);
    }
}

void DavItemCache::markAsChanged(const QString &remoteId)
{
    Q_ASSERT(!remoteId.isEmpty() && !isExceptionRemoteId(remoteId));
    mEtagCache->markAsChanged(remoteId);
}

bool DavItemCache::isOutOfDate(const QString &remoteId) const
{
    return mEtagCache->isOutOfDate(remoteId);
}

#include "moc_davitemcache.cpp"
