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
#include <kimap/selectjob.h>
#include <kimap/session.h>
#include <kimap/storejob.h>

MoveItemTask::MoveItemTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( resource, parent )
{

}

MoveItemTask::~MoveItemTask()
{
}

void MoveItemTask::doStart( KIMAP::Session *session )
{
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
    Q_ASSERT( !copy->resultingUids().isEmpty() );

    const qint64 oldUid = item().remoteId().toLongLong();

    // Create the item resulting of the operation, since at that point
    // the first part of the move succeeded
    m_item = item();

    // Go ahead, UIDPLUS is supposed to be supported and we copied a single message
    const qint64 newUid = copy->resultingUids().intervals().first().begin();

    // Update the item content with the new UID from the copy
    m_item.setRemoteId( QString::number( newUid ) );

    // Mark the old one ready for deletion
    KIMAP::StoreJob *store = new KIMAP::StoreJob( copy->session() );

    store->setUidBased( true );
    store->setSequenceSet( KIMAP::ImapSet( oldUid ) );
    store->setFlags( QList<QByteArray>() << "\\Deleted" );
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

  changeCommitted( m_item );
}

#include "moveitemtask.moc"


