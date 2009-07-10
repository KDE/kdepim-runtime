/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef COLLECTIONTREEBUILDER_H
#define COLLECTIONTREEBUILDER_H

#include "kolabproxyresource.h"

#include <akonadi/collection.h>
#include <akonadi/job.h>

class CollectionTreeBuilder : public Akonadi::Job
{
  Q_OBJECT
  public:
    explicit CollectionTreeBuilder(KolabProxyResource* parent = 0);

    Akonadi::Collection::List allCollections() const;

  protected:
    virtual void doStart();

  private:
    inline KolabProxyResource* resource() const { return m_resource; }
    static Akonadi::Collection::List treeToList( const QHash<Akonadi::Collection::Id, Akonadi::Collection::List> &tree,
                                                 const Akonadi::Collection &current );

  private slots:
    void collectionsReceived( const Akonadi::Collection::List &collections );
    void collectionFetchResult( KJob* job );

  private:
    KolabProxyResource *m_resource;
    Akonadi::Collection::List m_resultCollections;
    Akonadi::Collection::List m_kolabCollections;
    QHash<Akonadi::Collection::Id, Akonadi::Collection> m_allCollections;
};

#endif
