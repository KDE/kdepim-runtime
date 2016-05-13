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

#ifndef KOLABCHANGEITEMSRELATIONSTASK_H
#define KOLABCHANGEITEMSRELATIONSTASK_H

#include "kolabrelationresourcetask.h"

class KolabChangeItemsRelationsTask : public KolabRelationResourceTask
{
    Q_OBJECT
public:
    explicit KolabChangeItemsRelationsTask(const ResourceStateInterface::Ptr &resource, QObject *parent = Q_NULLPTR);

protected:
    virtual void startRelationTask(KIMAP::Session *session);

private Q_SLOTS:
    void onRelationFetchDone(KJob *job);

    void addRelation(const Akonadi::Relation &relation);
    void onItemsFetched(KJob *job);
    void removeRelation(const Akonadi::Relation &relation);
    void onSelectDone(KJob *job);
    void triggerStoreJob();
    void onChangeCommitted(KJob *job);

private:
    void processNextRelation();

    KIMAP::Session *mSession;
    Akonadi::Relation::List mAddedRelations;
    Akonadi::Relation::List mRemovedRelations;
    Akonadi::Relation mCurrentRelation;
    bool mAdding;
};

#endif
