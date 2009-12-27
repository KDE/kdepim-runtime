/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "augmentedmaildirstore.h"

#include "collectionfetchjob.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemmodifyjob.h"
#include "sessionimpls_p.h"
#include "storecompactjob.h"

#include "maildir.h"

#include <kmime/kmime_message.h>

#include <akonadi/attribute.h>
#include <akonadi/attributefactory.h>
#include <akonadi/cachepolicy.h>
#include <akonadi/collection.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/kmime/messageparts.h>

#include <KDebug>
#include <KConfig>
#include <KConfigGroup>
#include <KLocale>

#include <QFile>
#include <QFileInfo>
#include <QHash>

using namespace Akonadi::FileStore;

class AugmentedMailDirStore::AugmentedMailDirStore::Private : public Job::Visitor
{
  public:
    Private( AugmentedMailDirStore *parent )
      : mJobSession( 0 ),
        mParent( parent )
    {
    }

    virtual ~Private()
    {
    }

    void onJobsReady( const QList<Job*> &jobs )
    {
      foreach ( Job* job, jobs ) {
        job->accept( this );
      }
    }

    // job visitor interface
    bool visit( Job *job ) {
      kError() << "Unhandled job class" << job->metaObject()->className();
      return false;
    }

    bool visit( CollectionFetchJob *job ) {
      processCollectionFetch( job );
      return true;
    }

    bool visit( ItemCreateJob *job ) {
      processItemCreate( job );
      return true;
    }

    bool visit( ItemDeleteJob *job ) {
      processItemDelete( job );
      return true;
    }

    bool visit( ItemFetchJob *job ) {
      processItemFetch( job );
      return true;
    }

    bool visit( ItemModifyJob *job ) {
      processItemModify( job );
      return true;
    }

    bool visit( StoreCompactJob *job ) {
      processStoreCompact( job );
      return true;
    }

    KPIM::Maildir mailDirForPath( const QString &path );

  public:
    mutable AbstractJobSession *mJobSession;

    typedef QHash<QString, KPIM::Maildir> MailDirMap;
    MailDirMap mMailDirs;

  private:
    AugmentedMailDirStore *mParent;

  private:
    void processCollectionFetch( CollectionFetchJob *job );

    Akonadi::Collection::List fetchChildCollections( const KPIM::Maildir &mailDir, const Akonadi::Collection &parent );

    Akonadi::Collection::List fetchChildCollectionsRecursive( const KPIM::Maildir &mailDir, const Akonadi::Collection &parent );

    void processItemFetch( ItemFetchJob *job );
    void processItemFetchAll( const Akonadi::Collection &parent, const KPIM::Maildir &mailDir, ItemFetchJob *job );
    void processItemFetchSingle( ItemFetchJob *job );

    bool fetchPayload( const KPIM::Maildir &mailDir, const QString &entryName, Item &item, ItemFetchJob *job ) const;
    bool fetchAttributes( const KPIM::Maildir &mailDir, const QString &entryName, Item &item, ItemFetchJob *job ) const;
    bool fetchFlags( const KPIM::Maildir &mailDir, const QString &entryName, Item &item, ItemFetchJob *job ) const;

    void processItemCreate( ItemCreateJob *job );

    void processItemModify( ItemModifyJob *job );

    void processItemDelete( ItemDeleteJob *job );

    bool storePayload( KPIM::Maildir &mailDir, QString &entryName, Item &item, Job *job );
    bool storeAttributes( const KPIM::Maildir &mailDir, const QString &entryName, const Item &item, Job *job );
    bool storeFlags( const KPIM::Maildir &mailDir, const QString &entryName, const Item &item, Job *job );

    void processStoreCompact( StoreCompactJob *job );
};

AugmentedMailDirStore::AugmentedMailDirStore( QObject *parent )
  : AbstractDirectAccessStore( parent ), d( new Private( this ) )
{
  d->mJobSession = new FiFoQueueJobSession( this );

  Akonadi::Collection collection;
  collection.setContentMimeTypes( QStringList() << Akonadi::Collection::mimeType() << KMime::Message::mimeType() );

  setTopLevelCollection( collection );

  QObject::connect( d->mJobSession, SIGNAL( jobsReady( const QList<Job*>& ) ),
                    this, SLOT( onJobsReady( const QList<Job*>& ) ) );
}

AugmentedMailDirStore::~AugmentedMailDirStore()
{
  delete d;
}

void AugmentedMailDirStore::setFileName( const QString &fileName )
{
/*  kDebug() << "fileName=" << fileName << ", old=" << this->fileName();*/
  d->mJobSession->cancelAllJobs();

  d->mMailDirs.clear();

  AbstractDirectAccessStore::setFileName( fileName );

  KPIM::Maildir mailDir( AbstractDirectAccessStore::fileName() );
  if ( mailDir.isValid() || mailDir.create() ) {
    d->mMailDirs.insert( AbstractDirectAccessStore::fileName(), mailDir );
  }
}

CollectionFetchJob *AugmentedMailDirStore::fetchCollections( const Collection &collection, CollectionFetchJob::Type type, const CollectionFetchScope *fetchScope ) const
{
  CollectionFetchJob *job = new CollectionFetchJob( collection, type, d->mJobSession );

  if ( d->mMailDirs.isEmpty() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "<filename>%1</filename> is not a valid MailDir folder" ).arg( AbstractDirectAccessStore::fileName() );
    d->mJobSession->setError( job, Job::InvalidStoreState, message );
    return job;
  }

  if ( collection.remoteId().isEmpty() ||
       !d->mailDirForPath( collection.remoteId() ).isValid() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "Collection remote ID is empty or not the store's one" );
    d->mJobSession->setError( job, Job::InvalidJobContext, message );
    return job;
  }

  if ( fetchScope != 0 ) {
    job->setFetchScope( *fetchScope );
  }

  return job;
}

ItemFetchJob *AugmentedMailDirStore::fetchItems( const Collection &collection, const ItemFetchScope *fetchScope ) const
{
  ItemFetchJob *job = new ItemFetchJob( collection, d->mJobSession );

  if ( d->mMailDirs.isEmpty() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "<filename>%1</filename> is not a valid MailDir folder" ).arg( AbstractDirectAccessStore::fileName() );
    d->mJobSession->setError( job, Job::InvalidStoreState, message );
    return job;
  }

  if ( collection.remoteId().isEmpty() ||
       !d->mailDirForPath( collection.remoteId() ).isValid() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "Collection remote ID is empty or not the store's one" );
    d->mJobSession->setError( job, Job::InvalidJobContext, message );
    return job;
  }

  if ( fetchScope != 0 ) {
    job->setFetchScope( *fetchScope );
  }

  return job;
}

ItemFetchJob *AugmentedMailDirStore::fetchItem( const Item &item, const ItemFetchScope *fetchScope ) const
{
  ItemFetchJob *job = new ItemFetchJob( item, d->mJobSession );

  if ( d->mMailDirs.isEmpty() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "<filename>%1</filename> is not a valid MailDir folder" ).arg( AbstractDirectAccessStore::fileName() );
    d->mJobSession->setError( job, Job::InvalidStoreState, message );
    return job;
  }

  if ( item.remoteId().isEmpty() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "Item remote ID is empty" );
    d->mJobSession->setError( job, Job::InvalidJobContext, message );
    return job;
  }

  if ( fetchScope != 0 ) {
    job->setFetchScope( *fetchScope );
  }

  return job;
}

ItemCreateJob *AugmentedMailDirStore::createItem( const Item &item, const Collection &collection )
{
  ItemCreateJob *job = new ItemCreateJob( item, collection, d->mJobSession );

  if ( d->mMailDirs.isEmpty() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "<filename>%1</filename> is not a valid MailDir folder" ).arg( AbstractDirectAccessStore::fileName() );
    d->mJobSession->setError( job, Job::InvalidStoreState, message );
    return job;
  }

  if ( collection.remoteId().isEmpty() ||
       !d->mailDirForPath( collection.remoteId() ).isValid() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "Collection remote ID is empty or not the store's one" );
    d->mJobSession->setError( job, Job::InvalidJobContext, message );
    return job;
  }

  if ( !item.hasPayload<KMime::Message::Ptr>() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "No message data available" );
    d->mJobSession->setError( job, Job::InvalidJobContext, message );
    return job;
  }

  // TODO check payload?
  KMime::Message::Ptr messagePtr = item.payload<KMime::Message::Ptr>();
  if ( messagePtr->encodedContent().isEmpty() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "No message data available" );
    d->mJobSession->setError( job, Job::InvalidJobContext, message );
    return job;
  }

  if ( !item.remoteId().isEmpty() ) {
    // TODO warning logging, store generates the remoteId
  }

  return job;
}

ItemModifyJob *AugmentedMailDirStore::modifyItem( const Item &item, bool ignorePayload )
{
  ItemModifyJob *job = new ItemModifyJob( item, d->mJobSession );
  job->setIgnorePayload( ignorePayload );

  if ( d->mMailDirs.isEmpty() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "<filename>%1</filename> is not a valid MailDir folder" ).arg( AbstractDirectAccessStore::fileName() );
    d->mJobSession->setError( job, Job::InvalidStoreState, message );
    return job;
  }

  if ( item.remoteId().isEmpty() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "Item remote ID is empty" );
    d->mJobSession->setError( job, Job::InvalidJobContext, message );
    return job;
  }

  if ( !ignorePayload ) {
    if ( !item.hasPayload<KMime::Message::Ptr>() ) {
      // TODO logging
      // TODO better messahe
      const QString message = i18n( "No message data in item" );
      d->mJobSession->setError( job, Job::InvalidJobContext, message );
      return job;
    }
  }

  return job;
}

ItemDeleteJob *AugmentedMailDirStore::deleteItem( const Item &item )
{
  ItemDeleteJob *job = new ItemDeleteJob( item, d->mJobSession );

  if ( d->mMailDirs.isEmpty() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "<filename>%1</filename> is not a valid MailDir folder" ).arg( AbstractDirectAccessStore::fileName() );
    d->mJobSession->setError( job, Job::InvalidStoreState, message );
    return job;
  }

  if ( item.remoteId().isEmpty() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "Item remote ID is empty" );
    d->mJobSession->setError( job, Job::InvalidJobContext, message );
    return job;
  }

  return job;
}

StoreCompactJob *AugmentedMailDirStore::compactStore()
{
  StoreCompactJob *job = new StoreCompactJob( d->mJobSession );

  if ( d->mMailDirs.isEmpty() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "<filename>%1</filename> is not a valid MailDir folder" ).arg( AbstractDirectAccessStore::fileName() );
    d->mJobSession->setError( job, Job::InvalidStoreState, message );
    return job;
  }

  return job;
}

KPIM::Maildir AugmentedMailDirStore::Private::mailDirForPath( const QString &path )
{
  Q_ASSERT( !path.isEmpty() );

  MailDirMap::const_iterator findIt = mMailDirs.constFind( path );
  if ( findIt != mMailDirs.constEnd() ) {
    return findIt.value();
  }

  Q_ASSERT( path.startsWith( mParent->fileName() ) );
  Q_ASSERT( path != mParent->fileName() );

  const QString relativePath = path.mid( mParent->fileName().length() );
  const QStringList relativeNames = relativePath.split( QLatin1Char( '/' ), QString::SkipEmptyParts );

  KPIM::Maildir mailDir( mParent->fileName() );
  foreach ( const QString &subFolder, relativeNames ) {
    mailDir = mailDir.subFolder( subFolder );
    if ( !mailDir.isValid() ) {
      return KPIM::Maildir();
    }
  }

  Q_ASSERT( mailDir.isValid() );

  mMailDirs.insert( path, mailDir );

  return mailDir;
}

void AugmentedMailDirStore::Private::processCollectionFetch( CollectionFetchJob *job )
{
  const Akonadi::Collection collection = job->collection();

  const KPIM::Maildir mailDir = mailDirForPath( collection.remoteId() );
  if ( !mailDir.isValid() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "<filename>%1</filename> is not a valid MailDir folder" ).arg( collection.remoteId() );
    mJobSession->setError( job, Job::InvalidStoreState, message );
    return;
  }

  Akonadi::Collection::List collections;

  switch ( job->type() )
  {
    case CollectionFetchJob::Base:
      if ( collection.remoteId() == mParent->topLevelCollection().remoteId() ) {
        collections << mParent->topLevelCollection();
      } else if ( mailDirForPath( collection.remoteId() ).isValid() ) {
        Collection result( collection );
        result.setContentMimeTypes( QStringList() << KMime::Message::mimeType() );
        collections << result;
      }
      break;

    case CollectionFetchJob::FirstLevel:
      collections = fetchChildCollections( mailDir, collection );
      break;

    case CollectionFetchJob::Recursive:
      collections = fetchChildCollectionsRecursive( mailDir, collection );
      break;
  }

  mJobSession->notifyCollectionsReceived( job, collections );
  mJobSession->emitResult( job );
}

Akonadi::Collection::List AugmentedMailDirStore::Private::fetchChildCollections( const KPIM::Maildir &mailDir, const Akonadi::Collection &parent )
{
  const QStringList subFolders = mailDir.subFolderList();

  const QString remoteIdTemplate = parent.remoteId() + QLatin1String( "/%1" );

  Akonadi::Collection::List collections;
  foreach ( const QString &subFolder, subFolders ) {
    Collection collection;
    collection.setContentMimeTypes( QStringList() << KMime::Message::mimeType() );
    collection.setRemoteId( remoteIdTemplate.arg( subFolder ) );
    collection.setParentCollection( parent );

    const KPIM::Maildir childMailDir = mailDirForPath( collection.remoteId() );
    if ( !childMailDir.isValid() ) {
      kError() << collection.remoteId() << "is not a valid maildir";
    } else {
      collections << collection;
    }
  }

  return collections;
}

Akonadi::Collection::List AugmentedMailDirStore::Private::fetchChildCollectionsRecursive( const KPIM::Maildir &mailDir, const Akonadi::Collection &parent )
{
  Akonadi::Collection::List childCollections = fetchChildCollections( mailDir, parent );

  if ( !childCollections.isEmpty() ) {
    Akonadi::Collection::List grandChildCollections;
    foreach ( const Akonadi::Collection &collection, childCollections ) {
      const KPIM::Maildir childMailDir = mailDirForPath( collection.remoteId() );
      if ( !childMailDir.isValid() ) {
        kError() << collection.remoteId() << "is not a valid maildir";
      } else {
        grandChildCollections << fetchChildCollectionsRecursive( childMailDir, collection );
      }
    }

    childCollections << grandChildCollections;
  }

  return childCollections;
}

void AugmentedMailDirStore::Private::processItemFetch( ItemFetchJob *job )
{
  const Akonadi::Collection collection = job->collection();
  if ( !collection.remoteId().isEmpty() ) {
    const KPIM::Maildir mailDir = mailDirForPath( collection.remoteId() );
    if ( !mailDir.isValid() ) {
      // TODO better message
      const QString message = i18n( "Collection remote ID is empty or not the store's one" );
      mJobSession->setError( job, Job::InvalidJobContext, message );
      mJobSession->emitResult( job );
      return;
    }

    processItemFetchAll( collection, mailDir, job );
  } else {
    processItemFetchSingle( job );
  }
}

void AugmentedMailDirStore::Private::processItemFetchAll( const Akonadi::Collection &parent, const KPIM::Maildir &mailDir, ItemFetchJob *job )
{
  const QString pathTemplate = mailDir.path() + QLatin1String( "/%1" );

  Akonadi::Item::List items;

  const QStringList entryList = mailDir.entryList();
  foreach ( const QString &entryName, entryList ) {
    Akonadi::Item item( KMime::Message::mimeType() );
    item.setRemoteId( pathTemplate.arg( entryName ) );
    item.setParentCollection( parent );

    bool ok = true;
    ok = ok && fetchPayload( mailDir, entryName, item, job );
    ok = ok && fetchAttributes( mailDir, entryName, item, job );
    ok = ok && fetchFlags( mailDir, entryName, item, job );
    if ( !ok ) {
      mJobSession->emitResult( job );
      return;
    }

    items << item;
  }

  mJobSession->notifyItemsReceived( job, items );
  mJobSession->emitResult( job );
}

void AugmentedMailDirStore::Private::processItemFetchSingle( ItemFetchJob *job )
{
  Item item( job->item() );

  QFileInfo fileInfo( item.remoteId() );
  const QString entryName = fileInfo.fileName();
  const QString path = fileInfo.path();

  const KPIM::Maildir mailDir = mailDirForPath( path );
  if ( !mailDir.isValid() ) {
    // TODO better message
    const QString message = i18n( "Item remote ID is not the store's one" );
    mJobSession->setError( job, Job::InvalidJobContext, message );
    mJobSession->emitResult( job );
    return;
  }

  bool ok = true;
  ok = ok && fetchPayload( mailDir, entryName, item, job );
  ok = ok && fetchAttributes( mailDir, entryName, item, job );
  ok = ok && fetchFlags( mailDir, entryName, item, job );
  if ( !ok ) {
    mJobSession->emitResult( job );
    return;
  }

  Akonadi::Collection parent;
  parent.setContentMimeTypes( QStringList() << KMime::Message::mimeType() );
  parent.setRemoteId( path );

  item.setParentCollection( parent );

  Akonadi::Item::List items;
  items << item;

  mJobSession->notifyItemsReceived( job, items );
  mJobSession->emitResult( job );
}

bool AugmentedMailDirStore::Private::fetchPayload( const KPIM::Maildir &mailDir, const QString &entryName, Item &item, ItemFetchJob *job ) const
{
  const QSet<QByteArray> parts = job->fetchScope().payloadParts();
  if ( job->fetchScope().fullPayload() || parts.contains( Akonadi::MessagePart::Body ) ) {
    const QByteArray mail = mailDir.readEntry( entryName );
    if ( mail.isEmpty() ) {
      // TODO logging
      // TODO better message
      const QString message = i18n( "Item remote ID does not address any message in this store" );
      mJobSession->setError( job, Job::InvalidJobContext, message );
      return false;
    }

    KMime::Message::Ptr messagePtr( new KMime::Message() );
    messagePtr->setContent( KMime::CRLFtoLF( mail ) );
    item.setPayload<KMime::Message::Ptr>( messagePtr );
  } else if ( parts.contains( Akonadi::MessagePart::Envelope ) ||
              parts.contains( Akonadi::MessagePart::Header )) {
    const QByteArray headers = mailDir.readEntryHeaders( entryName );
    if ( headers.isEmpty() ) {
      // TODO logging
      // TODO better message
      const QString message = i18n( "Item remote ID does not address any message in this store" );
      mJobSession->setError( job, Job::InvalidJobContext, message );
      return false;
    }

    KMime::Message::Ptr messagePtr( new KMime::Message() );
    messagePtr->setHead( KMime::CRLFtoLF( headers ) );
    item.setPayload<KMime::Message::Ptr>( messagePtr );
  }

  return true;
}

bool AugmentedMailDirStore::Private::fetchAttributes( const KPIM::Maildir &mailDir, const QString &entryName, Item &item, ItemFetchJob *job ) const
{
  if ( !job->fetchScope().allAttributes() && job->fetchScope().attributes().isEmpty() ) {
    return true;
  }

  const QString fileName = mailDir.path() + QLatin1String( "/akonadi/" ) + entryName + QLatin1String( ".attributes" );

  QFile attributeFile( fileName );
  if ( attributeFile.exists() && !attributeFile.open( QIODevice::ReadOnly ) ) {
      // TODO logging
      // TODO better message
      const QString message = i18n( "Item attribute file %1 exists but cannot be read", fileName );
      // TODO better error code
      mJobSession->setError( job, Job::InvalidJobContext, message );
      return false;
  }

  const QSet<QByteArray> attributeSet = job->fetchScope().attributes();

  QDataStream stream( &attributeFile );
  while ( !stream.atEnd() ) {
    QByteArray attributeName;
    QByteArray attributeData;

    stream >> attributeName >> attributeData;
    if ( job->fetchScope().allAttributes() || attributeSet.contains( attributeName ) ) {
      Akonadi::Attribute *attribute = Akonadi::AttributeFactory::createAttribute( attributeName );
      if ( attribute != 0 ) {
        if ( attributeData.isEmpty() ) {
          kWarning() << "Attribute" << attributeName << "has empty data";
        }
        attribute->deserialize( attributeData );
        item.addAttribute( attribute );
      } else {
        kWarning() << "Failed to create attribute" << attributeName;
      }
    }
  }

  return true;
}

bool AugmentedMailDirStore::Private::fetchFlags( const KPIM::Maildir &mailDir, const QString &entryName, Item &item, ItemFetchJob *job ) const
{
  const QString fileName = mailDir.path() + QLatin1String( "/akonadi/" ) + entryName + QLatin1String( ".flags" );

  QFile flagFile( fileName );
  if ( flagFile.exists() && !flagFile.open( QIODevice::ReadOnly ) ) {
      // TODO logging
      // TODO better message
      const QString message = i18n( "Item flag file %1 exists but cannot be read", fileName );
      // TODO better error code
      mJobSession->setError( job, Job::InvalidJobContext, message );
      return false;
  }

  QDataStream stream( &flagFile );

  Akonadi::Item::Flags flags;
  stream >> flags;

  item.setFlags( flags );

  return true;
}

void AugmentedMailDirStore::Private::processItemCreate( ItemCreateJob *job )
{
  const Akonadi::Collection collection = job->collection();
  KPIM::Maildir mailDir = mailDirForPath( collection.remoteId() );
  if ( !mailDir.isValid() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "Collection remote ID is empty or not the store's one" );
    mJobSession->setError( job, Job::InvalidJobContext, message );
    return;
  }

  Akonadi::Item item = job->item();
  if ( !item.remoteId().isEmpty() ) {
    kDebug() << "Remote ID of item to create is not empty, discarding it. Only the store assigns remote IDs";
    item.setRemoteId( QString() );
  }

  QString entryName;

  bool ok = true;
  ok = storePayload( mailDir, entryName, item, job );
  ok = ok && storeAttributes( mailDir, entryName, item, job );
  ok = ok && storeFlags( mailDir, entryName, item, job );

  if ( ok ) {
    item.setParentCollection( collection );
    mJobSession->notifyItemCreated( job, item );
  }

  mJobSession->emitResult( job );
}

void AugmentedMailDirStore::Private::processItemModify( ItemModifyJob *job )
{
  Akonadi::Item item = job->item();

  QFileInfo fileInfo( item.remoteId() );
  QString entryName = fileInfo.fileName();
  const QString path = fileInfo.path();

  KPIM::Maildir mailDir = mailDirForPath( path );
  if ( !mailDir.isValid() ) {
    // TODO better message
    const QString message = i18n( "Item remote ID is not the store's one" );
    mJobSession->setError( job, Job::InvalidJobContext, message );
    mJobSession->emitResult( job );
    return;
  }

  bool ok = true;
  if ( !job->ignorePayload() ) {
    ok = storePayload( mailDir, entryName, item, job );
  }
  ok = ok && storeAttributes( mailDir, entryName, item, job );
  ok = ok && storeFlags( mailDir, entryName, item, job );

  if ( ok ) {
    mJobSession->notifyItemModified( job, item );
  }

  mJobSession->emitResult( job );
}

void AugmentedMailDirStore::Private::processItemDelete( ItemDeleteJob *job )
{
  const Akonadi::Item item = job->item();

  QFileInfo fileInfo( item.remoteId() );
  const QString entryName = fileInfo.fileName();
  const QString path = fileInfo.path();

  KPIM::Maildir mailDir = mailDirForPath( path );
  if ( !mailDir.isValid() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "Maildir folder %1 does not contain a subfolder %2", mParent->fileName(), path );
    mJobSession->setError( job, Job::InvalidJobContext, message );
    mJobSession->emitResult( job );
    return;
  }

  if ( !mailDir.removeEntry( entryName ) ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "Maildir folder %1 does not contain a message with ID %2", path, entryName );
    mJobSession->setError( job, Job::InvalidJobContext, message );
    mJobSession->emitResult( job );
    return;
  }

  const QString attributeFileName = mailDir.path() + QLatin1String( "/akonadi/" ) + entryName + QLatin1String( ".attributes" );

  QFile attributeFile( attributeFileName );
  if ( attributeFile.exists() && !attributeFile.remove() ) {
    kWarning() << "Cannot remove attribute file" << attributeFileName;
  }

  const QString flagFileName = mailDir.path() + QLatin1String( "/akonadi/" ) + entryName + QLatin1String( ".flags" );

  QFile flagFile( flagFileName );
  if ( flagFile.exists() && !flagFile.remove() ) {
    kWarning() << "Cannot remove attribute file" << flagFileName;
  }

  mJobSession->emitResult( job );
}

bool AugmentedMailDirStore::Private::storePayload( KPIM::Maildir &mailDir, QString &entryName, Item &item, Job *job )
{
  if ( !item.remoteId().isEmpty() ) {
    QFileInfo fileInfo( item.remoteId() );
    entryName = fileInfo.fileName();
    if ( entryName.isEmpty() || mailDir.path() != fileInfo.path() || mailDir.size( entryName ) <= 0 ){
      // TODO logging
      // TODO better message
      const QString message = i18n( "Item remote ID is not valid" );
      mJobSession->setError( job, Job::InvalidJobContext, message );
      return false;
    }
  }

  const KMime::Message::Ptr messagePtr = item.payload<KMime::Message::Ptr>();
  if ( entryName.isEmpty() ) {
    entryName = mailDir.addEntry( messagePtr->encodedContent() );
    item.setRemoteId( mailDir.path() + QLatin1Char( '/' ) + entryName );
  } else {
    mailDir.writeEntry( entryName, messagePtr->encodedContent() );
  }

  return true;
}

bool AugmentedMailDirStore::Private::storeAttributes( const KPIM::Maildir &mailDir, const QString &entryName, const Item &item, Job *job )
{
  Q_ASSERT( !item.remoteId().isEmpty() );

  const QString fileName = mailDir.path() + QLatin1String( "/akonadi/" ) + entryName + QLatin1String( ".attributes" );

  QFile attributeFile( fileName );
  if ( !attributeFile.open( QIODevice::WriteOnly ) ) {
      // TODO logging
      // TODO better message
      const QString message = i18n( "Item attribute file %1 cannot be written", fileName );
      // TODO better error code
      mJobSession->setError( job, Job::InvalidJobContext, message );
      return false;
  }

  attributeFile.resize( 0 );
  QDataStream stream( &attributeFile );

  const Akonadi::Attribute::List attributes = item.attributes();
  foreach ( const Akonadi::Attribute *attribute, attributes ) {
    stream << attribute->type() << attribute->serialized();
  }

  return true;
}

bool AugmentedMailDirStore::Private::storeFlags( const KPIM::Maildir &mailDir, const QString &entryName, const Item &item, Job *job )
{
  Q_ASSERT( !item.remoteId().isEmpty() );

  const QString fileName = mailDir.path() + QLatin1String( "/akonadi/" ) + entryName + QLatin1String( ".flags" );

  QFile flagFile( fileName );
  if ( !flagFile.open( QIODevice::WriteOnly ) ) {
      // TODO logging
      // TODO better message
      const QString message = i18n( "Item flag file %1 cannot be written", fileName );
      // TODO better error code
      mJobSession->setError( job, Job::InvalidJobContext, message );
      return false;
  }

  flagFile.resize( 0 );

  QDataStream stream( &flagFile );
  stream << item.flags();

  return true;
}

void AugmentedMailDirStore::Private::processStoreCompact( StoreCompactJob *job )
{
  // TODO check if there are stale attribute or flags

  mJobSession->emitResult( job );
}

#include "augmentedmaildirstore.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
