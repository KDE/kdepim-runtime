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

#include "retrieveitemtask.h"

#include <KDE/KDebug>
#include <KDE/KLocale>

#include <kimap/selectjob.h>
#include <kimap/session.h>

RetrieveItemTask::RetrieveItemTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( resource, parent ), m_session( 0 ), m_uid( 0 ), m_messageReceived( false )
{

}

RetrieveItemTask::~RetrieveItemTask()
{
}

void RetrieveItemTask::doStart( KIMAP::Session *session )
{
  m_session = session;

  const QString mailBox = mailBoxForCollection( item().parentCollection() );
  m_uid = item().remoteId().toLongLong();

  if ( session->selectedMailBox() != mailBox ) {
    KIMAP::SelectJob *select = new KIMAP::SelectJob( m_session );
    select->setMailBox( mailBox );
    connect( select, SIGNAL( result( KJob* ) ),
             this, SLOT( onSelectDone( KJob* ) ) );
    select->start();
  } else {
    triggerFetchJob();
  }
}

void RetrieveItemTask::onSelectDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    triggerFetchJob();
  }
}

void RetrieveItemTask::triggerFetchJob()
{
  KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_session );
  KIMAP::FetchJob::FetchScope scope;
  fetch->setUidBased( true );
  fetch->setSequenceSet( KIMAP::ImapSet( m_uid ) );
  scope.parts.clear();// = parts.toList();
  scope.mode = KIMAP::FetchJob::FetchScope::Content;
  fetch->setScope( scope );
  connect( fetch, SIGNAL( messagesReceived( QString, QMap<qint64, qint64>, QMap<qint64, KIMAP::MessagePtr> ) ),
           this, SLOT( onMessagesReceived( QString, QMap<qint64, qint64>, QMap<qint64, KIMAP::MessagePtr> ) ) );
  //TODO: Handle parts retrieval
  //connect( fetch, SIGNAL( partsReceived( QString, QMap<qint64, qint64>, QMap<qint64, KIMAP::MessageParts> ) ),
  //         this, SLOT( onPartsReceived( QString, QMap<qint64, qint64>, QMap<qint64, KIMAP::MessageParts> ) ) );
  connect( fetch, SIGNAL( result( KJob* ) ),
           this, SLOT( onContentFetchDone( KJob* ) ) );
  fetch->start();
}

void RetrieveItemTask::onMessagesReceived( const QString &mailBox, const QMap<qint64, qint64> &uids,
                                           const QMap<qint64, KIMAP::MessagePtr> &messages )
{
  Q_UNUSED( mailBox );

  KIMAP::FetchJob *fetch = qobject_cast<KIMAP::FetchJob*>( sender() );
  Q_ASSERT( fetch!=0 );
  Q_ASSERT( uids.size()==1 );
  Q_ASSERT( messages.size()==1 );

  Akonadi::Item i = item();

  kDebug(5327) << "MESSAGE from Imap server" << item().remoteId();
  Q_ASSERT( item().isValid() );

  KIMAP::MessagePtr message = messages[messages.keys().first()];

  i.setMimeType( "message/rfc822" );
  i.setPayload( KMime::Message::Ptr( message ) );

  kDebug(5327) << "Has Payload: " << i.hasPayload();
  kDebug(5327) << message->head().isEmpty() << message->body().isEmpty() << message->contents().isEmpty() << message->hasContent() << message->hasHeader("Message-ID");

  m_messageReceived = true;
  itemRetrieved( i );
}

void RetrieveItemTask::onContentFetchDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else if ( !m_messageReceived ) {
    cancelTask( i18n("No message retrieved, server reply was empty.") );
  }
}


#include "retrieveitemtask.moc"


