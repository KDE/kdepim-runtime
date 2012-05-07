/*
 *  Copyright (c) 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#include "upgradejob.h"

#include "kolabdefs.h"
#include "kolabhandler.h"

#include "collectionannotationsattribute.h" //from shared

#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>

#define IMAP_COLLECTION "IMAP_COLLECTION"
#define FOLDER_TYPE "FOLDER_TYPE"

UpgradeJob::UpgradeJob( Kolab::Version targetVersion,
                        const Akonadi::AgentInstance &instance,
                        QObject *parent )
  : Akonadi::Job( parent ),
    m_agentInstance( instance ),
    m_targetVersion( targetVersion )
{
  kDebug() << targetVersion;
}

void UpgradeJob::doStart()
{
  kDebug();
  //Get all subdirectories of kolab resource
  Akonadi::CollectionFetchJob *job =
    new Akonadi::CollectionFetchJob( Akonadi::Collection::root(),
                                     Akonadi::CollectionFetchJob::Recursive, this );
  job->fetchScope().setResource( m_agentInstance.identifier() );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(collectionFetchResult(KJob*)) );
}

void UpgradeJob::collectionFetchResult( KJob *job )
{
  kDebug();
  if ( job->error() ) {
    kDebug() << job->errorString();
    emitResult();
    return;
  }
  int collCount = 0;
  Akonadi::Collection::List collections =
    static_cast<Akonadi::CollectionFetchJob*>( job )->collections();
  foreach ( const Akonadi::Collection &col, collections ) {
    //FIXME:
    // find a way to properly detect shared folders.
    // Collection::CanCreateCollection never applies to shared folders,
    // but that's just a workaround.
    if ( !( col.rights() & Akonadi::Collection::CanCreateCollection ) ||
         !( col.rights() & Akonadi::Collection::CanChangeItem ) ) {
      kDebug() << "skipping shared/non-editable folder";
      continue;
    }
    KolabV2::FolderType folderType = KolabV2::Mail;
    Akonadi::CollectionAnnotationsAttribute *attr = 0;
    if ( ( attr = col.attribute<Akonadi::CollectionAnnotationsAttribute>() ) ) {
      folderType =
        KolabV2::folderTypeFromString(
          attr->annotations().value( KOLAB_FOLDER_TYPE_ANNOTATION ) );
    }
    if ( folderType == KolabV2::Mail ) {
      //kWarning() << "Wrong folder annotation "
      //           << "(this should never happen, the annotation is probably not available)";
      continue;
    }

    kDebug() << "upgrading " << col.id();
    collCount++;
    Akonadi::ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob( col, this );
    itemFetchJob->fetchScope().fetchFullPayload( true );
    itemFetchJob->fetchScope().setCacheOnly( false );
    itemFetchJob->setProperty( IMAP_COLLECTION, QVariant::fromValue( col ) );
    itemFetchJob->setProperty( FOLDER_TYPE, QVariant::fromValue( static_cast<int>(folderType) ) );
    connect( itemFetchJob, SIGNAL(result(KJob*)), this, SLOT(itemFetchResult(KJob*)) );
  }
  //Percent is only emitted when Bytes is the unit
  setTotalAmount( Bytes, collCount );
  if ( !collCount ) {
    emitResult();
  }
}

void UpgradeJob::itemFetchResult( KJob *job )
{
  if ( job->error() ) {
    kDebug() << job->errorString();
    checkResult();
    return; // Akonadi::Job propagates that automatically
  }
  Akonadi::ItemFetchJob *j = static_cast<Akonadi::ItemFetchJob*>( job );
  if ( j->items().isEmpty() ) {
    qWarning() << "no items fetched ";
    checkResult();
    return;
  }

  const Akonadi::Collection imapCollection =
    j->property( IMAP_COLLECTION ).value<Akonadi::Collection>();
  if ( !imapCollection.isValid() ) {
    qWarning() << "invalid imap collection";
    checkResult();
    return;
  }

  KolabV2::FolderType folderType =
    static_cast<KolabV2::FolderType>( j->property(FOLDER_TYPE).toInt() );

  KolabHandler::Ptr handler = KolabHandler::createHandler( folderType, imapCollection );

  if ( !handler ) {
    qWarning() << "invalid handler";
    checkResult();
    return;
  }
  handler->setKolabFormatVersion( m_targetVersion );

  foreach ( Akonadi::Item imapItem, j->items() ) { //krazy:exclude=foreach
    if ( !imapItem.isValid() ) {
      qWarning() << "invalid item";
      continue;
    }
    kDebug() << "updating item " << imapItem.id();
    const Akonadi::Item::List &translatedItems =
      handler->translateItems( Akonadi::Item::List() << imapItem );

    if ( translatedItems.size() != 1 ) {
      qWarning() << "failed to translateItems" << translatedItems.size();
      continue;
    }
    handler->toKolabFormat( translatedItems.first(), imapItem );
    Akonadi::ItemModifyJob *modJob = new Akonadi::ItemModifyJob( imapItem, this );
    connect( modJob, SIGNAL(result(KJob*)), this, SLOT(itemModifyResult(KJob*)) );
  }
  checkResult();
}

void UpgradeJob::checkResult()
{
  setProcessedAmount( Bytes, processedAmount( Bytes ) + 1 );
  if ( processedAmount( Bytes ) >= totalAmount( Bytes ) ) {
    emitResult();
  }
}

void UpgradeJob::itemModifyResult( KJob *job )
{
  if ( job->error() ) {
    kDebug() << job->errorString();
    return; // Akonadi::Job propagates that automatically
  }
  kDebug() << "modjob done";
}

