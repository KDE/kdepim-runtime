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

#include <collectionannotationsattribute.h>

#include <akonadi/collectionfetchjob.h>

using namespace Akonadi;

CollectionTreeBuilder::CollectionTreeBuilder(KolabProxyResource* parent) :
  Job( parent ),
  m_resource( parent )
{
}

void CollectionTreeBuilder::doStart()
{
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, this );
  connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)), SLOT(collectionsReceived(Akonadi::Collection::List)) );
  connect(job, SIGNAL(result(KJob*)), SLOT(collectionFetchResult(KJob*)) );
}

void CollectionTreeBuilder::collectionsReceived(const Akonadi::Collection::List& collections)
{
  foreach( const Collection &collection, collections ) {
    if ( collection.resource() == resource()->identifier() )
      continue;
    if ( resource()->registerHandlerForCollection( collection ) ) {
      m_kolabCollections.append( collection );
    }
    m_allCollections.insert( collection.id(), collection );
  }
}

Collection::List CollectionTreeBuilder::allCollections() const
{
  return m_resultCollections;
}

Collection::List CollectionTreeBuilder::treeToList( const QHash< Entity::Id, Collection::List > &tree,
                                                    const Akonadi::Collection& current )
{
  Collection::List rv;
  foreach ( const Collection &child, tree.value( current.id() ) ) {
    rv += child;
    rv += treeToList( tree, child );
  }
  return rv;
}

void CollectionTreeBuilder::collectionFetchResult(KJob* job)
{
  m_resultCollections.clear();

  // step 1: build the minimal sub-tree that contains all Kolab collections
  QHash<Collection::Id, Collection::List> remainingTree;
  foreach ( const Collection kolabCollection, m_kolabCollections ) {
    Collection child = kolabCollection;
    Collection::Id parentId = child.parent();
    while ( child.isValid() && !remainingTree.value( parentId ).contains( child ) ) {
      remainingTree[ parentId ].append( child );
      child = m_allCollections.value( parentId );
      parentId = child.parent();
    }
  }

  // step 2: path contraction
  // TODO

  // step 3: flatten the tree and adjust root node
  foreach ( Collection topLevel, remainingTree.value( Collection::root().id() ) ) {
    topLevel.setParent( resource()->root() );
    m_resultCollections.append( topLevel );
    m_resultCollections += treeToList( remainingTree, topLevel );
  }

  emitResult(); // error handling already done by Akonadi::Job
}

#include "collectiontreebuilder.moc"
