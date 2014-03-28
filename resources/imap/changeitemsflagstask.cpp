/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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


#include "changeitemsflagstask.h"

#include <kimap/session.h>
#include <kimap/selectjob.h>
#include <kimap/storejob.h>

ChangeItemsFlagsTask::ChangeItemsFlagsTask( ResourceStateInterface::Ptr resource, QObject* parent ):
    ResourceTask( ResourceTask::DeferIfNoSession, resource, parent )
{

}

ChangeItemsFlagsTask::~ChangeItemsFlagsTask()
{
}

void ChangeItemsFlagsTask::doStart(KIMAP::Session* session)
{
  const QString mailBox = mailBoxForCollection( items().first().parentCollection() );

  if ( session->selectedMailBox() != mailBox ) {
    KIMAP::SelectJob *select = new KIMAP::SelectJob( session );
    select->setMailBox( mailBox );

    connect( select, SIGNAL(result(KJob*)),
             this, SLOT(onSelectDone(KJob*)) );

    select->start();

  } else {
    if ( !addedFlags().isEmpty() ) {
        triggerAppendFlagsJob( session );
    } else if ( !removedFlags().isEmpty() ) {
        triggerRemoveFlagsJob( session );
    } else {
        changeProcessed();
    }
  }
}

void ChangeItemsFlagsTask::onSelectDone(KJob* job)
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob*>( job );
    if ( !addedFlags().isEmpty() ) {
        triggerAppendFlagsJob( select->session() );
    } else if ( !removedFlags().isEmpty() ) {
        triggerRemoveFlagsJob( select->session() );
    } else {
        changeProcessed();
    }
  }
}

KIMAP::StoreJob* ChangeItemsFlagsTask::prepareJob( KIMAP::Session *session )
{
  KIMAP::ImapSet set;
  foreach( const Akonadi::Item &item, items() ) {
    set.add( item.remoteId().toLong() );
  }

  KIMAP::StoreJob *store = new KIMAP::StoreJob( session );
  store->setUidBased( true );
  store->setSequenceSet( set );

  return store;
}

void ChangeItemsFlagsTask::triggerAppendFlagsJob(KIMAP::Session* session)
{
  KIMAP::StoreJob *store = prepareJob( session );
  store->setFlags( fromAkonadiToSupportedImapFlags( addedFlags().toList(), items().first().parentCollection() ) );
  store->setMode( KIMAP::StoreJob::AppendFlags );
  connect( store, SIGNAL(result(KJob*)), SLOT(onAppendFlagsDone(KJob*)) );
  store->start();
}

void ChangeItemsFlagsTask::triggerRemoveFlagsJob(KIMAP::Session* session)
{
  KIMAP::StoreJob *store = prepareJob( session );
  store->setFlags( fromAkonadiToSupportedImapFlags( removedFlags().toList(), items().first().parentCollection() ) );
  store->setMode( KIMAP::StoreJob::RemoveFlags );
  connect( store, SIGNAL(result(KJob*)), SLOT(onRemoveFlagsDone(KJob*)) );
  store->start();
}

void ChangeItemsFlagsTask::onAppendFlagsDone(KJob* job)
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    if ( removedFlags().isEmpty() ) {
      changeProcessed();
    } else {
      KIMAP::StoreJob *storeJob = qobject_cast<KIMAP::StoreJob*>( job );
      triggerRemoveFlagsJob( storeJob->session() );
    }
  }
}

void ChangeItemsFlagsTask::onRemoveFlagsDone(KJob* job)
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    changeProcessed();
  }
}

