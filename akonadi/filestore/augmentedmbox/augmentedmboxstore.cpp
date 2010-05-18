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

#include "augmentedmboxstore.h"

#include "collectionfetchjob.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemmodifyjob.h"
#include "sessionimpls_p.h"
#include "storecompactjob.h"

#include "mbox.h"

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
#include <QHash>

using namespace Akonadi::FileStore;

class AugmentedMBoxStore::AugmentedMBoxStore::Private : public Job::Visitor
{
  public:
    Private( AugmentedMBoxStore *parent )
      : mJobSession( 0 ),
        mMailBox( 0 ), mFileReadable( false ), mFileWritable( false ),
        mIndex( 0 ),
        mParent( parent )
    {
    }

    virtual ~Private()
    {
      delete mIndex;
      delete mMailBox;
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
      Akonadi::Collection::List collections;

      if ( job->type() == CollectionFetchJob::Base ) {
        collections << mParent->topLevelCollection();
      }

      mJobSession->notifyCollectionsReceived( job, collections );
      mJobSession->emitResult( job );
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

  public:
    mutable AbstractJobSession *mJobSession;

    MBox *mMailBox;

    bool mFileReadable;
    bool mFileWritable;

    KConfig* mIndex;

    struct MailInfo
    {
      quint64 offset;
      KMime::Message::Ptr headers;
    };
    typedef QHash<QByteArray, MailInfo> MailInfoMap;
    MailInfoMap mMailInfos;

    QSet<quint64> mDeleteOffsets;

  private:
    AugmentedMBoxStore *mParent;

  private:
    void buildMailInfoMap();

    void processItemFetch( ItemFetchJob *job );
    void processItemFetchAll( ItemFetchJob *job );
    void processItemFetchSingle( ItemFetchJob *job );

    bool fetchPayload( Item &item, ItemFetchJob *job ) const;
    bool fetchAttributes( Item &item, ItemFetchJob *job ) const;
    bool fetchFlags( Item &item, ItemFetchJob *job ) const;

    void processItemCreate( ItemCreateJob *job );

    void processItemModify( ItemModifyJob *job );

    void processItemDelete( ItemDeleteJob *job );

    bool storePayload( Item &item, Job *job );
    bool storeAttributes( const Item &item, Job *job );
    bool storeFlags( const Item &item, Job *job );

    void processStoreCompact( StoreCompactJob *job );
};

AugmentedMBoxStore::AugmentedMBoxStore( QObject *parent )
  : AbstractDirectAccessStore( parent ), d( new Private( this ) )
{
  d->mJobSession = new FiFoQueueJobSession( this );

  Akonadi::Collection collection;
  collection.setContentMimeTypes( QStringList() << KMime::Message::mimeType() );

  setTopLevelCollection( collection );

  QObject::connect( d->mJobSession, SIGNAL( jobsReady( const QList<Job*>& ) ),
                    this, SLOT( onJobsReady( const QList<Job*>& ) ) );
}

AugmentedMBoxStore::~AugmentedMBoxStore()
{
  if ( !d->mDeleteOffsets.isEmpty() && d->mMailBox != 0 ) {
    kWarning() << d->mDeleteOffsets.count()
               << "messages marked for deletion have not been purged yet. Purging now";
    d->mMailBox->purge( d->mDeleteOffsets );
  }

  delete d;
}

void AugmentedMBoxStore::setFileName( const QString &fileName )
{
/*  kDebug() << "fileName=" << fileName << ", old=" << this->fileName();*/
  d->mJobSession->cancelAllJobs();

  d->mMailInfos.clear();
  d->mDeleteOffsets.clear();
  delete d->mIndex;
  delete d->mMailBox;

  AbstractDirectAccessStore::setFileName( fileName );

  QFile file( AbstractDirectAccessStore::fileName() );
  if ( file.open( QIODevice::ReadWrite ) ) {
    d->mFileReadable = true;
    d->mFileWritable = true;
  } else if ( file.open( QIODevice::ReadOnly ) ) {
    d->mFileReadable = true;
    d->mFileWritable = false;
  } else {
    d->mFileReadable = false;
    d->mFileWritable = false;
  }
  file.close();

  d->mMailBox = new MBox();
  if ( !d->mMailBox->load( AbstractDirectAccessStore::fileName() ) ) {
    d->mFileReadable = false;
    d->mFileWritable = false;
  }

  d-> mIndex = new KConfig( AbstractDirectAccessStore::fileName() + QLatin1String( ".index" ),
                            KConfig::SimpleConfig );
}

CollectionFetchJob *AugmentedMBoxStore::fetchCollections( const Collection &collection, CollectionFetchJob::Type type, const CollectionFetchScope *fetchScope ) const
{
  CollectionFetchJob *job = new CollectionFetchJob( collection, type, d->mJobSession );

  if ( !d->mFileReadable ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File not readable" );
    d->mJobSession->setError( job, Job::InvalidStoreState, message );
    return job;
  }

  if ( collection.remoteId().isEmpty() ||
       collection.remoteId() != topLevelCollection().remoteId() ) {
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

ItemFetchJob *AugmentedMBoxStore::fetchItems( const Collection &collection, const ItemFetchScope *fetchScope ) const
{
  ItemFetchJob *job = new ItemFetchJob( collection, d->mJobSession );

  if ( !d->mFileReadable ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File not readable" );
    d->mJobSession->setError( job, Job::InvalidStoreState, message );
    return job;
  }

  if ( collection.remoteId().isEmpty() ||
       collection.remoteId() != topLevelCollection().remoteId() ) {
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

ItemFetchJob *AugmentedMBoxStore::fetchItem( const Item &item, const ItemFetchScope *fetchScope ) const
{
  ItemFetchJob *job = new ItemFetchJob( item, d->mJobSession );

  if ( !d->mFileReadable ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File not readable" );
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

ItemCreateJob *AugmentedMBoxStore::createItem( const Item &item, const Collection &collection )
{
  ItemCreateJob *job = new ItemCreateJob( item, collection, d->mJobSession );

  if ( !d->mFileWritable ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File not writable" );
    d->mJobSession->setError( job, Job::InvalidStoreState, message );
    return job;
  }

  if ( d->mIndex->accessMode() != KConfigBase::ReadWrite ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "Index not writable" );
    d->mJobSession->setError( job, Job::InvalidStoreState, message );
    return job;
  }

  if ( collection.remoteId().isEmpty() ||
       collection.remoteId() != topLevelCollection().remoteId() ) {
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

ItemModifyJob *AugmentedMBoxStore::modifyItem( const Item &item, bool ignorePayload )
{
  ItemModifyJob *job = new ItemModifyJob( item, d->mJobSession );
  job->setIgnorePayload( ignorePayload );

  if ( !d->mFileWritable ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File not writable" );
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

ItemDeleteJob *AugmentedMBoxStore::deleteItem( const Item &item )
{
  ItemDeleteJob *job = new ItemDeleteJob( item, d->mJobSession );

  if ( !d->mFileWritable ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File not writable" );
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

StoreCompactJob *AugmentedMBoxStore::compactStore()
{
  StoreCompactJob *job = new StoreCompactJob( d->mJobSession );

  if ( !d->mFileWritable ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File not writable" );
    d->mJobSession->setError( job, Job::InvalidStoreState, message );
    return job;
  }

  return job;
}

void AugmentedMBoxStore::Private::buildMailInfoMap()
{
  const QList<MsgEntryInfo> msgInfoList = mMailBox->entryList( mDeleteOffsets );
  foreach ( const MsgEntryInfo &msgInfo, msgInfoList ) {
    MailInfo mailInfo;
    mailInfo.offset = msgInfo.offset;

    mailInfo.headers = KMime::Message::Ptr( new KMime::Message() );
    mailInfo.headers->setHead( KMime::CRLFtoLF( mMailBox->readEntryHeaders( mailInfo.offset ) ) );

    mailInfo.headers->parse(); // otherwise messageID will return empty
    const QByteArray id = mailInfo.headers->messageID()->identifier();

    // use lastest occurence of a message id, mark earlier ones as deletes
    MailInfoMap::iterator findIt = mMailInfos.find( id );
    if ( findIt != mMailInfos.end() ) {
      mDeleteOffsets << findIt->offset;
    }
    mMailInfos.insert( id, mailInfo );
  }
}

void AugmentedMBoxStore::Private::processItemFetch( ItemFetchJob *job )
{
  const Akonadi::Collection collection = job->collection();
  if ( !collection.remoteId().isEmpty() ) {
    if ( collection.remoteId() != mParent->topLevelCollection().remoteId() ) {
      // TODO better message
      const QString message = i18n( "Collection remote ID is empty or not the store's one" );
      mJobSession->setError( job, Job::InvalidJobContext, message );
      mJobSession->emitResult( job );
      return;
    }

    processItemFetchAll( job );
  } else {
    processItemFetchSingle( job );
  }
}

void AugmentedMBoxStore::Private::processItemFetchAll( ItemFetchJob *job )
{
  if ( mMailInfos.isEmpty() ) {
    buildMailInfoMap();
  }

  Akonadi::Item::List items;

  MailInfoMap::const_iterator it    = mMailInfos.constBegin();
  MailInfoMap::const_iterator endIt = mMailInfos.constEnd();
  for ( ; it != endIt; ++it ) {
    Akonadi::Item item( KMime::Message::mimeType() );
    item.setRemoteId( QString::fromUtf8( it.key() ) );
    item.setParentCollection( mParent->topLevelCollection() );

    bool ok = true;
    ok = ok && fetchPayload( item, job );
    ok = ok && fetchAttributes( item, job );
    ok = ok && fetchFlags( item, job );
    if ( !ok ) {
      mJobSession->emitResult( job );
      return;
    }

    items << item;
  }

  mJobSession->notifyItemsReceived( job, items );
  mJobSession->emitResult( job );
}

void AugmentedMBoxStore::Private::processItemFetchSingle( ItemFetchJob *job )
{
  if ( mMailInfos.isEmpty() ) {
    buildMailInfoMap();
  }

  Item item( job->item() );

  bool ok = true;
  ok = ok && fetchPayload( item, job );
  ok = ok && fetchAttributes( item, job );
  ok = ok && fetchFlags( item, job );
  if ( !ok ) {
    mJobSession->emitResult( job );
    return;
  }

  item.setParentCollection( mParent->topLevelCollection() );

  Akonadi::Item::List items;
  items << item;

  mJobSession->notifyItemsReceived( job, items );
  mJobSession->emitResult( job );
}

bool AugmentedMBoxStore::Private::fetchPayload( Item &item, ItemFetchJob *job ) const
{
  const QByteArray remoteId = item.remoteId().toUtf8();

  const MailInfoMap::const_iterator findIt = mMailInfos.constFind( remoteId );
  if ( findIt == mMailInfos.constEnd() ) {
    // TODO logging
    // TODO better message
      const QString message = i18n( "Item remote ID does not address any message in this store" );
    mJobSession->setError( job, Job::InvalidJobContext, message );
    return false;
  }

  const QSet<QByteArray> parts = job->fetchScope().payloadParts();
  if ( job->fetchScope().fullPayload() || parts.contains( Akonadi::MessagePart::Body ) ) {
    KMime::Message *mail = mMailBox->readEntry( findIt->offset );
    if ( mail == 0 ) {
      // TODO logging
      // TODO better message
      const QString message = i18n( "Item remote ID does not address any message in this store" );
      mJobSession->setError( job, Job::InvalidJobContext, message );
      return false;
    }

    KMime::Message::Ptr messagePtr( mail );
    item.setPayload<KMime::Message::Ptr>( messagePtr );
  } else if ( parts.contains( Akonadi::MessagePart::Envelope ) ||
              parts.contains( Akonadi::MessagePart::Header )) {
    // TODO check whether MBox::readEntryHeaders() is appropriate for both part types
    item.setPayload<KMime::Message::Ptr>( findIt->headers );
  }

  return true;
}

bool AugmentedMBoxStore::Private::fetchAttributes( Item &item, ItemFetchJob *job ) const
{
  Q_UNUSED( job );

  const KConfigGroup indexGroup( mIndex, item.remoteId() );
  if ( indexGroup.isValid() ) {
    const KConfigGroup attributeGroup( &indexGroup, QLatin1String( "attributes" ) );
    if ( attributeGroup.isValid() ) {
      QSet<QString> keySet;
      if ( job->fetchScope().allAttributes() ) {
        keySet = QSet<QString>::fromList( attributeGroup.keyList() );
      } else {
        const QSet<QByteArray> attributes = job->fetchScope().attributes();
        foreach ( const QByteArray &attribute, attributes ) {
          const QString key = QString::fromUtf8( attribute );
          if ( attributeGroup.hasKey( key ) ) {
            keySet.insert( key );
          }
        }
      }

      foreach ( const QString &key, keySet ) {
        Akonadi::Attribute *attribute = Akonadi::AttributeFactory::createAttribute( key.toUtf8() );
        if ( attribute != 0 ) {
          const QByteArray data = attributeGroup.readEntry( key, QByteArray() );
          if ( data.isEmpty() ) {
            kWarning() << "Attribute" << key << "has empty data";
          }
          attribute->deserialize( data );
          item.addAttribute( attribute );
        } else {
          kWarning() << "Failed to create attribute" << key;
        }
      }
    }
  }

  return true;
}

bool AugmentedMBoxStore::Private::fetchFlags( Item &item, ItemFetchJob *job ) const
{
  Q_UNUSED( job );

  const KConfigGroup indexGroup( mIndex, item.remoteId() );
  if ( indexGroup.isValid() ) {
    const QList<QByteArray> values = indexGroup.readEntry( QLatin1String( "Flags" ),QList<QByteArray>() );

    item.setFlags( Akonadi::Item::Flags::fromList( values ) );
  }

  return true;
}

void AugmentedMBoxStore::Private::processItemCreate( ItemCreateJob *job )
{
  if ( mMailInfos.isEmpty() ) {
    buildMailInfoMap();
  }

  Akonadi::Item item = job->item();
  if ( !item.remoteId().isEmpty() ) {
    kDebug() << "Remote ID of item to create is not empty, discarding it. Only the store assigns remote IDs";
    item.setRemoteId( QString() );
  }

  bool ok = true;
  ok = storePayload( item, job );
  ok = ok && storeAttributes( item, job );
  ok = ok && storeFlags( item, job );

  if ( ok ) {
    mIndex->sync();
    mMailBox->save();
    mJobSession->notifyItemCreated( job, item );
  } else {
    mIndex->reparseConfiguration();
  }

  mJobSession->emitResult( job );
}

void AugmentedMBoxStore::Private::processItemModify( ItemModifyJob *job )
{
  if ( mMailInfos.isEmpty() ) {
    buildMailInfoMap();
  }

  Akonadi::Item item = job->item();

  MailInfoMap::iterator findIt = mMailInfos.find( item.remoteId().toUtf8() );
  if ( findIt == mMailInfos.end() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File %1 does not contain a message with ID %2", mParent->fileName(), item.remoteId() );
    mJobSession->setError( job, Job::InvalidJobContext, message );
    mJobSession->emitResult( job );
    return;
  }

  bool ok = true;
  if ( !job->ignorePayload() ) {
    ok = storePayload( item, job );
  }
  ok = ok && storeAttributes( item, job );
  ok = ok && storeFlags( item, job );

  if ( ok ) {
    mIndex->sync();
    if ( !job->ignorePayload() ) {
      mMailBox->save();
    }
    mJobSession->notifyItemModified( job, item );
  } else {
    mIndex->reparseConfiguration();
  }

  mJobSession->emitResult( job );
}

void AugmentedMBoxStore::Private::processItemDelete( ItemDeleteJob *job )
{
  if ( mMailInfos.isEmpty() ) {
    buildMailInfoMap();
  }

  const Akonadi::Item item = job->item();

  MailInfoMap::iterator findIt = mMailInfos.find( item.remoteId().toUtf8() );
  if ( findIt == mMailInfos.end() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File %1 does not contain a message with ID %2", mParent->fileName(), item.remoteId() );
    mJobSession->setError( job, Job::InvalidJobContext, message );
    mJobSession->emitResult( job );
    return;
  }

  mDeleteOffsets << findIt->offset;
  mMailInfos.erase( findIt );

  // TODO when do purge?

  mJobSession->emitResult( job );
}

bool AugmentedMBoxStore::Private::storePayload( Item &item, Job *job )
{
  MailInfoMap::iterator findIt = mMailInfos.end();
  if ( !item.remoteId().isEmpty() ) {
    const QByteArray id = item.remoteId().toUtf8();
    findIt = mMailInfos.find( id );
    if ( findIt == mMailInfos.end() ) {
      // TODO logging
      // TODO better message
      const QString message = i18n( "Item remote ID is not valid" );
      mJobSession->setError( job, Job::InvalidJobContext, message );
      return false;
    }
  }

  const KMime::Message::Ptr messagePtr = item.payload<KMime::Message::Ptr>();
  qint64 offset = mMailBox->appendEntry( messagePtr );
  if ( offset == -1 ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "Could not append message" );
    // TODO better error code
    mJobSession->setError( job, Job::InvalidJobContext, message );
    return false;
  }

  if ( item.remoteId().isEmpty() ) {
    const QByteArray id = messagePtr->messageID()->identifier();
    item.setRemoteId( QString::fromUtf8( id ) );

    MailInfo mailInfo;
    findIt = mMailInfos.insert( id, mailInfo );
    findIt->headers = KMime::Message::Ptr( new KMime::Message() );
  } else {
    mDeleteOffsets << findIt->offset;
  }

  findIt->offset = offset;
  findIt->headers->setHead( messagePtr->head() );

  return true;
}

bool AugmentedMBoxStore::Private::storeAttributes( const Item &item, Job *job )
{
  Q_ASSERT( !item.remoteId().isEmpty() );
  Q_UNUSED( job );

  KConfigGroup indexGroup( mIndex, item.remoteId() );
  if ( indexGroup.isValid() ) {
    KConfigGroup attributeGroup( &indexGroup, QLatin1String( "attributes" ) );
    attributeGroup.deleteGroup();

    const Akonadi::Attribute::List attributes = item.attributes();
    foreach ( const Akonadi::Attribute *attribute, attributes ) {
      attributeGroup.writeEntry( QString::fromUtf8( attribute->type() ), attribute->serialized() );
    }
  }

  return true;
}

bool AugmentedMBoxStore::Private::storeFlags( const Item &item, Job *job )
{
  Q_ASSERT( !item.remoteId().isEmpty() );
  Q_UNUSED( job );

  KConfigGroup indexGroup( mIndex, item.remoteId() );
  if ( indexGroup.isValid() ) {
    indexGroup.writeEntry( QLatin1String( "Flags" ), item.flags().toList() );
  }

  return true;
}

void AugmentedMBoxStore::Private::processStoreCompact( StoreCompactJob *job )
{
  if ( !mDeleteOffsets.isEmpty() ) {
    mMailBox->purge( mDeleteOffsets );
    mDeleteOffsets.clear();
  }

  mJobSession->emitResult( job );
}

#include "augmentedmboxstore.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
