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

#include "collectiontreebuilder.h"

#include <Akonadi/CollectionFetchJob>

CollectionTreeBuilder::CollectionTreeBuilder( KolabProxyResource *parent )
  : Job( parent ),
    m_resource( parent )
{
}

void CollectionTreeBuilder::doStart()
{
  Akonadi::CollectionFetchJob *job =
    new Akonadi::CollectionFetchJob( Akonadi::Collection::root(),
                                     Akonadi::CollectionFetchJob::Recursive, this );

  connect( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
           SLOT(collectionsReceived(Akonadi::Collection::List)) );

  connect( job, SIGNAL(result(KJob*)), SLOT(collectionFetchResult(KJob*)) );
}

void CollectionTreeBuilder::collectionsReceived( const Akonadi::Collection::List &collections )
{
  foreach ( const Akonadi::Collection &collection, collections ) {
    if ( collection.resource() == resource()->identifier() ) {
      continue;
    }
    if ( resource()->registerHandlerForCollection( collection ) ) {
      m_kolabCollections.append( collection );
    }
    m_allCollections.insert( collection.id(), collection );
  }
}

Akonadi::Collection::List CollectionTreeBuilder::allCollections() const
{
  return m_resultCollections;
}

Akonadi::Collection::List CollectionTreeBuilder::treeToList(
  const QHash< Akonadi::Entity::Id, Akonadi::Collection::List > &tree,
  const Akonadi::Collection &current )
{
  Akonadi::Collection::List rv;
  foreach ( const Akonadi::Collection &child, tree.value( current.id() ) ) {
    rv += child;
    rv += treeToList( tree, child );
  }
  return rv;
}

void CollectionTreeBuilder::collectionFetchResult( KJob *job )
{
  Q_UNUSED( job );
  m_resultCollections.clear();

  // step 1: build the minimal sub-tree that contains all Kolab collections
  QHash<Akonadi::Collection::Id, Akonadi::Collection::List> remainingTree;
  foreach ( const Akonadi::Collection &kolabCollection, m_kolabCollections ) {
    Akonadi::Collection child = kolabCollection;
    Akonadi::Collection::Id parentId = child.parentCollection().id();
    while ( child.isValid() && !remainingTree.value( parentId ).contains( child ) ) {
      remainingTree[ parentId ].append( child );
      child = m_allCollections.value( parentId );
      parentId = child.parentCollection().id();
    }
  }

  // step 2: path contraction
  // TODO

  // step 3: flatten the tree and adjust root node
  Akonadi::Collection::List collections = remainingTree.value( Akonadi::Collection::root().id() );
  foreach ( Akonadi::Collection topLevel, collections ) { //krazy:exclude=foreach
    topLevel.setParentCollection( Akonadi::Collection::root() );
    m_resultCollections.append( topLevel );
    m_resultCollections += treeToList( remainingTree, topLevel );
  }

  emitResult(); // error handling already done by Akonadi::Job
}

#include "collectiontreebuilder.moc"
