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

#include "expungecollectiontask.h"

#include <KDE/KDebug>
#include <KDE/KLocale>

#include <kimap/expungejob.h>
#include <kimap/selectjob.h>
#include <kimap/session.h>

ExpungeCollectionTask::ExpungeCollectionTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( CancelIfNoSession, resource, parent )
{

}

ExpungeCollectionTask::~ExpungeCollectionTask()
{
}

void ExpungeCollectionTask::doStart( KIMAP::Session *session )
{
  const QString mailBox = mailBoxForCollection( collection() );

  if ( session->selectedMailBox() != mailBox ) {
    KIMAP::SelectJob *select = new KIMAP::SelectJob( session );
    select->setMailBox( mailBox );

    connect( select, SIGNAL( result( KJob* ) ),
             this, SLOT( onSelectDone( KJob* ) ) );

    select->start();

  } else {
    triggerExpungeJob( session );
  }
}

void ExpungeCollectionTask::onSelectDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob*>( job );
    triggerExpungeJob( select->session() );
  }
}

void ExpungeCollectionTask::triggerExpungeJob( KIMAP::Session *session )
{
  KIMAP::ExpungeJob *expunge = new KIMAP::ExpungeJob( session );

  connect( expunge, SIGNAL( result( KJob* ) ),
           this, SLOT( onExpungeDone( KJob * ) ) );

  expunge->start();
}

void ExpungeCollectionTask::onExpungeDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    taskDone();
  }
}

#include "expungecollectiontask.moc"


