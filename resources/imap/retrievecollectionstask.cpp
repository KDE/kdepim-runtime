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

#include "retrievecollectionstask.h"

#include "noselectattribute.h"

#include <akonadi/cachepolicy.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/kmime/messageparts.h>

#include <kmime/kmime_message.h>

#include <KDE/KDebug>
#include <KDE/KLocale>

RetrieveCollectionsTask::RetrieveCollectionsTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( CancelIfNoSession, resource, parent )
{
}

RetrieveCollectionsTask::~RetrieveCollectionsTask()
{
}

void RetrieveCollectionsTask::doStart( KIMAP::Session *session )
{
  Akonadi::Collection root;
  root.setName( resourceName() );
  root.setRemoteId( rootRemoteId() );
  root.setContentMimeTypes( QStringList( Akonadi::Collection::mimeType() ) );
  root.setRights( Akonadi::Collection::ReadOnly );
  root.setParentCollection( Akonadi::Collection::root() );
  root.addAttribute( new NoSelectAttribute( true ) );

  Akonadi::CachePolicy policy;
  policy.setInheritFromParent( false );
  policy.setSyncOnDemand( true );

  QStringList localParts;
  localParts << Akonadi::MessagePart::Envelope
             << Akonadi::MessagePart::Header;
  int cacheTimeout = 60;

  if ( isDisconnectedModeEnabled() ) {
    // For disconnected mode we also cache the body
    // and we keep all data indifinitely
    localParts << Akonadi::MessagePart::Body;
    cacheTimeout = -1;
  }

  policy.setLocalParts( localParts );
  policy.setCacheTimeout( cacheTimeout );
  policy.setIntervalCheckTime( intervalCheckTime() );

  root.setCachePolicy( policy );

  m_reportedCollections.insert( QString(), root );

  // this is ugly, but the result of LSUB is unfortunately not a sub-set of LIST
  // it also contains subscribed but currently not available (eg. deleted) mailboxes
  // so we need to use both and exclude mailboxes in LSUB but not in LIST
  if ( isSubscriptionEnabled() ) {
    KIMAP::ListJob *fullListJob = new KIMAP::ListJob( session );
    fullListJob->setIncludeUnsubscribed( true );
    fullListJob->setQueriedNamespaces( serverNamespaces() );
    connect( fullListJob, SIGNAL(mailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)),
             this, SLOT(onFullMailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)) );
    connect( fullListJob, SIGNAL(result(KJob*)), SLOT(onFullMailBoxesReceiveDone(KJob*)) );
    fullListJob->start();
  }

  KIMAP::ListJob *listJob = new KIMAP::ListJob( session );
  listJob->setIncludeUnsubscribed( !isSubscriptionEnabled() );
  listJob->setQueriedNamespaces( serverNamespaces() );
  connect( listJob, SIGNAL(mailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)),
           this, SLOT(onMailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)) );
  connect( listJob, SIGNAL(result(KJob*)), SLOT(onMailBoxesReceiveDone(KJob*)) );
  listJob->start();
}

void RetrieveCollectionsTask::onMailBoxesReceived( const QList< KIMAP::MailBoxDescriptor > &descriptors,
                                                   const QList< QList<QByteArray> > &flags )
{
  QStringList contentTypes;
  contentTypes << KMime::Message::mimeType() << Akonadi::Collection::mimeType();

  for ( int i=0; i<descriptors.size(); ++i ) {
    KIMAP::MailBoxDescriptor descriptor = descriptors[i];

    // skip phantom mailboxes contained in LSUB but not LIST
    if ( isSubscriptionEnabled() && !m_fullReportedCollections.contains( descriptor.name ) ) {
      kDebug() << "Got phantom mailbox: " << descriptor.name;
      continue;
    }

    const QStringList pathParts = descriptor.name.split(descriptor.separator);
    const QString separator = descriptor.separator;
    Q_ASSERT( separator.size() == 1 ); // that's what the spec says

    QString parentPath;
    QString currentPath;

    for ( int j = 0; j < pathParts.size(); ++j ) {
      const bool isDummy = j != pathParts.size() - 1;
      const QString pathPart = pathParts.at( j );
      currentPath += separator + pathPart;

      if ( m_reportedCollections.contains( currentPath ) ) {
        if ( m_dummyCollections.contains( currentPath ) && !isDummy ) {
          kDebug() << "Received the real collection for a dummy one : " << currentPath;

          //set the correct attributes for the collection, eg. noselect needs to be removed
          Akonadi::Collection c = m_reportedCollections.value( currentPath );
          c.setContentMimeTypes( contentTypes );
          c.setRights( Akonadi::Collection::AllRights );
          c.removeAttribute<NoSelectAttribute>();

          m_dummyCollections.remove( currentPath );
          m_reportedCollections.remove( currentPath );
          m_reportedCollections.insert( currentPath, c );

        }
        parentPath = currentPath;
        continue;
      }

      const QList<QByteArray> currentFlags = isDummy ? (QList<QByteArray>() << "\\noselect") : flags[i];

      Akonadi::Collection c;
      c.setName( pathPart );
      c.setRemoteId( separator + pathPart );
      const Akonadi::Collection parentCollection = m_reportedCollections.value( parentPath );
      c.setParentCollection( parentCollection );
      c.setContentMimeTypes( contentTypes );

      // If the folder is the Inbox, make some special settings.
      if ( currentPath.compare( separator + QLatin1String("INBOX") , Qt::CaseInsensitive ) == 0 ) {
        Akonadi::EntityDisplayAttribute *attr = c.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Collection::AddIfMissing );
        attr->setDisplayName( i18n( "Inbox" ) );
        attr->setIconName( "mail-folder-inbox" );
        setIdleCollection( c );
      }

      // If the folder is the user top-level folder, mark it as well, even although it is not officially noted in the RFC
      if ( currentPath == (separator + QLatin1String( "user" )) && currentFlags.contains( "\\noselect" ) ) {
        Akonadi::EntityDisplayAttribute *attr = c.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Collection::AddIfMissing );
        attr->setDisplayName( i18n( "Shared Folders" ) );
        attr->setIconName( "x-mail-distribution-list" );
      }

      // If this folder is a noselect folder, make some special settings.
      if ( currentFlags.contains( "\\noselect" ) ) {
        kDebug() << "Dummy collection created: " << currentPath;
        c.addAttribute( new NoSelectAttribute( true ) );
        c.setContentMimeTypes( QStringList() << Akonadi::Collection::mimeType() );
        c.setRights( Akonadi::Collection::ReadOnly );
      } else {
        // remove the noselect attribute explicitly, in case we had set it before (eg. for non-subscribed non-leaf folders)
        c.removeAttribute<NoSelectAttribute>();
      }

      m_reportedCollections.insert( currentPath, c );

      if ( isDummy ) {
        m_dummyCollections.insert( currentPath, c );
      }

      parentPath = currentPath;
    }
  }
}

void RetrieveCollectionsTask::onMailBoxesReceiveDone( KJob* job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    collectionsRetrieved( m_reportedCollections.values() );
  }
}

void RetrieveCollectionsTask::onFullMailBoxesReceived( const QList< KIMAP::MailBoxDescriptor >& descriptors,
                                                       const QList< QList< QByteArray > >& flags )
{
  Q_UNUSED( flags );
  foreach ( const KIMAP::MailBoxDescriptor &descriptor, descriptors ) {
    m_fullReportedCollections.insert( descriptor.name );
  }
}

void RetrieveCollectionsTask::onFullMailBoxesReceiveDone(KJob* job)
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  }
}

#include "retrievecollectionstask.moc"


