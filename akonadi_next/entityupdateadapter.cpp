/*
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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


#include "entityupdateadapter.h"

#include <QVariant>
#include <QStringList>

#include <akonadi/session.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectioncopyjob.h>
#include <akonadi/collectionstatisticsjob.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/itemmovejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/transactionsequence.h>
#include <akonadi/entitydisplayattribute.h>
#include "collectionchildorderattribute.h"


#include <kdebug.h>

using namespace Akonadi;

EntityUpdateAdapter::EntityUpdateAdapter( Session *s, QObject *parent )
    : QObject( parent ), m_session( s ), m_job_parent( s )
{
  m_session->setParent(this);

}

EntityUpdateAdapter::~EntityUpdateAdapter()
{
  delete m_job_parent;
}

void EntityUpdateAdapter::beginTransaction()
{
  m_job_parent = new TransactionSequence(m_session);
}

void EntityUpdateAdapter::endTransaction()
{
  m_job_parent = m_session;
}

void EntityUpdateAdapter::moveEntities( Item::List movedItems, Collection::List movedCollections, Collection src, Collection dst, int row )
{
  kDebug() << movedItems.size() << movedCollections.size() << src.remoteId() << dst.remoteId();

  // TODO: Use row to update the CollectionChildOrderAttribute correctly.

  TransactionSequence *transaction = new TransactionSequence( m_session );

  foreach( Collection col, movedCollections ) {
    // This is effectively a collectionmovejob.
    col.setParent( dst.id() );
    kDebug() << "moving" << col.remoteId();
    Akonadi::CollectionModifyJob *modifyJob = new Akonadi::CollectionModifyJob( col, transaction );
//     Akonadi::CollectionModifyJob *modifyJob = new Akonadi::CollectionModifyJob(col, m_job_parent);
  }
  foreach( Item item, movedItems ) {
    // This doesn't have any effect on the resource.
    kDebug() << "moving" << item.remoteId();
    Akonadi::ItemMoveJob *itemMoveJob = new Akonadi::ItemMoveJob( item, dst, transaction );
//     Akonadi::ItemMoveJob *itemMoveJob = new Akonadi::ItemMoveJob(item, dst, m_job_parent);
  }

  // Disable this stuff.
  if (false)
  if ( src.hasAttribute<CollectionChildOrderAttribute>() ) {
    // Update the source collection...
    CollectionChildOrderAttribute *srcCcoa = src.attribute<CollectionChildOrderAttribute>();
    QStringList srcl = srcCcoa->orderList();
    kDebug() << srcl;
    foreach( Collection col, movedCollections ) {
      srcl.removeOne( col.remoteId() );
    }
    foreach( Item item, movedItems ) {
      srcl.removeOne( item.remoteId() );
    }
    kDebug() << srcl;
    srcCcoa->setOrderList( srcl );
    src.addAttribute( srcCcoa );

    // ... And the destination collection.
    CollectionChildOrderAttribute *dstCcoa = dst.attribute<CollectionChildOrderAttribute>();
    QStringList dstl = dstCcoa->orderList();
    kDebug() << dstl;
    foreach( Collection col, movedCollections ) {
      dstl.append( col.remoteId() );
    }
    foreach( Item item, movedItems ) {
      dstl.append( item.remoteId() );
    }
    kDebug() << dstl;
    dstCcoa->setOrderList( dstl );
    dst.addAttribute( dstCcoa );

    Akonadi::CollectionModifyJob *srcModifyJob = new Akonadi::CollectionModifyJob( src, transaction );
//     Akonadi::CollectionModifyJob *srcModifyJob = new Akonadi::CollectionModifyJob( src, m_job_parent );
    if ( dst != src ) {
      Akonadi::CollectionModifyJob *dstModifyJob = new Akonadi::CollectionModifyJob( dst, transaction );
//       Akonadi::CollectionModifyJob *dstModifyJob = new Akonadi::CollectionModifyJob( dst, m_job_parent );
    }
  }
}

void EntityUpdateAdapter::removeEntities( Item::List removedItems, Collection::List removedCollections, Collection parent )
{

}

void EntityUpdateAdapter::updateEntities( Collection::List updatedCollections, Item::List updatedItems )
{

}

void EntityUpdateAdapter::updateEntities( Collection::List updatedCollections )
{

}

void EntityUpdateAdapter::updateEntities( Item::List updatedItems )
{
  foreach( Item item, updatedItems ) {
    Akonadi::ItemModifyJob *itemModifyJob = new Akonadi::ItemModifyJob( item, m_job_parent );
  }
}

void EntityUpdateAdapter::addEntities( Item::List newItems, Collection::List newCollections, Collection parent, int row )
{

}

// #include "entityupdateadapter.moc"
