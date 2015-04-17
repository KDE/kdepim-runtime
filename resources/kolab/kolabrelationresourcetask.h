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

#ifndef KOLABRELATIONRESOURCETASK_H
#define KOLABRELATIONRESOURCETASK_H

#include <resourcetask.h>

class KolabRelationResourceTask : public ResourceTask
{
    Q_OBJECT
public:
    explicit KolabRelationResourceTask(ResourceStateInterface::Ptr resource, QObject *parent = 0);

    Akonadi::Collection relationCollection() const;

    using ResourceTask::mailBoxForCollection;
    using ResourceTask::resourceState;

protected:
    Akonadi::Collection mRelationCollection;

protected:
    virtual void doStart(KIMAP::Session *session);

    virtual void startRelationTask(KIMAP::Session *session) = 0;

private:
    KIMAP::Session *mImapSession;

private Q_SLOTS:
    void onCollectionFetchResult(KJob *job);
    void onCreateDone(KJob *job);
    void onSetMetaDataDone(KJob *job);
    void onLocalCreateDone(KJob *job);
};

#endif // KOLABRELATIONRESOURCETASK_H
