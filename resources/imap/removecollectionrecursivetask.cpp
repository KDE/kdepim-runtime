/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Tobias Koenig <tokoe@kdab.com>

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

#include "removecollectionrecursivetask.h"

#include <akonadi/kmime/messageflags.h>
#include <kimap/deletejob.h>
#include <kimap/expungejob.h>
#include <kimap/selectjob.h>
#include <kimap/storejob.h>
#include <klocale.h>

RemoveCollectionRecursiveTask::RemoveCollectionRecursiveTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( CancelIfNoSession, resource, parent ),
    mSession( 0 ), mRunningDeleteJobs( 0 )
{
}

RemoveCollectionRecursiveTask::~RemoveCollectionRecursiveTask()
{
}

void RemoveCollectionRecursiveTask::doStart( KIMAP::Session *session )
{
  mSession = session;

  KIMAP::ListJob *listJob = new KIMAP::ListJob( session );
  listJob->setIncludeUnsubscribed( !isSubscriptionEnabled() );
  listJob->setQueriedNamespaces( serverNamespaces() );
  connect( listJob, SIGNAL(mailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)),
           this, SLOT(onMailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)) );
  connect( listJob, SIGNAL(result(KJob*)), SLOT(onJobDone(KJob*)) );
  listJob->start();
}

void RemoveCollectionRecursiveTask::onMailBoxesReceived( const QList< KIMAP::MailBoxDescriptor > &descriptors,
                                                         const QList< QList<QByteArray> >& )
{
  const QString mailBox = mailBoxForCollection( collection() );

  // We have to delete the deepest-nested folders first, so
  // we use a map here that has the level of nesting as key.
  QMap<int, QList<KIMAP::MailBoxDescriptor> > foldersToDelete;

  for ( int i = 0; i < descriptors.size(); ++i ) {
    const KIMAP::MailBoxDescriptor descriptor = descriptors[ i ];

    if ( descriptor.name.startsWith( mailBox ) ) { // a sub folder to delete
      const QStringList pathParts = descriptor.name.split( descriptor.separator );
      foldersToDelete[ pathParts.count() ].append( descriptor );
    }
  }

  if  ( foldersToDelete.isEmpty() ) {
      changeProcessed();

      kDebug(5327) << "Failed to find the folder to be deleted, resync the folder tree";
      emitWarning( i18n( "Failed to find the folder to be deleted, restoring folder list." ) );
      synchronizeCollectionTree();
      return;
  }

  // Now start the actual deletion work
  QMapIterator<int, QList<KIMAP::MailBoxDescriptor> > it( foldersToDelete );
  it.toBack(); // we start with largest nesting value first
  while ( it.hasPrevious() ) {
    it.previous();

    foreach ( const KIMAP::MailBoxDescriptor &descriptor, it.value() ) {
      // first select the mailbox
      KIMAP::SelectJob *selectJob = new KIMAP::SelectJob( mSession );
      selectJob->setMailBox( descriptor.name );
      connect( selectJob, SIGNAL(result(KJob*)), SLOT(onJobDone(KJob*)) );
      selectJob->start();

      // mark all items as deleted
      KIMAP::ImapSet allItems;
      allItems.add( KIMAP::ImapInterval( 1, 0 ) ); // means 1:*
      KIMAP::StoreJob *storeJob = new KIMAP::StoreJob( mSession );
      storeJob->setSequenceSet( allItems );
      storeJob->setFlags( KIMAP::MessageFlags() << Akonadi::MessageFlags::Deleted );
      storeJob->setMode( KIMAP::StoreJob::AppendFlags );
      connect( storeJob, SIGNAL(result(KJob*)), SLOT(onJobDone(KJob*)) );
      storeJob->start();

      // expunge the mailbox
      KIMAP::ExpungeJob *expungeJob = new KIMAP::ExpungeJob( mSession );
      connect( expungeJob, SIGNAL(result(KJob*)), SLOT(onJobDone(KJob*)) );
      expungeJob->start();

      // finally delete the mailbox
      KIMAP::DeleteJob *deleteJob = new KIMAP::DeleteJob( mSession );
      deleteJob->setMailBox( descriptor.name );
      connect( deleteJob, SIGNAL(result(KJob*)), SLOT(onDeleteJobDone(KJob*)) );
      mRunningDeleteJobs++;
      deleteJob->start();
    }
  }
}

void RemoveCollectionRecursiveTask::onDeleteJobDone( KJob* job )
{
  if ( job->error() ) {
    changeProcessed();

    kDebug(5327) << "Failed to delete the folder, resync the folder tree";
    emitWarning( i18n( "Failed to delete the folder, restoring folder list." ) );
    synchronizeCollectionTree();
  } else {
    mRunningDeleteJobs--;

    if ( mRunningDeleteJobs == 0 ) {
      changeProcessed(); // finished job
    }
  }
}

void RemoveCollectionRecursiveTask::onJobDone( KJob* job )
{
  if ( job->error() ) {
    changeProcessed();

    kDebug(5327) << "Failed to delete the folder, resync the folder tree";
    emitWarning( i18n( "Failed to delete the folder, restoring folder list." ) );
    synchronizeCollectionTree();
  }
}

#include "removecollectionrecursivetask.moc"
