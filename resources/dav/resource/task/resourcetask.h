/*
    SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include "davitemcache.h"

#include <Akonadi/Item>
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
    explicit ResourceTask(DavGroupwareResource *resource, QObject *parent = nullptr);
    ~ResourceTask() override;
    void start();

protected:
    virtual void doStart();

    QMap<QString, std::shared_ptr<DavItemCache>> resourceDavItemCache();
    Settings *resourceSettings();
    void changeProcessed();
    void cancelTask();

private:
    void finishTask();

private:
    DavGroupwareResource *m_resource = nullptr;
};
