/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef CHANGEITEMSFLAGSTASK_H
#define CHANGEITEMSFLAGSTASK_H

#include "resourcetask.h"

namespace KIMAP {
class StoreJob;
}

class ChangeItemsFlagsTask : public ResourceTask
{
    Q_OBJECT

public:
    explicit ChangeItemsFlagsTask(const ResourceStateInterface::Ptr &resource, QObject *parent = nullptr);
    ~ChangeItemsFlagsTask() override;

protected Q_SLOTS:
    void onSelectDone(KJob *job);
    void onAppendFlagsDone(KJob *job);
    void onRemoveFlagsDone(KJob *job);

protected:
    KIMAP::StoreJob *prepareJob(KIMAP::Session *session);

    void doStart(KIMAP::Session *session) override;

    virtual void triggerAppendFlagsJob(KIMAP::Session *session);
    virtual void triggerRemoveFlagsJob(KIMAP::Session *session);

protected:
    int m_processedItems = 0;
};

#endif
