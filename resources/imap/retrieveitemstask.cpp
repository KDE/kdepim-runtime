/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#include "retrieveitemstask.h"

#include "collectionflagsattribute.h"
#include "noselectattribute.h"
#include "uidvalidityattribute.h"
#include "uidnextattribute.h"

#include <akonadi/cachepolicy.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/kmime/messageparts.h>

#include <KDE/KDebug>
#include <KDE/KLocale>

#include <kimap/expungejob.h>
#include <kimap/fetchjob.h>
#include <kimap/selectjob.h>
#include <kimap/session.h>

RetrieveItemsTask::RetrieveItemsTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( CancelIfNoSession, resource, parent ), m_session( 0 )
{

}

RetrieveItemsTask::~RetrieveItemsTask()
{
}

void RetrieveItemsTask::doStart( KIMAP::Session *session )
{
  // Prevent fetching items from noselect folders.
  if ( collection().hasAttribute( "noselect" ) ) {
    NoSelectAttribute* noselect = static_cast<NoSelectAttribute*>( collection().attribute( "noselect" ) );
    if ( noselect->noSelect() ) {
      kDebug(5327) << "No Select folder";
      itemsRetrievalDone();
      return;
    }
  }

  m_session = session;

  const QString mailBox = mailBoxForCollection( collection() );

  // Now is the right time to expunge the messages marked \\Deleted from this mailbox.
  if ( isAutomaticExpungeEnabled() ) {
    if ( m_session->selectedMailBox() != mailBox ) {
      triggerPreExpungeSelect( mailBox );
    } else {
      triggerExpunge( mailBox );
    }
  } else {
    // Always select to get the stats updated
    triggerFinalSelect( mailBox );
  }
}

void RetrieveItemsTask::triggerPreExpungeSelect( const QString &mailBox )
{
  KIMAP::SelectJob *select = new KIMAP::SelectJob( m_session );
  select->setMailBox( mailBox );
  connect( select, SIGNAL( result( KJob* ) ),
           this, SLOT( onPreExpungeSelectDone( KJob* ) ) );
  select->start();
}

void RetrieveItemsTask::onPreExpungeSelectDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob*>( job );
    triggerExpunge( select->mailBox() );
  }
}

void RetrieveItemsTask::triggerExpunge( const QString &mailBox )
{
  kDebug(5327) << mailBox;

  KIMAP::ExpungeJob *expunge = new KIMAP::ExpungeJob( m_session );
  connect( expunge, SIGNAL( result( KJob* ) ),
           this, SLOT( onExpungeDone( KJob* ) ) );
  expunge->start();
}

void RetrieveItemsTask::onExpungeDone( KJob */*job*/ )
{
  // We can ignore the error IMO, we just had a wrong expunge so some old messages will just reappear

  // We have to re-select the mailbox to update all the stats after the expunge
  // (the EXPUNGE command doesn't return enough for our needs)
  triggerFinalSelect( m_session->selectedMailBox() );
}

void RetrieveItemsTask::triggerFinalSelect( const QString &mailBox )
{
  KIMAP::SelectJob *select = new KIMAP::SelectJob( m_session );
  select->setMailBox( mailBox );
  connect( select, SIGNAL( result( KJob* ) ),
           this, SLOT( onFinalSelectDone( KJob* ) ) );
  select->start();
}

void RetrieveItemsTask::onFinalSelectDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
    return;
  }

  KIMAP::SelectJob *select = qobject_cast<KIMAP::SelectJob*>( job );

  const QString mailBox = select->mailBox();
  const int messageCount = select->messageCount();
  const qint64 uidValidity = select->uidValidity();
  const qint64 nextUid = select->nextUid();
  const QList<QByteArray> flags = select->flags();

  // uidvalidity can change between sessions, we don't want to refetch
  // folders in that case. Keep track of what is processed and what not.
  static QStringList processed;
  bool firstTime = false;
  if ( processed.indexOf( mailBox ) == -1 ) {
    firstTime = true;
    processed.append( mailBox );
  }

  Akonadi::Collection col = collection();
  bool modifyNeeded = false;

  // Get the current uid validity value and store it
  int oldUidValidity = 0;
  if ( !col.hasAttribute( "uidvalidity" ) ) {
    UidValidityAttribute* currentUidValidity  = new UidValidityAttribute( uidValidity );
    col.addAttribute( currentUidValidity );
    modifyNeeded = true;
  } else {
    UidValidityAttribute* currentUidValidity =
      static_cast<UidValidityAttribute*>( col.attribute( "uidvalidity" ) );
    oldUidValidity = currentUidValidity->uidValidity();
    if ( oldUidValidity != uidValidity ) {
      currentUidValidity->setUidValidity( uidValidity );
      modifyNeeded = true;
    }
  }

  // Get the current uid next value and store it
  int oldNextUid = 0;
  if ( !col.hasAttribute( "uidnext" ) ) {
    UidNextAttribute* currentNextUid  = new UidNextAttribute( nextUid );
    col.addAttribute( currentNextUid );
    modifyNeeded = true;
  } else {
    UidNextAttribute* currentNextUid =
      static_cast<UidNextAttribute*>( col.attribute( "uidnext" ) );
    oldNextUid = currentNextUid->uidNext();
    if ( oldNextUid != nextUid ) {
      currentNextUid->setUidNext( nextUid );
      modifyNeeded = true;
    }
  }

  // Store the mailbox flags
  if ( !col.hasAttribute( "collectionflags" ) ) {
    Akonadi::CollectionFlagsAttribute *flagsAttribute  = new Akonadi::CollectionFlagsAttribute( flags );
    col.addAttribute( flagsAttribute );
    modifyNeeded = true;
  } else {
    Akonadi::CollectionFlagsAttribute *flagsAttribute =
      static_cast<Akonadi::CollectionFlagsAttribute*>( col.attribute( "collectionflags" ) );
    const QList<QByteArray> oldFlags = flagsAttribute->flags();
    if ( oldFlags != flags ) {
      flagsAttribute->setFlags( flags );
      modifyNeeded = true;
    }
  }

  if ( modifyNeeded )
    applyCollectionChanges( col );

  KIMAP::FetchJob::FetchScope scope;
  scope.parts.clear();
  scope.mode = KIMAP::FetchJob::FetchScope::Headers;

  if ( col.cachePolicy()
       .localParts().contains( Akonadi::MessagePart::Body ) ) {
    scope.mode = KIMAP::FetchJob::FetchScope::Full;
  }

  const qint64 realMessageCount = col.statistics().count();

  // First check the uidvalidity, if this has changed, it means the folder
  // has been deleted and recreated. So we wipe out the messages and
  // retrieve all.
  if ( oldUidValidity != uidValidity && !firstTime
    && oldUidValidity != 0 ) {
    kDebug(5327) << "UIDVALIDITY check failed (" << oldUidValidity << "|"
                 << uidValidity <<") refetching "<< mailBox;

    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_session );
    fetch->setSequenceSet( KIMAP::ImapSet( 1, messageCount ) );
    fetch->setScope( scope );
    connect( fetch, SIGNAL( headersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                             QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ),
             this, SLOT( onHeadersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                            QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onHeadersFetchDone( KJob* ) ) );
    fetch->start();
  } else if( messageCount > realMessageCount ) {
    // The amount on the server is bigger than that we have in the cache
    // that probably means that there is new mail. Fetch missing.
    kDebug(5327) << "Fetch missing: " << messageCount << " But: " << realMessageCount;

    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_session );
    fetch->setSequenceSet( KIMAP::ImapSet( realMessageCount+1, messageCount ) );
    fetch->setScope( scope );
    connect( fetch, SIGNAL( headersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                             QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ),
             this, SLOT( onHeadersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                            QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onHeadersFetchDone( KJob* ) ) );
    fetch->start();
  } else if ( messageCount == realMessageCount && oldNextUid != nextUid
           && oldNextUid != 0 && !firstTime ) {
    // amount is right but uidnext is different.... something happened
    // behind our back...
    kDebug(5327) << "UIDNEXT check failed, refetching mailbox";

    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_session );
    fetch->setSequenceSet( KIMAP::ImapSet( 1, messageCount ) );
    fetch->setScope( scope );
    connect( fetch, SIGNAL( headersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                             QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ),
             this, SLOT( onHeadersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                            QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onHeadersFetchDone( KJob* ) ) );
    fetch->start();
  } else {
    kDebug(5327) << "All fine, asking for all message flags looking for changes";
    listFlagsForImapSet( KIMAP::ImapSet( 1, messageCount ) );
  }
}

void RetrieveItemsTask::listFlagsForImapSet( const KIMAP::ImapSet& set )
{
  KIMAP::FetchJob::FetchScope scope;
  scope.parts.clear();
  scope.mode = KIMAP::FetchJob::FetchScope::Flags;

  KIMAP::FetchJob* fetch = new KIMAP::FetchJob( m_session );
  fetch->setSequenceSet( set );
  fetch->setScope( scope );
  connect( fetch, SIGNAL( headersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                           QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ),
           this, SLOT( onFlagsReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                        QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ) );
  connect( fetch, SIGNAL( result( KJob* ) ),
           this, SLOT( onFlagsFetchDone( KJob* ) ) );
  fetch->start();
}

void RetrieveItemsTask::onHeadersReceived( const QString &mailBox, const QMap<qint64, qint64> &uids,
                                           const QMap<qint64, qint64> &sizes,
                                           const QMap<qint64, KIMAP::MessageFlags> &flags,
                                           const QMap<qint64, KIMAP::MessagePtr> &messages )
{
  Q_UNUSED( mailBox );

  Akonadi::Item::List addedItems;

  foreach ( qint64 number, uids.keys() ) {
    Akonadi::Item i;
    i.setRemoteId( QString::number( uids[number] ) );
    i.setMimeType( "message/rfc822" );
    i.setPayload( KMime::Message::Ptr( messages[number] ) );
    i.setSize( sizes[number] );

    foreach( const QByteArray &flag, flags[number] ) {
      i.setFlag( flag );
    }
    //kDebug(5327) << "Flags: " << i.flags();
    addedItems << i;
  }

  itemsRetrieved( addedItems );
}

void RetrieveItemsTask::onHeadersFetchDone( KJob *job )
{
  if ( job->error() ) {
      cancelTask( job->errorString() );
      return;
  }

  KIMAP::FetchJob *fetch = static_cast<KIMAP::FetchJob*>( job );
  KIMAP::ImapSet alreadyFetched = fetch->sequenceSet();

  // If this is the first fetch of a folder, skip getting flags, we
  // already have them all from the previous full fetch. This is not
  // just an optimization, as incremental retrieval assumes nothing
  // will be listed twice.
  if ( alreadyFetched.intervals().first().begin() <= 1 ) {
    itemsRetrievalDone();
    return;
  }

  KIMAP::ImapSet set( 1, alreadyFetched.intervals().first().begin()-1 );
  listFlagsForImapSet( set );
}

void RetrieveItemsTask::onFlagsReceived( const QString &mailBox, const QMap<qint64, qint64> &uids,
                                         const QMap<qint64, qint64> &sizes,
                                         const QMap<qint64, KIMAP::MessageFlags> &flags,
                                         const QMap<qint64, KIMAP::MessagePtr> &messages )
{
  Q_UNUSED( mailBox );
  Q_UNUSED( sizes );
  Q_UNUSED( messages );

  Akonadi::Item::List changedItems;

  foreach ( qint64 number, uids.keys() ) {
    Akonadi::Item i;
    i.setRemoteId( QString::number( uids[number] ) );
    i.setMimeType( "message/rfc822" );
    i.setFlags( Akonadi::Item::Flags::fromList( toAkonadiFlags( flags[number] ) ) );

    //kDebug(5327) << "Flags: " << i.flags();
    changedItems << i;
  }

  itemsRetrieved( changedItems );
}

void RetrieveItemsTask::onFlagsFetchDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    itemsRetrievalDone();
  }
}

#include "retrieveitemstask.moc"


