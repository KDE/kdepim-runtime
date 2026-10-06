/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "resourcetask.h"

#include "davgroupwareresource.h"
#include "davresource_debug.h"

#include <kdav/davjobbase.h>

ResourceTask::ResourceTask(ResourceStateInterface::Ptr resource, QObject *parent)
    : QObject(parent)
    , m_resource(std::move(resource))
{
}

ResourceTask::~ResourceTask()
{
}

void ResourceTask::start()
{
    doStart();
}

void ResourceTask::finishTask()
{
    deleteLater();
}

KDAV::DavUrl ResourceTask::davUrlFromCollectionUrl(const QString &collectionUrl, const QString &finalUrl)
{
    return m_resource->settings()->davUrlFromCollectionUrl(collectionUrl, finalUrl);
}

ResourceTask::CollectionsDavItemCache ResourceTask::resourceDavItemCache()
{
    return m_resource->davItemCache();
}

void ResourceTask::changeProcessed()
{
    m_resource->changeProcessed();
    finishTask();
}

void ResourceTask::taskDone()
{
    m_resource->taskDone();
    finishTask();
}

void ResourceTask::cancelTask()
{
    m_resource->cancelTask();
    finishTask();
}

void ResourceTask::cancelTask(const QString &errorMessage)
{
    m_resource->cancelTask(errorMessage);
    finishTask();
}

void ResourceTask::retryAfterFailure(const QString &errorMessage)
{
    m_resource->retryAfterFailure(errorMessage);
    finishTask();
}

void ResourceTask::synchronizeCollection(const Akonadi::Collection &collection)
{
    Q_ASSERT(collection.isValid());
    if (collection.isValid()) {
        m_resource->synchronizeCollection(collection.id());
    }
}

void ResourceTask::onError(ErrorType errorType)
{
    switch (errorType) {
    case ErrorType::NoError:
        Q_ASSERT(false);
        break;
    case ErrorType::Retryable:
        m_error = errorType;
        break;
    case ErrorType::Unrecoverable:
        if (m_error != ErrorType::Retryable) {
            m_error = errorType;
        }
        break;
    }
}

void ResourceTask::onDavJobError(const KDAV::DavJobBase *job)
{
    if (job->error()) {
        if (job->canRetryLater()) {
            onError(ErrorType::Retryable);
            m_errorMessage = job->errorString();
        } else {
            onError(ErrorType::Unrecoverable);
        }
    }
}

#include "moc_resourcetask.cpp"
