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

#ifndef KOLABADDTAGTASK_H
#define KOLABADDTAGTASK_H

#include "kolabrelationresourcetask.h"

class KolabAddTagTask : public KolabRelationResourceTask
{
    Q_OBJECT
public:
    explicit KolabAddTagTask(const ResourceStateInterface::Ptr &resource, QObject *parent = nullptr);

protected:
    void startRelationTask(KIMAP::Session *session) Q_DECL_OVERRIDE;

private:
    QByteArray mMessageId;

private:
    void applyFoundUid(qint64 uid);
    void triggerSearchJob(KIMAP::Session *session);

private Q_SLOTS:
    void onAppendMessageDone(KJob *job);
    void onPreSearchSelectDone(KJob *job);
    void onSearchDone(KJob *job);
};

#endif // KOLABADDTAGTASK_H
