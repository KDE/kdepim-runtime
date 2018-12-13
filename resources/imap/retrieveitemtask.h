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

#ifndef RETRIEVEITEMTASK_H
#define RETRIEVEITEMTASK_H

#include <kimap/fetchjob.h>

#include "resourcetask.h"

class RetrieveItemTask : public ResourceTask
{
    Q_OBJECT

public:
    explicit RetrieveItemTask(const ResourceStateInterface::Ptr &resource, QObject *parent = nullptr);
    ~RetrieveItemTask() override;

private Q_SLOTS:
    void onSelectDone(KJob *job);
    void onMessagesReceived(const QMap<qint64, KIMAP::Message> &messages);
    void onContentFetchDone(KJob *job);

protected:
    void doStart(KIMAP::Session *session) override;

private:
    void triggerFetchJob();

    KIMAP::Session *m_session = nullptr;
    qint64 m_uid = 0;
    bool m_messageReceived = false;
};

#endif
