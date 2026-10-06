/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "davitemcache.h"
#include "resourcestateinterface.h"

#include <Akonadi/Item>
#include <KDAV/DavUrl>
#include <QObject>

class Settings;
class DavGroupwareResource;

/**
 * @class ResourceTask
 * @brief A Dav Resource Task Base class
 *
 * This class provides a base structure for tasks to ease async operations and
 * avoid polluting the resource class.
 *
 * Responsibilities of derived implementations :
 * - Overriding doStart() with the desired logic,
 * - Finalise the resource's task using changeProcessed / changeCommitted / etc.
 */
class ResourceTask : public QObject
{
    Q_OBJECT
public:
    using CollectionsDavItemCache = QMap<QString, std::shared_ptr<DavItemCache>>;

public:
    explicit ResourceTask(ResourceStateInterface::Ptr resource, QObject *parent = nullptr);
    ~ResourceTask() override;
    void start();

protected:
    virtual void doStart() = 0;

    KDAV::DavUrl davUrlFromCollectionUrl(const QString &collectionUrl, const QString &finalUrl = QString());
    CollectionsDavItemCache resourceDavItemCache();

    void changeProcessed();
    void taskDone();
    void cancelTask();
    void cancelTask(const QString &errorMessage);
    void retryAfterFailure(const QString &errorMessage);

    void synchronizeCollection(const Akonadi::Collection &collection);

private:
    // Call once when task is finished, will schedule cleanup of the task eg. deleteLater
    void finishTask();

private:
    ResourceStateInterface::Ptr m_resource;
};
