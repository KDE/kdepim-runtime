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

#include "movecollectiontask.h"

#include <KDE/KDebug>
#include <KDE/KLocale>

#include <kimap/renamejob.h>
#include <kimap/session.h>
#include <kimap/subscribejob.h>

MoveCollectionTask::MoveCollectionTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( DeferIfNoSession, resource, parent )
{

}

MoveCollectionTask::~MoveCollectionTask()
{
}

void MoveCollectionTask::doStart( KIMAP::Session *session )
{
  if ( collection().remoteId().isEmpty() ) {
    emitError( i18n( "Cannot move IMAP folder '%1', it does not exist on the server.",
                     collection().name() ) );
    changeProcessed();
    return;
  }

  if ( sourceCollection().remoteId().isEmpty() ) {
    emitError( i18n( "Cannot move IMAP folder '%1' out of '%2', '%2' does not exist on the server.",
                     collection().name(),
                     sourceCollection().name() ) );
    changeProcessed();
    return;
  }

  if ( targetCollection().remoteId().isEmpty() ) {
    emitError( i18n( "Cannot move IMAP folder '%1' to '%2', '%2' does not exist on the server.",
                     collection().name(),
                     sourceCollection().name() ) );
    changeProcessed();
    return;
  }

  // collection.remoteId() already includes the separator
  const QString oldMailBox = mailBoxForCollection( sourceCollection() )+collection().remoteId();
  const QString newMailBox = mailBoxForCollection( targetCollection() )+collection().remoteId();

  if ( oldMailBox != newMailBox ) {
    KIMAP::RenameJob *job = new KIMAP::RenameJob( session );
    job->setSourceMailBox( oldMailBox );
    job->setDestinationMailBox( newMailBox );

    connect( job, SIGNAL(result(KJob*)),
             this, SLOT(onRenameDone(KJob*)) );

    job->start();

  } else {
    changeProcessed();
  }
}


void MoveCollectionTask::onRenameDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    // Automatically subscribe to the new mailbox name
    KIMAP::RenameJob *rename = static_cast<KIMAP::RenameJob*>( job );

    KIMAP::SubscribeJob *subscribe = new KIMAP::SubscribeJob( rename->session() );
    subscribe->setMailBox( rename->destinationMailBox() );

    connect( subscribe, SIGNAL(result(KJob*)),
             this, SLOT(onSubscribeDone(KJob*)) );

    subscribe->start();
  }
}

void MoveCollectionTask::onSubscribeDone( KJob *job )
{
  if ( job->error() ) {
    emitWarning( i18n( "Failed to subscribe to the folder '%1' on the IMAP server. "
                       "It will disappear on next sync. Use the subscription dialog to overcome that",
                       collection().name() ) );
  }

  changeCommitted( collection() );
}

#include "movecollectiontask.moc"


