/*
    SPDX-FileCopyrightText: 2014 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Kevin Krammer <kevin.krammer@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOLABREMOVETAGTASK_H
#define KOLABREMOVETAGTASK_H

#include "kolabrelationresourcetask.h"

class KolabRemoveTagTask : public KolabRelationResourceTask
{
    Q_OBJECT
public:
    explicit KolabRemoveTagTask(const ResourceStateInterface::Ptr &resource, QObject *parent = nullptr);

protected:
    void startRelationTask(KIMAP::Session *session) override;

private:
    void triggerStoreJob(KIMAP::Session *session);

private Q_SLOTS:
    void onSelectDone(KJob *job);
    void onStoreFlagsDone(KJob *job);
};

#endif // KOLABREMOVETAGTASK_H
