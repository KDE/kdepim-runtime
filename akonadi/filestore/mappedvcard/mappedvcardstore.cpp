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

#include "mappedvcardstore.h"

#include "collectionfetchjob.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemmodifyjob.h"
#include "sessionimpls_p.h"
#include "storecompactjob.h"

#include <kabc/addressee.h>
#include <kabc/vcardconverter.h>

#include <akonadi/cachepolicy.h>
#include <akonadi/collection.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/entitydisplayattribute.h>

#include <KDebug>
#include <KLocale>
#include <KSaveFile>

#include <QFile>
#include <QHash>

const char *sVCardSeparator = "\r\n\r\n";

using namespace Akonadi::FileStore;

class MappedVCardStore::MappedVCardStore::Private : public Job::Visitor
{
  public:
    Private( MappedVCardStore *parent )
      : mJobSession( 0 ),
        mFile( 0 ), mFileReadable( false ), mFileWritable( false ),
        mParent( parent )
    {
    }

    virtual ~Private()
    {
      delete mFile;
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

    mutable QFile *mFile;
    bool mFileReadable;
    bool mFileWritable;

    struct FilePosition
    {
      QString uid;
      qint64 begin;
      qint64 length;
      qint64 available;
    };
    typedef QHash<QString, FilePosition> FilePositionMap;
    FilePositionMap mFilePositions;

    uchar* mMappedFileContent;

  private:
    MappedVCardStore *mParent;
    KABC::VCardConverter mVCardConverter;

  private:
    void processItemFetch( ItemFetchJob *job );
    void processItemFetchAll( ItemFetchJob *job );
    void processItemFetchSingle( ItemFetchJob *job );

    void processItemCreate( ItemCreateJob *job );

    void processItemModify( ItemModifyJob *job );

    void processItemDelete( ItemDeleteJob *job );

    void buildFilePositionMap();

    void processStoreCompact( StoreCompactJob *job );

    static bool lessThanByBegin( const FilePosition &position1, const FilePosition &position2 )
    {
      return ( position1.begin < position2.begin );
    }
};

MappedVCardStore::MappedVCardStore( QObject *parent )
  : AbstractDirectAccessStore( parent ), d( new Private( this ) )
{
  d->mJobSession = new FiFoQueueJobSession( this );

  Akonadi::Collection collection;
  collection.setContentMimeTypes( QStringList() << KABC::Addressee::mimeType() );

  Akonadi::CachePolicy cachePolicy;
  cachePolicy.setSyncOnDemand( true );
  cachePolicy.setCacheTimeout( 10 );
  collection.setCachePolicy( cachePolicy );

  setTopLevelCollection( collection );

  QObject::connect( d->mJobSession, SIGNAL( jobsReady( const QList<Job*>& ) ),
                    this, SLOT( onJobsReady( const QList<Job*>& ) ) );
}

MappedVCardStore::~MappedVCardStore()
{
  delete d;
}

void MappedVCardStore::setFileName( const QString &fileName )
{
/*  kDebug() << "fileName=" << fileName << ", old=" << this->fileName();*/
  d->mJobSession->cancelAllJobs();

  d->mFilePositions.clear();
  delete d->mFile;

  AbstractDirectAccessStore::setFileName( fileName );

  d->mFile = new QFile( AbstractDirectAccessStore::fileName() );
  if ( d->mFile->open( QIODevice::ReadWrite ) ) {
    d->mFileReadable = true;
    d->mFileWritable = true;
  } else if ( d->mFile->open( QIODevice::ReadOnly ) ) {
    d->mFileReadable = true;
    d->mFileWritable = false;
  } else {
    d->mFileReadable = false;
    d->mFileWritable = false;
  }

  d->mMappedFileContent = d->mFile->map( 0, d->mFile->size() );
}

CollectionFetchJob *MappedVCardStore::fetchCollections( const Collection &collection, CollectionFetchJob::Type type, const CollectionFetchScope *fetchScope ) const
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

ItemFetchJob *MappedVCardStore::fetchItems( const Collection &collection, const ItemFetchScope *fetchScope ) const
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

ItemFetchJob *MappedVCardStore::fetchItem( const Item &item, const ItemFetchScope *fetchScope ) const
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

ItemCreateJob *MappedVCardStore::createItem( const Item &item, const Collection &collection )
{
  ItemCreateJob *job = new ItemCreateJob( item, collection, d->mJobSession );

  if ( !d->mFileWritable ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File not writable" );
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

  if ( !item.hasPayload<KABC::Addressee>() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "No vcard data available" );
    d->mJobSession->setError( job, Job::InvalidJobContext, message );
    return job;
  }

  const KABC::Addressee addressee = item.payload<KABC::Addressee>();
  if ( addressee.isEmpty() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "No vcard data available" );
    d->mJobSession->setError( job, Job::InvalidJobContext, message );
    return job;
  }

  if ( !item.remoteId().isEmpty() ) {
    // TODO warning logging, store generates the remoteId
  }

  return job;
}

ItemModifyJob *MappedVCardStore::modifyItem( const Item &item, bool ignorePayload )
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
    if ( !item.hasPayload<KABC::Addressee>() ) {
      // TODO logging
      // TODO better message
      const QString message = i18n( "No vcard data available" );
      d->mJobSession->setError( job, Job::InvalidJobContext, message );
      return job;
    }

    const KABC::Addressee addressee = item.payload<KABC::Addressee>();
    if ( addressee.isEmpty() ) {
      // TODO logging
      // TODO better message
      const QString message = i18n( "No vcard data available" );
      d->mJobSession->setError( job, Job::InvalidJobContext, message );
      return job;
    }
  }

  return job;
}

ItemDeleteJob *MappedVCardStore::deleteItem( const Item &item )
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

StoreCompactJob *MappedVCardStore::compactStore()
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

void MappedVCardStore::Private::processItemFetch( ItemFetchJob *job )
{
  // there are no items in empty files
  if ( mFile->size() == 0 ) {
    mJobSession->emitResult( job );
    return;
  }

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

void MappedVCardStore::Private::processItemFetchAll( ItemFetchJob *job )
{
  Akonadi::Item::List items;

  if ( job->fetchScope().fullPayload() ) {
    const QByteArray fileData( reinterpret_cast<char*>( mMappedFileContent ),
                               static_cast<int>( mFile->size() ) );
    const KABC::Addressee::List addressees = mVCardConverter.parseVCards( fileData );

    foreach ( const KABC::Addressee &addressee, addressees ) {
      Item item( KABC::Addressee::mimeType() );
      item.setRemoteId( addressee.uid() );
      item.setParentCollection( mParent->topLevelCollection() );
      item.setPayload<KABC::Addressee>( addressee );

      items << item;
    }
  } else {
    if ( mFilePositions.isEmpty() ) {
      buildFilePositionMap();
    }

    FilePositionMap::const_iterator it    = mFilePositions.constBegin();
    FilePositionMap::const_iterator endIt = mFilePositions.constEnd();
    for ( ; it != endIt; ++it ) {
      Item item( KABC::Addressee::mimeType() );
      item.setRemoteId( it.key() );
      item.setParentCollection( mParent->topLevelCollection() );

      items << item;
    }
  }

  mJobSession->notifyItemsReceived( job, items );
  mJobSession->emitResult( job );
}

void MappedVCardStore::Private::processItemFetchSingle( ItemFetchJob *job )
{
  if ( mFilePositions.isEmpty() ) {
    buildFilePositionMap();
  }

  const QString remoteId = job->item().remoteId();

  const FilePositionMap::const_iterator findIt = mFilePositions.constFind( remoteId );
  if ( findIt == mFilePositions.constEnd() ) {
    // TODO better message
    const QString message = i18n( "File %1 does not contain a vcard with ID %2", mParent->fileName(), remoteId );
    mJobSession->setError( job, Job::InvalidJobContext, message );
    mJobSession->emitResult( job );
    return;
  }

  Akonadi::Item item( job->item() );
  item.setParentCollection( mParent->topLevelCollection() );

  if ( job->fetchScope().fullPayload() ) {
    const QByteArray vcard( reinterpret_cast<char*>( mMappedFileContent ) + findIt->begin,
                            static_cast<int>( findIt->length ) );

    const KABC::Addressee addressee = mVCardConverter.parseVCard( vcard );
    Q_ASSERT( !addressee.isEmpty() );

    item.setPayload<KABC::Addressee>( addressee );
  }

  Akonadi::Item::List items;
  items << item;

  mJobSession->notifyItemsReceived( job, items );
  mJobSession->emitResult( job );
}

void MappedVCardStore::Private::processItemCreate( ItemCreateJob *job )
{
  if ( mFilePositions.isEmpty() ) {
    buildFilePositionMap();
  }

  Akonadi::Item item = job->item();

  const KABC::Addressee addressee = item.payload<KABC::Addressee>();
  if ( mFilePositions.constFind( addressee.uid() ) != mFilePositions.constEnd() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File %1 already contains a vcard with ID %2", mParent->fileName(), addressee.uid() );
    mJobSession->setError( job, Job::InvalidJobContext, message );
    mJobSession->emitResult( job );
    return;
  }

  item.setRemoteId( addressee.uid() );

  const QByteArray vcard = mVCardConverter.createVCard( addressee );

  qint64 oldSize = mFile->size();
  if ( !mFile->resize( oldSize + vcard.size() ) ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File %1 could not be resized for new vcard", mParent->fileName() );
    // TODO new enum value
    mJobSession->setError( job, Job::InvalidJobContext, message );
    mJobSession->emitResult( job );
    return;
  }

  FilePosition filePosition;
  filePosition.begin = oldSize;
  filePosition.length = vcard.size();
  filePosition.available = vcard.size();

  mFilePositions.insert( addressee.uid(), filePosition );

  mFile->unmap( mMappedFileContent );

  mFile->seek( oldSize );
  mFile->write( vcard );
  mFile->flush();

  mMappedFileContent = mFile->map( 0, mFile->size() );

/*  QByteArray fileData( reinterpret_cast<char*>( mMappedFileContent ),
                       static_cast<int>( mFile->size() ) );
  fileData.replace( filePosition.begin, vcard.size(), vcard );*/

  mJobSession->notifyItemCreated( job, item );
  mJobSession->emitResult( job );
}

void MappedVCardStore::Private::processItemModify( ItemModifyJob *job )
{
  const Akonadi::Item item = job->item();
  if ( job->ignorePayload() ) {
    kWarning() << "ItemModifyJob with ignorePayload: this store does not modify anything else";
    mJobSession->notifyItemModified( job, item );
    mJobSession->emitResult( job );
    return;
  }

  if ( mFilePositions.isEmpty() ) {
    buildFilePositionMap();
  }

  FilePositionMap::iterator findIt = mFilePositions.find( item.remoteId() );
  if ( findIt == mFilePositions.end() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File %1 does not contain a vcard with ID %2", mParent->fileName(), item.remoteId() );
    mJobSession->setError( job, Job::InvalidJobContext, message );
    mJobSession->emitResult( job );
    return;
  }

  const KABC::Addressee addressee = item.payload<KABC::Addressee>();
  const QByteArray vcard = mVCardConverter.createVCard( addressee );

  if ( vcard.size() > findIt->available ) {
    const bool atEnd = ( findIt->begin + findIt->available ) == mFile->size();

    qint64 oldSize = mFile->size();
    qint64 newSize = ( atEnd ? findIt->begin : mFile->size() ) + vcard.size();
    if ( !mFile->resize( newSize ) ) {
      // TODO logging
      // TODO better message
      const QString message = i18n( "File %1 could not be resized for modified vcard", mParent->fileName() );
      // TODO new enum value
      mJobSession->setError( job, Job::InvalidJobContext, message );
      mJobSession->emitResult( job );
      return;
    }

    if ( !atEnd ) {
      // find the FilePosition entry before the current one
      FilePositionMap::iterator posIt    = mFilePositions.begin();
      FilePositionMap::iterator posEndIt = mFilePositions.end();
      for ( ; posIt != posEndIt; ++posIt ) {
        if ( ( posIt->begin + posIt->available ) == findIt->begin ) {
          posIt->available += findIt->available;
          break;
        }
      }

      mFile->seek( findIt->begin );
      const QByteArray fill( findIt->available, '\n' );
      mFile->write( fill );

      findIt->begin = oldSize;
    }

    findIt->length = vcard.size();
    findIt->available = vcard.size();

    mFile->unmap( mMappedFileContent );

    mFile->seek( findIt->begin );
    mFile->write( vcard );
    mFile->flush();

    mMappedFileContent = mFile->map( 0, mFile->size() );

  } else {
    qint64 remaining = findIt->available - vcard.size();

    findIt->length = vcard.size();

    const QByteArray fill( remaining, '\n' );

    mFile->seek( findIt->begin );
    mFile->write( vcard );
    if ( fill.size() > 0 ) mFile->write( fill );
    mFile->flush();
  }

  mJobSession->emitResult( job );
}

void MappedVCardStore::Private::processItemDelete( ItemDeleteJob *job )
{
  if ( mFilePositions.isEmpty() ) {
    buildFilePositionMap();
  }

  const Akonadi::Item item = job->item();

  FilePositionMap::iterator findIt = mFilePositions.find( item.remoteId() );
  if ( findIt == mFilePositions.end() ) {
    // TODO logging
    // TODO better message
    const QString message = i18n( "File %1 does not contain a vcard with ID %2", mParent->fileName(), item.remoteId() );
    mJobSession->setError( job, Job::InvalidJobContext, message );
    mJobSession->emitResult( job );
    return;
  }

  if ( findIt->begin != 0 ) {
    // find the FilePosition entry before the current one
    FilePositionMap::iterator posIt    = mFilePositions.begin();
    FilePositionMap::iterator posEndIt = mFilePositions.end();
    for ( ; posIt != posEndIt; ++posIt ) {
      if ( ( posIt->begin + posIt->available ) == findIt->begin ) {
        posIt->available += findIt->available;
        break;
      }
    }
  }

  const QByteArray fill( findIt->available, '\n' );

  mFile->seek( findIt->begin );
  mFile->write( fill );
  mFile->flush();

  mFilePositions.erase( findIt );

  mJobSession->emitResult( job );
}

// TODO errors instead of break?
void MappedVCardStore::Private::buildFilePositionMap()
{
  const QByteArray fileData( reinterpret_cast<char*>( mMappedFileContent ),
                             static_cast<int>( mFile->size() ) );

  int begin = fileData.indexOf( "BEGIN:VCARD" );
  int end = 0;
  while ( begin != -1 ) {
    end = fileData.indexOf( "END:VCARD", begin );
    if ( end == -1 ) {
      break;
    }

    const int length = end - begin + 9;

    int uidBegin = fileData.indexOf( "\nUID:", begin );
    if ( uidBegin == -1 || uidBegin > end ) {
      break;
    }

    uidBegin += 5;
    int uidEnd = fileData.indexOf( '\n', uidBegin );
    if ( uidEnd == -1 || uidEnd > end ) {
      break;
    }

    if ( fileData[ uidEnd - 1] == '\r' ) {
      --uidEnd;
    }

    const QByteArray uid = fileData.mid( uidBegin, ( uidEnd - uidBegin ) );

    FilePosition filePosition;
    filePosition.uid = QString::fromUtf8( uid );
    filePosition.begin = begin;
    filePosition.length = length;

    begin = fileData.indexOf( "BEGIN:VCARD", end );
    if ( begin != -1 ) {
      filePosition.available = begin - filePosition.begin;
    } else {
      filePosition.available = mFile->size() - filePosition.begin;
    }

/*    kDebug() << "addressee" << addressee.uid() << "at position=" << filePosition.begin
             << "length=" << filePosition.length
             << ", available=" << filePosition.available;*/

    mFilePositions.insert( filePosition.uid, filePosition );
  }
}

void MappedVCardStore::Private::processStoreCompact( StoreCompactJob *job )
{
  KSaveFile saveFile;
  saveFile.setFileName( mParent->fileName() );

  saveFile.open( QIODevice::WriteOnly );

  QList<FilePosition> filePositions = mFilePositions.values();
  qSort( filePositions.begin(), filePositions.end(), lessThanByBegin );

  FilePositionMap newFilePositions;
  quint64 offset = 0;
  foreach ( const FilePosition &filePosition, filePositions ) {
    const QByteArray vcard( reinterpret_cast<char*>( mMappedFileContent ) + filePosition.begin,
                            static_cast<int>( filePosition.length ) );

    if ( saveFile.write( vcard ) < filePosition.length ||
         saveFile.write( sVCardSeparator ) < 4 ) {
      // TODO logging
      // TODO better error code
      mJobSession->setError( job, Job::InvalidStoreState, saveFile.errorString() );
      mJobSession->emitResult( job );
      saveFile.abort();
      return;
    }

    FilePosition newFilePosition;
    newFilePosition.uid = filePosition.uid;
    newFilePosition.begin = offset;
    newFilePosition.length = filePosition.length;
    newFilePosition.available = filePosition.length + 4;

    newFilePositions.insert( newFilePosition.uid, newFilePosition );
    offset += filePosition.length + 4;
  }

  mFile->close();
  if ( !saveFile.finalize() ) {
    // TODO logging
    // TODO better error code
    mJobSession->setError( job, Job::InvalidStoreState, saveFile.errorString() );
    mJobSession->emitResult( job );
  } else {
    mFilePositions = newFilePositions;
  }

  if ( mFile->open( QIODevice::ReadWrite ) ) {
    mFileReadable = true;
    mFileWritable = true;
  } else if ( mFile->open( QIODevice::ReadOnly ) ) {
    mFileReadable = true;
    mFileWritable = false;
  } else {
    mFileReadable = false;
    mFileWritable = false;
  }

  mMappedFileContent = mFile->map( 0, mFile->size() );

  mJobSession->emitResult( job );
}

#include "mappedvcardstore.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
