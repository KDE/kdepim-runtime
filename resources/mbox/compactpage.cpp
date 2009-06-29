/*
    Copyright (c) 2009 Bertjan Broeksema <b.broeksema@kdemail.net>

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

#include "compactpage.h"

#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <libmbox/mbox.h>

#include "deleteditemsattribute.h"

using namespace Akonadi;

CompactPage::CompactPage( const QString &collectionId, QWidget *parent )
  : QWidget( parent )
  , mCollectionId( collectionId )
{
  ui.setupUi( this );

  connect( ui.compactButton, SIGNAL( clicked() ), this, SLOT( compact() ) );

  checkCollectionId();
}

void CompactPage::checkCollectionId()
{
  if ( !mCollectionId.isEmpty() ) {
    Collection collection;
    collection.setRemoteId( mCollectionId );
    CollectionFetchJob *fetchJob =
        new CollectionFetchJob( collection, CollectionFetchJob::Base );

    connect( fetchJob, SIGNAL( result( KJob* ) ),
             this, SLOT( onCollectionFetchCheck( KJob* ) ) );
  }
}

void CompactPage::compact()
{
  ui.compactButton->setEnabled( false );

  Collection collection;
  collection.setRemoteId( mCollectionId );
  CollectionFetchJob *fetchJob =
      new CollectionFetchJob( collection, CollectionFetchJob::Base );

  connect( fetchJob, SIGNAL( result( KJob* ) ),
           this, SLOT( onCollectionFetchCompact( KJob* ) ) );
}

void CompactPage::onCollectionFetchCheck( KJob *job )
{
  if ( job->error() ) {
    // If we cannot fetch the collection, than also disable compacting.
    ui.compactButton->setEnabled( false );
    return;
  }

  CollectionFetchJob *fetchJob = dynamic_cast<CollectionFetchJob*>( job );
  Q_ASSERT( fetchJob );
  Q_ASSERT( fetchJob->collections().size() == 1 );

  Collection mboxCollection = fetchJob->collections().first();
  DeletedItemsAttribute *attr
    = mboxCollection.attribute<DeletedItemsAttribute>( Akonadi::Entity::AddIfMissing );

  if ( attr->deletedItemOffsets().size() > 0 ) {
    ui.compactButton->setEnabled( true );
    ui.messageLabel->setText( i18np("(1 message marked for deletion)",
     "(%1 messages marked for deletion)", attr->deletedItemOffsets().size() ) );
  }
}

void CompactPage::onCollectionFetchCompact( KJob *job )
{
  if ( job->error() ) {
    ui.messageLabel->setText( i18n( "Failed to fetch the collection." ) );
    ui.compactButton->setEnabled( true );
    return;
  }

  CollectionFetchJob *fetchJob = dynamic_cast<CollectionFetchJob*>( job );
  Q_ASSERT( fetchJob );
  Q_ASSERT( fetchJob->collections().size() == 1 );

  Collection mboxCollection = fetchJob->collections().first();
  DeletedItemsAttribute *attr
    = mboxCollection.attribute<DeletedItemsAttribute>( Akonadi::Entity::AddIfMissing );

  MBox mbox;
  // TODO: Set lock method.
  if ( !mbox.load( KUrl( mCollectionId ).path() ) ) {
    ui.messageLabel->setText( i18n( "Failed to load the mbox file" ) );
  } else {
    ui.messageLabel->setText( i18np( "(Deleting 1 message)",
      "(Deleting %1 messages)", attr->deletedItemOffsets().size() ) );
    // TODO: implement and connect to messageProcessed signal.
    if ( mbox.purge( attr->deletedItemOffsets() ) ) {
      mboxCollection.removeAttribute<DeletedItemsAttribute>();
      CollectionModifyJob *modifyJob = new CollectionModifyJob( mboxCollection );
      connect( modifyJob, SIGNAL( result( KJob * ) ),
               this, SLOT( onCollectionModify( KJob *) ) );
    } else
      ui.messageLabel->setText( i18n( "Failed to compact the mbox file." ) );
  }
}

void CompactPage::onCollectionModify( KJob *job )
{
  ui.messageLabel->setText( i18n( "MBox file compacted." ) );
}

#include "compactpage.moc"
