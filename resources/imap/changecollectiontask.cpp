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

#include "changecollectiontask.h"

#include <kimap/renamejob.h>
#include <kimap/setacljob.h>
#include <kimap/setmetadatajob.h>
#include <kimap/session.h>

#include "collectionannotationsattribute.h"
#include "imapaclattribute.h"
#include "imapquotaattribute.h"

#include <KLocale>

ChangeCollectionTask::ChangeCollectionTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( DeferIfNoSession, resource, parent ), m_pendingJobs(0)
{
}

ChangeCollectionTask::~ChangeCollectionTask()
{
}

void ChangeCollectionTask::doStart( KIMAP::Session *session )
{
  m_collection = collection();
  m_pendingJobs = 0;

  if ( parts().contains( "AccessRights" ) ) {
    Akonadi::ImapAclAttribute *aclAttribute = collection().attribute<Akonadi::ImapAclAttribute>();

    if ( aclAttribute==0 ) {
      emitWarning( i18n( "ACLs for '%1' need to be retrieved from the IMAP server first. Skipping ACL change",
                         collection().name() ) );
    } else {
      KIMAP::Acl::Rights imapRights = aclAttribute->rights()[userName().toUtf8()];
      Akonadi::Collection::Rights newRights = collection().rights();

      if ( newRights & Akonadi::Collection::CanChangeItem ) {
        imapRights|= KIMAP::Acl::Write;
      } else {
        imapRights&= ~KIMAP::Acl::Write;
      }

      if ( newRights & Akonadi::Collection::CanCreateItem ) {
        imapRights|= KIMAP::Acl::Insert;
      } else {
        imapRights&= ~KIMAP::Acl::Insert;
      }

      if ( newRights & Akonadi::Collection::CanDeleteItem ) {
        imapRights|= KIMAP::Acl::DeleteMessage;
      } else {
        imapRights&= ~KIMAP::Acl::DeleteMessage;
      }

      if ( newRights & ( Akonadi::Collection::CanChangeCollection | Akonadi::Collection::CanCreateCollection ) ) {
        imapRights|= KIMAP::Acl::CreateMailbox;
        imapRights|= KIMAP::Acl::Create;
      } else {
        imapRights&= ~KIMAP::Acl::CreateMailbox;
        imapRights&= ~KIMAP::Acl::Create;
      }

      if ( newRights & Akonadi::Collection::CanDeleteCollection ) {
        imapRights|= KIMAP::Acl::DeleteMailbox;
      } else {
        imapRights&= ~KIMAP::Acl::DeleteMailbox;
      }

      if ( ( newRights & Akonadi::Collection::CanDeleteItem )
        && ( newRights & Akonadi::Collection::CanDeleteCollection ) ) {
        imapRights|= KIMAP::Acl::Delete;
      } else {
        imapRights&= ~KIMAP::Acl::Delete;
      }

      kDebug(5327) << "imapRights:" << imapRights
                   << "newRights:" << newRights;

      KIMAP::SetAclJob *job = new KIMAP::SetAclJob( session );
      job->setMailBox( mailBoxForCollection( collection() ) );
      job->setRights( KIMAP::SetAclJob::Change, imapRights );
      job->setIdentifier( userName().toUtf8() );

      connect( job, SIGNAL( result( KJob* ) ),
               this, SLOT( onSetAclDone( KJob* ) ) );

      job->start();

      m_pendingJobs++;
    }
  }

  if ( parts().contains( "collectionannotations" ) ) {
    Akonadi::CollectionAnnotationsAttribute *annotationsAttribute =
      collection().attribute<Akonadi::CollectionAnnotationsAttribute>();

    if ( annotationsAttribute ) { // No annotations it seems... server is lieing to us?
      QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();
      kDebug(5327) << "All annotations: " << annotations;

      foreach ( const QByteArray &entry, annotations.keys() ) {
        KIMAP::SetMetaDataJob *job = new KIMAP::SetMetaDataJob( session );
        if ( serverCapabilities().contains( "METADATA" ) ) {
          job->setServerCapability( KIMAP::MetaDataJobBase::Metadata );
        } else {
          job->setServerCapability( KIMAP::MetaDataJobBase::Annotatemore );
        }

        QByteArray attribute = entry;
        if ( job->serverCapability()==KIMAP::MetaDataJobBase::Annotatemore ) {
          attribute = "value.shared";
        }

        job->setMailBox( mailBoxForCollection( collection() ) );
        job->setEntry( entry );
        job->addMetaData( attribute, annotations[entry] );

        kDebug(5327) << "Job got entry:" << entry << " attribute:" << attribute << "value:" << annotations[entry];

        connect( job, SIGNAL( result( KJob* ) ),
                 this, SLOT( onSetMetaDataDone( KJob* ) ) );

        job->start();

        m_pendingJobs++;
      }
    }
  }

  if ( parts().contains( "imapacl" ) ) {
    Akonadi::ImapAclAttribute *aclAttribute = collection().attribute<Akonadi::ImapAclAttribute>();

    if ( aclAttribute ) {
      const QMap<QByteArray, KIMAP::Acl::Rights> rights = aclAttribute->rights();
      const QMap<QByteArray, KIMAP::Acl::Rights> oldRights = aclAttribute->oldRights();
      const QList<QByteArray> oldIds = oldRights.keys();
      const QList<QByteArray> ids = rights.keys();

      // remove all ACL entries that have been deleted
      foreach ( const QByteArray &oldId, oldIds ) {
        if ( !ids.contains( oldId ) ) {
          KIMAP::SetAclJob *job = new KIMAP::SetAclJob( session );
          job->setMailBox( mailBoxForCollection( collection() ) );
          job->setIdentifier( oldId );
          job->setRights( KIMAP::SetAclJob::Remove, oldRights[oldId] );

          connect( job, SIGNAL( result( KJob* ) ),
                   this, SLOT( onSetAclDone( KJob* ) ) );

          job->start();

          m_pendingJobs++;
        }
      }

      foreach ( const QByteArray &id, ids ) {
        KIMAP::SetAclJob *job = new KIMAP::SetAclJob( session );
        job->setMailBox( mailBoxForCollection( collection() ) );
        job->setIdentifier( id );
        job->setRights( KIMAP::SetAclJob::Change, rights[id] );

        connect( job, SIGNAL( result( KJob* ) ),
                 this, SLOT( onSetAclDone( KJob* ) ) );

        job->start();

        m_pendingJobs++;
      }
    }
  }

  // Check if we need to rename the mailbox
  // This one goes last on purpose, we don't want the previous jobs
  // we triggered to act on the wrong mailbox name
  if ( parts().contains( "NAME" ) ) {
    m_collection.setRemoteId( collection().remoteId().at( 0 ) + collection().name() );

    const QString oldMailBox = mailBoxForCollection( collection() );
    const QString newMailBox = mailBoxForCollection( m_collection );

    if ( oldMailBox != newMailBox ) {
      KIMAP::RenameJob *job = new KIMAP::RenameJob( session );
      job->setSourceMailBox( oldMailBox );
      job->setDestinationMailBox( newMailBox );

      connect( job, SIGNAL( result( KJob* ) ),
               this, SLOT( onRenameDone( KJob* ) ) );

      job->start();

      m_pendingJobs++;
    }
  }

  // we scheduled no change on the server side, probably we got only
  // unsupported part, so just declare the task done
  if ( m_pendingJobs == 0 ) {
    changeCommitted( collection() );
  }
}

void ChangeCollectionTask::onRenameDone( KJob *job )
{
  if ( job->error() ) {
    const QString prevRid = collection().remoteId();
    Q_ASSERT( !prevRid.isEmpty() );

    emitWarning( i18n( "Failed to rename the folder, restoring folder list." ) );

    m_collection.setName( prevRid.mid( 1 ) );
    m_collection.setRemoteId( prevRid );
  }

  endTaskIfNeeded();
}

void ChangeCollectionTask::onSetAclDone( KJob *job )
{
  if ( job->error() ) {
    emitWarning( i18n( "Failed to write some ACLs for '%1' on the IMAP server. %2",
                       collection().name(), job->errorText() ) );
  }

  endTaskIfNeeded();
}

void ChangeCollectionTask::onSetMetaDataDone( KJob *job )
{
  if ( job->error() ) {
    emitWarning( i18n( "Failed to write some annotations for '%1' on the IMAP server. %2",
                       collection().name(), job->errorText() ) );
  }

  endTaskIfNeeded();
}

void ChangeCollectionTask::endTaskIfNeeded()
{
  if ( --m_pendingJobs == 0 ) {
    // the others have ended, we're done, the next one can go
    changeCommitted( m_collection );
  }
}

#include "changecollectiontask.moc"


