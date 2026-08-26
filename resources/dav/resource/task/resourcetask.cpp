/*
    SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "resourcetask.h"

#include "davgroupwareresource.h"
#include "davresource_debug.h"

ResourceTask::ResourceTask(DavGroupwareResource *resource, QObject *parent)
    : QObject(parent)
    , m_resource(resource)
{
}

ResourceTask::~ResourceTask()
{
}

void ResourceTask::start()
{
    doStart();
}

void ResourceTask::doStart()
{
    finishTask();
}

void ResourceTask::finishTask()
{
    deleteLater();
}

QMap<QString, std::shared_ptr<DavItemCache>> ResourceTask::resourceDavItemCache()
{
    return m_resource->mDavItemCache;
}

Settings *ResourceTask::resourceSettings()
{
    return m_resource->mSettings;
}

void ResourceTask::changeProcessed()
{
    m_resource->changeProcessed();
    finishTask();
}

void ResourceTask::cancelTask()
{
    m_resource->cancelTask();
    finishTask();
}

#include "moc_resourcetask.cpp"
