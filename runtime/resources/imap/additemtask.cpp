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

#include "additemtask.h"

#include <KDE/KDebug>
#include <KDE/KLocale>

#include <kimap/appendjob.h>
#include <kimap/session.h>

#include <kmime/kmime_message.h>

#include "uidnextattribute.h"

AddItemTask::AddItemTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( DeferIfNoSession, resource, parent )
{

}

AddItemTask::~AddItemTask()
{
}

void AddItemTask::doStart( KIMAP::Session *session )
{
  if ( !item().hasPayload<KMime::Message::Ptr>() ) {
    changeProcessed();
    return;
  }

  const QString mailBox = mailBoxForCollection( collection() );

  kDebug(5327) << "Got notification about item added for local id " << item().id() << " and remote id " << item().remoteId();

  // save message to the server.
  KMime::Message::Ptr msg = item().payload<KMime::Message::Ptr>();

  KIMAP::AppendJob *job = new KIMAP::AppendJob( session );
  job->setMailBox( mailBox );
  job->setContent( msg->encodedContent( true ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onAppendMessageDone( KJob* ) ) );
  job->start();
}


void AddItemTask::onAppendMessageDone( KJob *job )
{
  KIMAP::AppendJob *append = qobject_cast<KIMAP::AppendJob*>( job );

  if ( append->error() ) {
    cancelTask( append->errorString() );
    return;
  }

  Akonadi::Item i = item();

  qint64 uid = append->uid();
  Q_ASSERT( uid > 0 );

  const QString remoteId =  QString::number( uid );
  kDebug(5327) << "Setting remote ID to " << remoteId << " for item with local id " << i.id();
  i.setRemoteId( remoteId );

  changeCommitted( i );

  Akonadi::Collection c = collection();

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
  if ( uid==oldNextUid ) {
    if ( uidAttr==0 ) {
      uidAttr = new UidNextAttribute( uid+1 );
      c.addAttribute( uidAttr );
    } else {
      uidAttr->setUidNext( uid+1 );
    }

    applyCollectionChanges( c );
  }
}

#include "additemtask.moc"


