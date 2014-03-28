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
#include <kimap/selectjob.h>

#include <QtCore/QUuid>

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

  if ( session->selectedMailBox() != mailBoxForCollection( collection() ) ) {
    doRename( session );
    return;
  }

  // Some IMAP servers don't allow moving an opened mailbox, so make sure
  // it's not opened (https://bugs.kde.org/show_bug.cgi?id=324932) by examining
  // a non-existent mailbox. We don't use CLOSE in order not to trigger EXPUNGE
  KIMAP::SelectJob *examine = new KIMAP::SelectJob( session );
  examine->setOpenReadOnly( true ); // use EXAMINE instead of SELECT
  examine->setMailBox( QString::fromLatin1( "IMAP Resource non existing folder %1" ).arg( QUuid::createUuid().toString() ) );
  connect( examine, SIGNAL(result(KJob*)),
           this, SLOT(onExamineDone(KJob*)) );
  examine->start();
}

void MoveCollectionTask::onExamineDone( KJob* job )
{
  // We deliberately ignore any error here, because the SelectJob will always fail
  // when examining a non-existent mailbox

  KIMAP::SelectJob *examine = static_cast<KIMAP::SelectJob*>( job );
  doRename( examine->session() );
}

QString MoveCollectionTask::mailBoxForCollections( const Akonadi::Collection& parent, const Akonadi::Collection& child ) const
{
  const QString parentMailbox = mailBoxForCollection( parent );
  if ( parentMailbox.isEmpty() ) {
      return child.remoteId().mid(1); //Strip separator on toplevel mailboxes
  }
  return parentMailbox + child.remoteId();
}

void MoveCollectionTask::doRename( KIMAP::Session *session )
{
  // collection.remoteId() already includes the separator
  const QString oldMailBox = mailBoxForCollections( sourceCollection(), collection() );
  const QString newMailBox = mailBoxForCollections( targetCollection(), collection() );

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



