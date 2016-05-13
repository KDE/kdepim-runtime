/*
    Copyright (c) 2014 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Krammer <kevin.krammer@kdab.com>

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

#ifndef KOLABREMOVETAGTASK_H
#define KOLABREMOVETAGTASK_H

#include "kolabrelationresourcetask.h"

class KolabRemoveTagTask : public KolabRelationResourceTask
{
    Q_OBJECT
public:
    explicit KolabRemoveTagTask(const ResourceStateInterface::Ptr &resource, QObject *parent = Q_NULLPTR);

protected:
    virtual void startRelationTask(KIMAP::Session *session);

private:
    void triggerStoreJob(KIMAP::Session *session);

private Q_SLOTS:
    void onSelectDone(KJob *job);
    void onStoreFlagsDone(KJob *job);
};

#endif // KOLABREMOVETAGTASK_H
