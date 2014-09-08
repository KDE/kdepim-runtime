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

#include "removeitemstask.h"

#include "resource_imap_debug.h"
#include <QDebug>

#include <KLocalizedString>

#include <kimap/selectjob.h>
#include <kimap/session.h>
#include <kimap/storejob.h>

#include "imapflags.h"

RemoveItemsTask::RemoveItemsTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( DeferIfNoSession, resource, parent )
{

}

RemoveItemsTask::~RemoveItemsTask()
{
}

void RemoveItemsTask::doStart( KIMAP::Session *session )
{
  // The imap specs do not allow for a single message to be deleted. We can only
  // set the \Deleted flag. The message will actually be deleted when EXPUNGE will
  // be issued on the next retrieveItems().

  const QString mailBox = mailBoxForCollection( items().first().parentCollection() );

  qCDebug(RESOURCE_IMAP_LOG) << "Deleting " << items().size() << " messages from " << mailBox;

  if ( session->selectedMailBox() != mailBox ) {
    KIMAP::SelectJob *select = new KIMAP::SelectJob( session );
    select->setMailBox( mailBox );

    connect( select, SIGNAL(result(KJob*)),
             this, SLOT(onSelectDone(KJob*)) );

    select->start();

  } else {
    triggerStoreJob( session );
  }
}

void RemoveItemsTask::onSelectDone( KJob *job )
{
  if ( job->error() ) {
    qWarning() << "Failed to select mailbox: " << job->errorString();
    cancelTask( job->errorString() );
  } else {
    KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob*>( job );
    triggerStoreJob( select->session() );
  }
}

void RemoveItemsTask::triggerStoreJob( KIMAP::Session *session )
{
  KIMAP::ImapSet set;
  foreach( const Akonadi::Item &item, items() ) {
    set.add( item.remoteId().toLong() );
  }

  KIMAP::StoreJob *store = new KIMAP::StoreJob( session );
  store->setUidBased( true );
  store->setSequenceSet( set );
  store->setFlags( QList<QByteArray>() << ImapFlags::Deleted );
  store->setMode( KIMAP::StoreJob::AppendFlags );
  connect(store, &KIMAP::StoreJob::result, this, &RemoveItemsTask::onStoreFlagsDone);
  store->start();
}

void RemoveItemsTask::onStoreFlagsDone( KJob *job )
{
  //TODO use UID EXPUNGE if available
  if ( job->error() ) {
    qWarning() << "Failed to append flags: " << job->errorString();
    cancelTask( job->errorString() );
  } else {
    changeProcessed();
  }
}



