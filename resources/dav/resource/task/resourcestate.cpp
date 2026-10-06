/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "resourcestate.h"

#include "davgroupwareresource.h"

#include <klocalizedstring.h>

ResourceState::ResourceState(DavGroupwareResource *resource)
    : m_resource(resource)
{
}

ResourceState::~ResourceState() = default;

QString ResourceState::resourceName() const
{
    return m_resource->name();
}

QString ResourceState::resourceIdentifier() const
{
    return m_resource->identifier();
}

void ResourceState::itemRetrieved(const Akonadi::Item &item)
{
    m_resource->itemRetrieved(item);
}

void ResourceState::itemsRetrieved(const Akonadi::Item::List &items)
{
    m_resource->itemsRetrieved(items);
}

void ResourceState::setTotalItems(int items)
{
    m_resource->setTotalItems(items);
}

void ResourceState::itemsRetrievedIncremental(const Akonadi::Item::List &changed, const Akonadi::Item::List &removed)
{
    m_resource->itemsRetrievedIncremental(changed, removed);
}

void ResourceState::itemsRetrievalDone()
{
    m_resource->itemsRetrievalDone();
}

void ResourceState::collectionsRetrieved(const Akonadi::Collection::List &collections)
{
    m_resource->collectionsRetrieved(collections);
}

void ResourceState::collectionAttributesRetrieved(const Akonadi::Collection &collection)
{
    m_resource->collectionAttributesRetrieved(collection);
}

void ResourceState::itemChangeCommitted(const Akonadi::Item &item)
{
    m_resource->changeCommitted(item);
}

void ResourceState::itemsChangesCommitted(const Akonadi::Item::List &items)
{
    m_resource->changesCommitted(items);
}

void ResourceState::collectionChangeCommitted(const Akonadi::Collection &collection)
{
    m_resource->changeCommitted(collection);
}

void ResourceState::changeProcessed()
{
    m_resource->changeProcessed();
}

void ResourceState::taskDone()
{
    m_resource->taskDone();
}

void ResourceState::cancelTask()
{
    m_resource->cancelTask();
}

void ResourceState::cancelTask(const QString &errorString)
{
    m_resource->cancelTask(errorString);
}

void ResourceState::deferTask()
{
    m_resource->deferTask();
}

void ResourceState::retryAfterFailure(const QString &message)
{
    return m_resource->retryAfterFailure(message);
}

void ResourceState::emitError(const QString &message)
{
    Q_EMIT m_resource->error(message);
}

void ResourceState::emitWarning(const QString &message)
{
    Q_EMIT m_resource->warning(message);
}

void ResourceState::emitPercent(int percent)
{
    Q_EMIT m_resource->percent(percent);
}

void ResourceState::synchronizeCollection(Akonadi::Collection::Id id)
{
    m_resource->synchronizeCollection(id);
}

void ResourceState::synchronizeCollectionTree()
{
    m_resource->synchronizeCollectionTree();
}

auto ResourceState::settings() -> Settings *
{
    return m_resource->mSettings;
}

auto ResourceState::davItemCache() -> QMap<QString, std::shared_ptr<DavItemCache>>
{
    return m_resource->mDavItemCache;
}
