/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#include "retrieveitemsjob.h"
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/transactionsequence.h>

#include <QDateTime>
#include <KMime/Message>

RetrieveItemsJob::RetrieveItemsJob ( const Akonadi::Collection& collection, const KPIM::Maildir& md, QObject* parent ) :
  Job ( parent ),
  m_collection( collection ),
  m_maildir( md ),
  m_mimeType( KMime::Message::mimeType() ),
  m_transaction( 0 )
{
  Q_ASSERT( m_collection.isValid() );
  Q_ASSERT( m_maildir.isValid() );
}

void RetrieveItemsJob::setMimeType ( const QString& mimeType )
{
  m_mimeType = mimeType;
}

void RetrieveItemsJob::doStart()
{
  Q_ASSERT( !m_mimeType.isEmpty() );
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( m_collection, this );
  connect( job, SIGNAL(result(KJob*)), SLOT(localListDone(KJob*)) );
}

void RetrieveItemsJob::localListDone ( KJob* job )
{
  if ( job->error() ) return; // handled by base class

  const Akonadi::Item::List items = qobject_cast<Akonadi::ItemFetchJob*>( job )->items();
  m_localItems.reserve( items.size() );
  foreach ( const Akonadi::Item &item, items )
    m_localItems.insert( item.remoteId(), item );

  m_listingPath = m_maildir.path() + QLatin1String( "/new/" );
  m_entryList = m_maildir.listNew();
  m_previousMtime = m_collection.remoteRevision().toLongLong();
  m_highestMtime = 0;
  processEntry(0);
}

void RetrieveItemsJob::processEntry(qint64 index)
{
  if (index >= m_entryList.size()) {
    if ( m_listingPath.endsWith( QLatin1String( "/new/" ) ) ) {
      m_listingPath = m_maildir.path() + QLatin1String( "/cur/" );
      m_entryList = m_maildir.listCurrent();
      processEntry(0);
    } else {
      entriesProcessed();
    }
    return;
  }
    
  QString entry = m_entryList[index];  
  const qint64 currentMtime = m_maildir.lastModified( entry ).toMSecsSinceEpoch();
  m_highestMtime = qMax( m_highestMtime, currentMtime );
  if ( currentMtime <= m_previousMtime ) { // old, we got this one already
    m_localItems.remove( entry );
    processEntry(index+1);
    return;
  }

  Akonadi::Item item;
  item.setRemoteId( entry );
  item.setMimeType( m_mimeType );
  item.setSize( m_maildir.size( entry ) );
  KMime::Message *msg = new KMime::Message;
  msg->setHead( KMime::CRLFtoLF( m_maildir.readEntryHeaders( m_listingPath + entry ) ) );
  msg->parse();
  
  Akonadi::Item::Flags flags = m_maildir.readEntryFlags( entry );
  Q_FOREACH( Akonadi::Item::Flag flag, flags ) {
    item.setFlag(flag);      
  }
  
  item.setPayload( KMime::Message::Ptr( msg ) );

  if ( m_localItems.contains( entry ) ) { // modification
    item.setId( m_localItems.value( entry ).id() );
    new Akonadi::ItemModifyJob( item, transaction() );
    m_localItems.remove( entry );
  } else { // new item
    new Akonadi::ItemCreateJob( item, m_collection, transaction() );  
  }
  
  if ( index % 20 == 0 ) {
     QMetaObject::invokeMethod( this, "processEntry", Qt::QueuedConnection, Q_ARG(qint64, index + 1) );
  } else
      processEntry(index+1);

}

void RetrieveItemsJob::entriesProcessed()
{
  if ( !m_localItems.isEmpty() ) {
    Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob( m_localItems.values(), transaction() );
    transaction()->setIgnoreJobFailure( job );
  }

  // update mtime
  if ( m_highestMtime != m_previousMtime ) {
    Akonadi::Collection newCol( m_collection );
    newCol.setRemoteRevision( QString::number( m_highestMtime ) );
    Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( newCol, transaction() );
    transaction()->setIgnoreJobFailure( job );
  }
  
  if ( !m_transaction ) // no jobs created here -> done
    emitResult();
}



Akonadi::TransactionSequence* RetrieveItemsJob::transaction()
{
  if ( !m_transaction ) {
    m_transaction = new Akonadi::TransactionSequence( this );
    connect( m_transaction, SIGNAL(result(KJob*)), SLOT(transactionDone(KJob*)) );
  }
  return m_transaction;
}

void RetrieveItemsJob::transactionDone ( KJob* job )
{
  if ( job->error() ) return; // handled by base class
  emitResult();
}

#include "retrieveitemsjob.moc"
