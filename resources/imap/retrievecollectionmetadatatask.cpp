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

#include "retrievecollectionmetadatatask.h"

#include <QtCore/QDateTime>

#include <kimap/getacljob.h>
#include <kimap/getmetadatajob.h>
#include <kimap/getquotarootjob.h>
#include <kimap/myrightsjob.h>
#include <kimap/rfccodecs.h>
#include <kimap/session.h>
#include <klocale.h>

#include <akonadi/collectionquotaattribute.h>
#include <akonadi/entitydisplayattribute.h>
#include "collectionannotationsattribute.h"
#include "imapaclattribute.h"
#include "imapquotaattribute.h"
#include "noselectattribute.h"
#include "timestampattribute.h"

const uint RetrieveCollectionMetadataTask::TimestampTimeout = 3600 * 24 * 20; // 20 days

RetrieveCollectionMetadataTask::RetrieveCollectionMetadataTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( CancelIfNoSession, resource, parent ), m_isSpontaneous( true ),
    m_pendingMetaDataJobs( 0 ), m_collectionChanged( false )
{
}

RetrieveCollectionMetadataTask::~RetrieveCollectionMetadataTask()
{
}

bool RetrieveCollectionMetadataTask::isSpontaneous() const
{
  return m_isSpontaneous;
}

void RetrieveCollectionMetadataTask::setSpontaneous( bool spontaneous )
{
  m_isSpontaneous = spontaneous;
}

void RetrieveCollectionMetadataTask::doStart( KIMAP::Session *session )
{
  kDebug(5327) << collection().remoteId();

  // Prevent fetching items from noselect folders.
  if ( collection().hasAttribute( "noselect" ) ) {
    NoSelectAttribute* noselect = static_cast<NoSelectAttribute*>( collection().attribute( "noselect" ) );
    if ( noselect->noSelect() ) {
      kDebug(5327) << "No Select folder";
      endTask();
      return;
    }
  }

  // Evaluate timestamps only if this task is spontaneous (triggered from within the resource)
  if ( m_isSpontaneous ) {
    uint timestamp = 0;
    const uint currentTimestamp = QDateTime::currentDateTime().toTime_t();

    if ( collection().hasAttribute<TimestampAttribute>() ) {
      timestamp = collection().attribute<TimestampAttribute>()->timestamp();
    } else {
      m_collectionChanged = true;
    }

    // Refresh only if we're older than twenty days
    if ( timestamp + TimestampTimeout >  currentTimestamp ) {
      kDebug(5327) << "Not refreshing because of timestamp";
      endTask();
      return;
    }
  }

  m_collection = collection();
  const QString mailBox = mailBoxForCollection( m_collection );
  const QStringList capabilities = serverCapabilities();

  m_pendingMetaDataJobs = 0;

  // First get the annotations from the mailbox if it's supported
  if ( capabilities.contains( "METADATA" ) || capabilities.contains( "ANNOTATEMORE" ) ) {
    KIMAP::GetMetaDataJob *meta = new KIMAP::GetMetaDataJob( session );
    meta->setMailBox( mailBox );
    if ( capabilities.contains( "METADATA" ) ) {
      meta->setServerCapability( KIMAP::MetaDataJobBase::Metadata );
      meta->addEntry( "*" );
    } else {
      meta->setServerCapability( KIMAP::MetaDataJobBase::Annotatemore );
      meta->addEntry( "*", "value.shared" );
    }
    connect( meta, SIGNAL( result( KJob* ) ), SLOT( onGetMetaDataDone( KJob* ) ) );
    meta->start();
    m_pendingMetaDataJobs++;
  }

  // Get the ACLs from the mailbox if it's supported
  if ( capabilities.contains( "ACL" ) ) {
    KIMAP::GetAclJob *acl = new KIMAP::GetAclJob( session );
    acl->setMailBox( mailBox );
    connect( acl, SIGNAL( result( KJob* ) ), SLOT( onGetAclDone( KJob* ) ) );
    acl->start();
    m_pendingMetaDataJobs++;

    KIMAP::MyRightsJob *rights = new KIMAP::MyRightsJob( session );
    rights->setMailBox( mailBox );
    connect( rights, SIGNAL( result( KJob* ) ), SLOT( onRightsReceived( KJob* ) ) );
    rights->start();
    m_pendingMetaDataJobs++;
  }

  // Get the QUOTA info from the mailbox if it's supported
  if ( capabilities.contains( "QUOTA" ) ) {
    KIMAP::GetQuotaRootJob *quota = new KIMAP::GetQuotaRootJob( session );
    quota->setMailBox( mailBox );
    connect( quota, SIGNAL( result( KJob* ) ), SLOT( onQuotasReceived( KJob* ) ) );
    quota->start();
    m_pendingMetaDataJobs++;
  }

  // the server does not have any of the capabilities needed to get extra info, so this
  // step is done here
  if ( m_pendingMetaDataJobs == 0 ) {
    endTask();
  }
}

void RetrieveCollectionMetadataTask::onGetMetaDataDone( KJob *job )
{
  if ( job->error() ) {
    endTaskIfNeeded();
    return; // Well, no metadata for us then...
  }

  KIMAP::GetMetaDataJob *meta = qobject_cast<KIMAP::GetMetaDataJob*>( job );
  QMap<QByteArray, QMap<QByteArray, QByteArray> > rawAnnotations = meta->allMetaData( meta->mailBox() );

  QMap<QByteArray, QByteArray> annotations;
  QByteArray attribute = "";
  if ( meta->serverCapability()==KIMAP::MetaDataJobBase::Annotatemore ) {
    attribute = "value.shared";
  }

  foreach ( const QByteArray &entry, rawAnnotations.keys() ) {
    annotations[entry] = rawAnnotations[entry][attribute];
  }

  // filter out unused and annoying Cyrus annotation /vendor/cmu/cyrus-imapd/lastupdate
  // which contains the current date and time and thus constantly changes for no good
  // reason which triggers a change notification and thus a bunch of Akonadi operations
  annotations.remove( "/vendor/cmu/cyrus-imapd/lastupdate" );

  // Store the mailbox metadata
  Akonadi::CollectionAnnotationsAttribute *annotationsAttribute =
    m_collection.attribute<Akonadi::CollectionAnnotationsAttribute>( Akonadi::Collection::AddIfMissing );
  const QMap<QByteArray, QByteArray> oldAnnotations = annotationsAttribute->annotations();
  if ( oldAnnotations != annotations ) {
    annotationsAttribute->setAnnotations( annotations );
    m_collectionChanged = true;
  }

  endTaskIfNeeded();
}

void RetrieveCollectionMetadataTask::onGetAclDone( KJob *job )
{
  if ( job->error() ) {
    endTaskIfNeeded();
    return; // Well, no metadata for us then...
  }

  KIMAP::GetAclJob *acl = qobject_cast<KIMAP::GetAclJob*>( job );

  // Store the mailbox ACLs
  Akonadi::ImapAclAttribute *aclAttribute
    = m_collection.attribute<Akonadi::ImapAclAttribute>( Akonadi::Collection::AddIfMissing );
  const QMap<QByteArray, KIMAP::Acl::Rights> oldRights = aclAttribute->rights();
  if ( oldRights != acl->allRights() ) {
    aclAttribute->setRights( acl->allRights() );
    m_collectionChanged = true;
  }

  endTaskIfNeeded();
}

void RetrieveCollectionMetadataTask::onRightsReceived( KJob *job )
{
  if ( job->error() ) {
    endTaskIfNeeded();
    return; // Well, no metadata for us then...
  }

  KIMAP::MyRightsJob *rightsJob = qobject_cast<KIMAP::MyRightsJob*>( job );

  Akonadi::ImapAclAttribute * const parentAclAttribute =
        collection().parentCollection().attribute<Akonadi::ImapAclAttribute>();
  KIMAP::Acl::Rights parentRights = 0;
  if ( parentAclAttribute ) {
    parentRights = parentAclAttribute->rights()[userName().toUtf8()];
  }

  KIMAP::Acl::Rights imapRights = rightsJob->rights();
  Akonadi::Collection::Rights newRights = Akonadi::Collection::ReadOnly;

  // For renaming, the parent folder needs to have the CreateMailbox or Create permission.
  // We map renaming to CanChangeCollection here, which is not entirely correct, but we have no
  // CanRenameCollection flag.
  // If the ACL of the parent folder hasn't been retrieved yet, allow changing, since we don't know
  // better. If the parent folder is a noselect folder though, don't allow it, since for those we have
  // no CreateMailbox right.
  if ( ( !parentAclAttribute && !collection().parentCollection().hasAttribute( "noselect" ) ) ||
       parentRights & KIMAP::Acl::CreateMailbox ||
       parentRights & KIMAP::Acl::Create ) {
    newRights|= Akonadi::Collection::CanChangeCollection;
  }

  if ( imapRights & KIMAP::Acl::Write ) {
    newRights|= Akonadi::Collection::CanChangeItem;
  }

  if ( imapRights & KIMAP::Acl::Insert ) {
    newRights|= Akonadi::Collection::CanCreateItem;
  }

  if ( imapRights & ( KIMAP::Acl::DeleteMessage | KIMAP::Acl::Delete ) ) {
    newRights|= Akonadi::Collection::CanDeleteItem;
  }

  if ( imapRights & ( KIMAP::Acl::CreateMailbox | KIMAP::Acl::Create ) ) {
    newRights|= Akonadi::Collection::CanCreateCollection;
  }

  if ( imapRights & ( KIMAP::Acl::DeleteMailbox | KIMAP::Acl::Delete ) ) {
    newRights|= Akonadi::Collection::CanDeleteCollection;
  }

//  kDebug(5327) << collection.remoteId()
//               << "imapRights:" << imapRights
//               << "newRights:" << newRights
//               << "oldRights:" << collection.rights();

  if ( (m_collection.rights() & Akonadi::Collection::CanCreateItem) &&
       !(newRights & Akonadi::Collection::CanCreateItem) ) {
    // write access revoked
    const QString collectionName = m_collection.hasAttribute<Akonadi::EntityDisplayAttribute>() ?
                                     m_collection.attribute<Akonadi::EntityDisplayAttribute>()->displayName() :
                                     m_collection.name();

    showInformationDialog( i18n( "<p>Your access rights to folder <b>%1</b> have been restricted, "
                                 "it will no longer be possible to add messages to this folder.</p>",
                                 collectionName ),
                           i18n( "Access rights revoked" ), "ShowRightsRevokedWarning" );
  }

  if ( newRights != m_collection.rights() ) {
    m_collection.setRights( newRights );
    m_collectionChanged = true;
  }

  endTaskIfNeeded();
}

void RetrieveCollectionMetadataTask::onQuotasReceived( KJob *job )
{
  if ( job->error() ) {
    endTaskIfNeeded();
    return; // Well, no metadata for us then...
  }

  KIMAP::GetQuotaRootJob *quotaJob = qobject_cast<KIMAP::GetQuotaRootJob*>( job );
  const QString &mailBox = mailBoxForCollection( m_collection );

  QList<QByteArray> newRoots = quotaJob->roots();
  QList< QMap<QByteArray, qint64> > newLimits;
  QList< QMap<QByteArray, qint64> > newUsages;
  qint64 newCurrent = -1;
  qint64 newMax = -1;

  foreach ( const QByteArray &root, newRoots ) {
    newLimits << quotaJob->allLimits( root );
    newUsages << quotaJob->allUsages( root );

    const QString &decodedRoot = QString::fromUtf8( KIMAP::decodeImapFolderName( root ) );

    if ( newRoots.size() == 1 || decodedRoot == mailBox ) {
      newCurrent = newUsages.last()["STORAGE"] * 1024;
      newMax = newLimits.last()["STORAGE"] * 1024;
    }
  }

  // Store the mailbox IMAP Quotas
  Akonadi::ImapQuotaAttribute *imapQuotaAttribute
    = m_collection.attribute<Akonadi::ImapQuotaAttribute>( Akonadi::Collection::AddIfMissing );
  const QList<QByteArray> oldRoots = imapQuotaAttribute->roots();
  const QList< QMap<QByteArray, qint64> > oldLimits = imapQuotaAttribute->limits();
  const QList< QMap<QByteArray, qint64> > oldUsages = imapQuotaAttribute->usages();

  if ( oldRoots != newRoots
    || oldLimits != newLimits
    || oldUsages != newUsages )
  {
    imapQuotaAttribute->setQuotas( newRoots, newLimits, newUsages );
    m_collectionChanged = true;
  }

  // Store the collection Quota
  Akonadi::CollectionQuotaAttribute *quotaAttribute
    = m_collection.attribute<Akonadi::CollectionQuotaAttribute>( Akonadi::Collection::AddIfMissing );
  qint64 oldCurrent = quotaAttribute->currentValue();
  qint64 oldMax = quotaAttribute->maximumValue();

  if ( oldCurrent != newCurrent
    || oldMax != newMax ) {
    quotaAttribute->setCurrentValue( newCurrent );
    quotaAttribute->setMaximumValue( newMax );
    m_collectionChanged = true;
  }

  endTaskIfNeeded();
}

void RetrieveCollectionMetadataTask::endTaskIfNeeded()
{
  if ( --m_pendingMetaDataJobs == 0 ) {
    // the others have ended, we're done, the next one can go
    if ( m_collectionChanged ) {
      const uint currentTimestamp = QDateTime::currentDateTime().toTime_t();

      TimestampAttribute *attr = m_collection.attribute<TimestampAttribute>( Akonadi::Collection::AddIfMissing );
      attr->setTimestamp( currentTimestamp );

      if ( m_isSpontaneous ) {
        applyCollectionChanges( m_collection );
      }
    }

    endTask();
  }
}

void RetrieveCollectionMetadataTask::endTask()
{
  kDebug() << "Done!";
  if ( m_isSpontaneous ) {
    taskDone();
  } else {
    collectionAttributesRetrieved( m_collection );
  }
}

#include "retrievecollectionmetadatatask.moc"


