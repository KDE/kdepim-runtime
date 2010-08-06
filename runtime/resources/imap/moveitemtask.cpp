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

#include "moveitemtask.h"

#include <KDE/KDebug>
#include <KDE/KLocale>

#include <kimap/copyjob.h>
#include <kimap/searchjob.h>
#include <kimap/selectjob.h>
#include <kimap/session.h>
#include <kimap/storejob.h>

#include <kmime/kmime_message.h>

#include "imapflags.h"
#include "uidnextattribute.h"

MoveItemTask::MoveItemTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( DeferIfNoSession, resource, parent ), m_newUid( 0 )
{

}

MoveItemTask::~MoveItemTask()
{
}

void MoveItemTask::doStart( KIMAP::Session *session )
{
  if ( item().remoteId().isEmpty() ) {
    emitError( i18n( "Cannot move message, it does not exist on the server." ) );
    changeProcessed();
    return;
  }

  if ( sourceCollection().remoteId().isEmpty() ) {
    emitError( i18n( "Cannot move message out of '%1', '%1' does not exist on the server.",
                     sourceCollection().name() ) );
    changeProcessed();
    return;
  }

  if ( targetCollection().remoteId().isEmpty() ) {
    emitError( i18n( "Cannot move message to '%1', '%1' does not exist on the server.",
                     targetCollection().name() ) );
    changeProcessed();
    return;
  }

  const QString oldMailBox = mailBoxForCollection( sourceCollection() );
  const QString newMailBox = mailBoxForCollection( targetCollection() );

  if ( oldMailBox == newMailBox ) {
    changeProcessed();
    return;
  }

  if ( session->selectedMailBox() != oldMailBox ) {
    KIMAP::SelectJob *select = new KIMAP::SelectJob( session );

    select->setMailBox( oldMailBox );
    connect( select, SIGNAL( result( KJob* ) ),
             this, SLOT( onSelectDone( KJob* ) ) );

    select->start();
  } else {
    triggerCopyJob( session );
  }
}

void MoveItemTask::onSelectDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );

  } else {
    KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob*>( job );
    triggerCopyJob( select->session() );
  }
}

void MoveItemTask::triggerCopyJob( KIMAP::Session *session )
{
  const qint64 uid = item().remoteId().toLongLong();
  const QString newMailBox = mailBoxForCollection( targetCollection() );

  // save message id, might be needed later to search for the
  // resulting message uid.
  KMime::Message::Ptr msg = item().payload<KMime::Message::Ptr>();
  m_messageId = msg->messageID()->asUnicodeString().toUtf8();

  KIMAP::CopyJob *copy = new KIMAP::CopyJob( session );

  copy->setUidBased( true );
  copy->setSequenceSet( KIMAP::ImapSet( uid ) );
  copy->setMailBox( newMailBox );

  connect( copy, SIGNAL( result( KJob* ) ),
           this, SLOT( onCopyDone( KJob* ) ) );

  copy->start();
}

void MoveItemTask::onCopyDone( KJob *job )
{
  if ( job->error()  ) {
    cancelTask( job->errorString() );

  } else {
    KIMAP::CopyJob *copy = static_cast<KIMAP::CopyJob*>( job );

    const qint64 oldUid = item().remoteId().toLongLong();

    if ( !copy->resultingUids().isEmpty() ) {
      // Go ahead, UIDPLUS seems to be supported and we copied a single message
      m_newUid = copy->resultingUids().intervals().first().begin();
    }

    // Mark the old one ready for deletion
    KIMAP::StoreJob *store = new KIMAP::StoreJob( copy->session() );

    store->setUidBased( true );
    store->setSequenceSet( KIMAP::ImapSet( oldUid ) );
    store->setFlags( QList<QByteArray>() << ImapFlags::Deleted );
    store->setMode( KIMAP::StoreJob::AppendFlags );

    connect( store, SIGNAL( result( KJob* ) ),
             this,  SLOT( onStoreFlagsDone( KJob* ) ) );

    store->start();
  }
}

void MoveItemTask::onStoreFlagsDone( KJob *job )
{
  if ( job->error() ) {
    emitWarning( i18n( "Failed to mark the message from '%1' for deletion on the IMAP server. "
                       "It will reappear on next sync.",
                       sourceCollection().name() ) );
  }

  if ( m_newUid ) {
    recordNewUid();
  } else {
    // Let's go for a search to find the new UID :-)

    // We did a copy we're very likely not in the right mailbox
    KIMAP::StoreJob *store = static_cast<KIMAP::StoreJob*>( job );
    KIMAP::SelectJob *select = new KIMAP::SelectJob( store->session() );
    select->setMailBox( mailBoxForCollection( targetCollection() ) );

    connect( select, SIGNAL( result( KJob* ) ),
             this, SLOT( onPreSearchSelectDone( KJob* ) ) );

    select->start();
  }
}

void MoveItemTask::onPreSearchSelectDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
    return;
  }

  KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob*>( job );
  KIMAP::SearchJob *search = new KIMAP::SearchJob( select->session() );

  search->setUidBased( true );
  search->setSearchLogic( KIMAP::SearchJob::And );

  if ( !m_messageId.isEmpty() ) {
    QByteArray header = "Message-ID ";
    header+= m_messageId;

    search->addSearchCriteria( KIMAP::SearchJob::Header, header );
  } else {
    search->addSearchCriteria( KIMAP::SearchJob::New );

    UidNextAttribute *uidNext = targetCollection().attribute<UidNextAttribute>();
    KIMAP::ImapInterval interval( uidNext->uidNext() );

    search->addSearchCriteria( KIMAP::SearchJob::Uid, interval.toImapSequence() );
  }

  connect( search, SIGNAL( result( KJob* ) ),
           this, SLOT( onSearchDone( KJob* ) ) );

  search->start();
}

void MoveItemTask::onSearchDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
    return;
  }

  KIMAP::SearchJob *search = static_cast<KIMAP::SearchJob*>( job );

  if ( search->results().count()!=1 ) {
    cancelTask( i18n("Couldn't determine the UID for the newly created message on the server") );
    return;
  }

  m_newUid = search->results().first();
  recordNewUid();
}

void MoveItemTask::recordNewUid()
{
  Q_ASSERT(m_newUid>0);

  // Create the item resulting of the operation, since at that point
  // the first part of the move succeeded
  Akonadi::Item i = item();

  // Update the item content with the new UID from the copy
  i.setRemoteId( QString::number( m_newUid ) );

  changeCommitted( i );
}

#include "moveitemtask.moc"


