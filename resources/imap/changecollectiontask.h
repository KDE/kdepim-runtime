/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#ifndef CHANGECOLLECTIONTASK_H
#define CHANGECOLLECTIONTASK_H

#include "resourcetask.h"

class ChangeCollectionTask : public ResourceTask
{
    Q_OBJECT

public:
    explicit ChangeCollectionTask(const ResourceStateInterface::Ptr &resource, QObject *parent = nullptr);
    ~ChangeCollectionTask() override;

    void syncEnabledState(bool);

private Q_SLOTS:
    void onRenameDone(KJob *job);
    void onSubscribeDone(KJob *job);
    void onSetAclDone(KJob *job);
    void onSetMetaDataDone(KJob *job);

protected:
    void doStart(KIMAP::Session *session) override;

private:
    void endTaskIfNeeded();

    int m_pendingJobs = 0;
    Akonadi::Collection m_collection;
    bool m_syncEnabledState = true;
};

#endif
