/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Kevin Ottens <kevin@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EXPUNGECOLLECTIONTASK_H
#define EXPUNGECOLLECTIONTASK_H

#include "resourcetask.h"

class ExpungeCollectionTask : public ResourceTask
{
    Q_OBJECT

public:
    explicit ExpungeCollectionTask(const ResourceStateInterface::Ptr &resource, QObject *parent = nullptr);
    ~ExpungeCollectionTask() override;

private Q_SLOTS:
    void onSelectDone(KJob *job);
    void onExpungeDone(KJob *job);

protected:
    void doStart(KIMAP::Session *session) override;

private:
    void triggerExpungeJob(KIMAP::Session *session);
};

#endif
