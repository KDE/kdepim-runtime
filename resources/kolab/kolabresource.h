/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef KOLABRESOURCE_H
#define KOLABRESOURCE_H

#include "imapresource.h"
#include <resourcestate.h>

class KolabResource : public ImapResource
{
    Q_OBJECT

    using Akonadi::AgentBase::Observer::collectionChanged;

public:
    explicit KolabResource(const QString &id);
    ~KolabResource();

protected Q_SLOTS:
    virtual void retrieveCollections();
    virtual void retrieveItems(const Akonadi::Collection& col);

protected:
    virtual ResourceStateInterface::Ptr createResourceState(const TaskArguments &);

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemsMoved( const Akonadi::Item::List &item, const Akonadi::Collection &source,
                           const Akonadi::Collection &destination );
    //itemsRemoved and itemsFlags changed do not require translation, because they don't use the payload

    virtual void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent);
    virtual void collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &parts);
    //collectionRemoved & collectionMoved do not require adjustments since they don't change the annotations

    virtual QString defaultName() const;

private Q_SLOTS:
    void onItemRetrievalCollectionFetchDone(KJob *job);
};

#endif
