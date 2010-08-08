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

#include "changeitemtask.h"

#include <KDE/KDebug>
#include <KDE/KLocale>

#include <kimap/appendjob.h>
#include <kimap/searchjob.h>
#include <kimap/selectjob.h>
#include <kimap/session.h>
#include <kimap/storejob.h>

#include <kmime/kmime_message.h>

#include "imapflags.h"
#include "uidnextattribute.h"

ChangeItemTask::ChangeItemTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( DeferIfNoSession, resource, parent ), m_session( 0 ), m_oldUid( 0 ), m_newUid( 0 )
{

}

ChangeItemTask::~ChangeItemTask()
{
}

void ChangeItemTask::doStart( KIMAP::Session *session )
{
  m_session = session;

  const QString mailBox = mailBoxForCollection( item().parentCollection() );
  m_oldUid = item().remoteId().toLongLong();

  if ( parts().contains( "PLD:RFC822" ) ) {
    if ( !item().hasPayload<KMime::Message::Ptr>() ) {
      changeProcessed();
      return;
    }

    // save message to the server.
    KMime::Message::Ptr msg = item().payload<KMime::Message::Ptr>();
    m_messageId = msg->messageID()->asUnicodeString().toUtf8();

    KIMAP::AppendJob *job = new KIMAP::AppendJob( session );

    job->setMailBox( mailBox );
    job->setContent( msg->encodedContent( true ) );
    job->setFlags( fromAkonadiFlags( item().flags().toList() ) );

    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( onAppendMessageDone( KJob* ) ) );

    job->start();

  } else if ( parts().contains( "FLAGS" ) ) {

    if ( session->selectedMailBox() != mailBox ) {
      KIMAP::SelectJob *select = new KIMAP::SelectJob( session );
      select->setMailBox( mailBox );

      connect( select, SIGNAL( result( KJob* ) ),
             this, SLOT( onPreStoreSelectDone( KJob* ) ) );

      select->start();

    } else {
      triggerStoreJob();
    }

  } else {
    changeProcessed();
  }
}

void ChangeItemTask::onPreStoreSelectDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    triggerStoreJob();
  }
}

void ChangeItemTask::triggerStoreJob()
{
  KIMAP::StoreJob *store = new KIMAP::StoreJob( m_session );

  store->setUidBased( true );
  store->setSequenceSet( KIMAP::ImapSet( m_oldUid ) );
  store->setFlags( fromAkonadiFlags( item().flags().toList() ) );
  store->setMode( KIMAP::StoreJob::SetFlags );

  connect( store, SIGNAL( result( KJob* ) ),
           this, SLOT( onStoreFlagsDone( KJob* ) ) );

  store->start();
}

void ChangeItemTask::onStoreFlagsDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    changeProcessed();
  }
}

void ChangeItemTask::onAppendMessageDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
    return;
  }

  KIMAP::AppendJob *append = qobject_cast<KIMAP::AppendJob*>( job );

  m_newUid = append->uid();

  // OK it's a content change, so we've to mark the old version as deleted
  // remember, you can't modify messages in IMAP mailboxes so that's really
  // add+remove all the time.

  // APPEND does not require a SELECT, so we could be anywhere right now
  if ( m_session->selectedMailBox() != append->mailBox() ) {
    KIMAP::SelectJob *select = new KIMAP::SelectJob( m_session );
    select->setMailBox( append->mailBox() );

    connect( select, SIGNAL( result( KJob* ) ),
             this, SLOT( onPreDeleteSelectDone( KJob* ) ) );

    select->start();

  } else {
    if ( m_newUid > 0 ) {
      triggerDeleteJob();
    } else {
      triggerSearchJob();
    }
  }
}

void ChangeItemTask::onPreDeleteSelectDone( KJob *job )
{
  if ( job->error() ) {
    if ( m_newUid > 0 ) {
      recordNewUid();
    } else {
      cancelTask( job->errorString() );
    }
  } else {
    if ( m_newUid > 0 ) {
      triggerDeleteJob();
    } else {
      triggerSearchJob();
    }
  }
}

void ChangeItemTask::triggerSearchJob()
{
  KIMAP::SearchJob *search = new KIMAP::SearchJob( m_session );

  search->setUidBased( true );
  search->setSearchLogic( KIMAP::SearchJob::And );

  if ( !m_messageId.isEmpty() ) {
    QByteArray header = "Message-ID ";
    header+= m_messageId;

    search->addSearchCriteria( KIMAP::SearchJob::Header, header );
  } else {
    search->addSearchCriteria( KIMAP::SearchJob::New );

    UidNextAttribute *uidNext = item().parentCollection().attribute<UidNextAttribute>();
    KIMAP::ImapInterval interval( uidNext->uidNext() );

    search->addSearchCriteria( KIMAP::SearchJob::Uid, interval.toImapSequence() );
  }

  connect( search, SIGNAL( result( KJob* ) ),
           this, SLOT( onSearchDone( KJob* ) ) );

  search->start();
}

void ChangeItemTask::onSearchDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
    return;
  }

  KIMAP::SearchJob *search = static_cast<KIMAP::SearchJob*>( job );

  if ( search->results().count()!=1 ) {
    cancelTask( i18n("Could not determine the UID for the newly created message on the server") );
    return;
  }

  m_newUid = search->results().first();
  triggerDeleteJob();
}

void ChangeItemTask::triggerDeleteJob()
{
  KIMAP::StoreJob *store = new KIMAP::StoreJob( m_session );

  store->setUidBased( true );
  store->setSequenceSet( KIMAP::ImapSet( m_oldUid ) );
  store->setFlags( QList<QByteArray>() << ImapFlags::Deleted );
  store->setMode( KIMAP::StoreJob::AppendFlags );

  connect( store, SIGNAL( result( KJob* ) ),
           this, SLOT( onDeleteDone( KJob* ) ) );

  store->start();
}

void ChangeItemTask::onDeleteDone( KJob */*job*/ )
{
  recordNewUid();
}

void ChangeItemTask::recordNewUid()
{
  Q_ASSERT(m_newUid>0);

  Akonadi::Item i = item();
  Akonadi::Collection c = i.parentCollection();

  // Get the current uid next value and store it
  UidNextAttribute *uidAttr = 0;
  int oldNextUid = 0;
  if ( c.hasAttribute( "uidnext" ) ) {
    uidAttr = static_cast<UidNextAttribute*>( c.attribute( "uidnext" ) );
    oldNextUid = uidAttr->uidNext();
  }

  // If the uid we just got back is the expected next one of the box
  // then update the property to the probable next uid to keep the cache in sync.
  // If not something happened in our back, so we don't update and a refetch will
  // happen at some point.
  if ( m_newUid==oldNextUid ) {
    if ( uidAttr==0 ) {
      uidAttr = new UidNextAttribute( m_newUid+1 );
      c.addAttribute( uidAttr );
    } else {
      uidAttr->setUidNext( m_newUid+1 );
    }

    applyCollectionChanges( c );
  }

  const QString remoteId =  QString::number( m_newUid );
  kDebug(5327) << "Setting remote ID to " << remoteId << " for item with local id " << i.id();
  i.setRemoteId( remoteId );

  changeCommitted( i );
}

#include "changeitemtask.moc"


