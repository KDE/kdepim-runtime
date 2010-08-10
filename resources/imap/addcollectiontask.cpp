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

#include "addcollectiontask.h"

#include <KDE/KDebug>
#include <KDE/KLocale>

#include <kimap/createjob.h>
#include <kimap/session.h>
#include <kimap/subscribejob.h>

AddCollectionTask::AddCollectionTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( DeferIfNoSession, resource, parent )
{

}

AddCollectionTask::~AddCollectionTask()
{
}

void AddCollectionTask::doStart( KIMAP::Session *session )
{
  if ( parentCollection().remoteId().isEmpty() ) {
    emitError( i18n("Cannot add IMAP folder '%1' for a non-existing parent folder '%2'.",
                    collection().name(),
                    parentCollection().name() ) );
    changeProcessed();
    return;
  }

  const QChar separator = parentCollection().remoteId().at( 0 );
  m_collection = collection();
  m_collection.setName( m_collection.name().replace( separator, QString() ) );
  m_collection.setRemoteId( separator + m_collection.name() );

  QString newMailBox = mailBoxForCollection( parentCollection() );

  if ( !newMailBox.isEmpty() )
    newMailBox += separator;

  newMailBox += m_collection.name();

  kDebug(5327) << "New folder: " << newMailBox;

  KIMAP::CreateJob *job = new KIMAP::CreateJob( session );
  job->setMailBox( newMailBox );

  connect( job, SIGNAL( result( KJob* ) ),
           this, SLOT( onCreateDone( KJob* ) ) );

  job->start();
}


void AddCollectionTask::onCreateDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    // Automatically subscribe to newly created mailbox
    KIMAP::CreateJob *create = static_cast<KIMAP::CreateJob*>( job );

    KIMAP::SubscribeJob *subscribe = new KIMAP::SubscribeJob( create->session() );
    subscribe->setMailBox( create->mailBox() );

    connect( subscribe, SIGNAL( result( KJob* ) ),
             this, SLOT( onSubscribeDone( KJob* ) ) );

    subscribe->start();
  }
}

void AddCollectionTask::onSubscribeDone( KJob *job )
{
  if ( job->error() && isSubscriptionEnabled() ) {
    emitWarning( i18n( "Failed to subscribe to the collection '%1' on the IMAP server. "
                       "It will disappear on next sync. Use the subscription dialog to overcome that",
                       m_collection.name() ) );
  }

  changeCommitted( m_collection );
}

#include "addcollectiontask.moc"


