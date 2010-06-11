/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

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

#include "mixedmaildirstore.h"

#include "kmindexreader/kmindexreader.h"

#include "filestore/collectioncreatejob.h"
#include "filestore/collectiondeletejob.h"
#include "filestore/collectionfetchjob.h"
#include "filestore/collectionmovejob.h"
#include "filestore/collectionmodifyjob.h"
#include "filestore/entitycompactchangeattribute.h"
#include "filestore/itemcreatejob.h"
#include "filestore/itemdeletejob.h"
#include "filestore/itemfetchjob.h"
#include "filestore/itemmodifyjob.h"
#include "filestore/itemmovejob.h"
#include "filestore/storecompactjob.h"

#include "libmaildir/maildir.h"
#include "libmbox/mbox.h"

#include <kmime/kmime_message.h>

#include <akonadi/kmime/messageparts.h>

#include <akonadi/cachepolicy.h>
#include <akonadi/itemfetchscope.h>

#include <kpimutils/kfileio.h>

#include <KLocale>

#include <QDir>
#include <QFileInfo>
#include <QStringList>

using namespace Akonadi;
using KPIM::Maildir;

static bool indexFileForFolder( const QFileInfo &folderDirInfo, QFileInfo &indexFileInfo )
{
  indexFileInfo = QFileInfo( folderDirInfo.dir(), QString::fromUtf8( ".%1.index" ).arg( folderDirInfo.fileName() ) );

  if ( !indexFileInfo.exists() || !indexFileInfo.isReadable() ) {
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "No index file" << indexFileInfo.absoluteFilePath() << "or not readable";
    return false;
  }

  return true;
}

class MBoxContext
{
  public:
    MBoxContext() : mIndexDataLoaded( false ) {}

    QString fileName() const
    {
      return mMBox.fileName();
    }

    bool load( const QString &fileName )
    {
      mDeletedOffsets.clear();
      return mMBox.load( fileName );
    }

    QList<MsgEntryInfo> entryList() const
    {
      return mMBox.entryList();
    }

    QByteArray readRawEntry( quint64 offset )
    {
      return mMBox.readRawEntry( offset );
    }

    QByteArray readEntryHeaders( quint64 offset )
    {
      return mMBox.readEntryHeaders( offset );
    }

    KMime::Message *readEntry( quint64 offset )
    {
      return mMBox.readEntry( offset );
    }

    qint64 appendEntry( const MessagePtr &entry )
    {
      return mMBox.appendEntry( entry );
    }

    void deleteEntry( quint64 offset )
    {
      mDeletedOffsets << offset;
    }

    bool save()
    {
      return mMBox.save();
    }

    int purge( QList<MsgInfo> &movedEntries )
    {
      const int deleteCount = mDeletedOffsets.count();
      const bool result = mMBox.purge( mDeletedOffsets, &movedEntries );

      IndexDataHash indexData = mIndexData;
      Q_FOREACH( quint64 offset, mDeletedOffsets ) {
        indexData.remove( offset );
      }
      mDeletedOffsets.clear();

      Q_FOREACH( const MsgInfo &entry, movedEntries ) {
        const KMIndexDataPtr data = indexData.take( entry.first );
        indexData.insert( entry.second, data );
      }

      return ( result ? deleteCount : -1 );
    }

    MBox &mbox()
    {
      return mMBox;
    }

    const MBox &mbox() const
    {
      return mMBox;
    }

    void addDeletedOffset( quint64 offset )
    {
      mDeletedOffsets << offset;
    }

    void readIndexData();

    KMIndexDataPtr indexData( quint64 offset ) const {
      if ( !mDeletedOffsets.contains( offset ) ) {
        IndexDataHash::const_iterator it = mIndexData.constFind( offset );
        if ( it != mIndexData.constEnd() ) {
          return it.value();
        }
      }

      return KMIndexDataPtr();
    }

    bool hasIndexData() const {
      return !mIndexData.isEmpty();
    }

  public:
    Collection mCollection;

  private:
    QSet<quint64> mDeletedOffsets;
    MBox mMBox;

    typedef QHash<quint64, KMIndexDataPtr> IndexDataHash;
    IndexDataHash mIndexData;
    bool mIndexDataLoaded;
};

typedef boost::shared_ptr<MBoxContext> MBoxPtr;

void MBoxContext::readIndexData()
{
  if ( mIndexDataLoaded ) {
    return;
  }

  mIndexDataLoaded = true;

  const QFileInfo mboxFileInfo( mMBox.fileName() );
  QFileInfo indexFileInfo;
  if ( !indexFileForFolder( mboxFileInfo, indexFileInfo ) ) {
    return;
  }

  if ( mboxFileInfo.lastModified() > indexFileInfo.lastModified() ) {
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "MBox file newer than the index: mbox modified at"
                                     << mboxFileInfo.lastModified() << ", index modified at"
                                     << indexFileInfo.lastModified();
    return;
  }

  KMIndexReader indexReader( indexFileInfo.absoluteFilePath() );
  if ( indexReader.error() || !indexReader.readIndex() ) {
    kError() << "Index file" << indexFileInfo.path() << "could not be read";
    return;
  }

  const QList<MsgEntryInfo> entries = mMBox.entryList();
  Q_FOREACH( const MsgEntryInfo &entry, entries ) {
    const quint64 indexOffset = entry.offset + entry.separatorSize;
    const KMIndexDataPtr data = indexReader.dataByOffset( indexOffset );
    if ( data != 0 ) {
      mIndexData.insert( entry.offset, data );
    }
  }
}

class MaildirContext
{
  public:
    MaildirContext( const QString &path, bool isTopLevel )
      : mMaildir( path, isTopLevel ), mIndexDataLoaded( false )
    {
    }

    MaildirContext( const Maildir &md )
      : mMaildir( md ), mIndexDataLoaded( false )
    {
    }

    QStringList entryList() const {
      return mMaildir.entryList();
    }

    Maildir mailDir() const {
      return mMaildir;
    }

    void readIndexData();

    KMIndexDataPtr indexData( const QString &fileName ) const {
      IndexDataHash::const_iterator it = mIndexData.constFind( fileName );
      if ( it != mIndexData.constEnd() ) {
        return it.value();
      }

      return KMIndexDataPtr();
    }

    bool hasIndexData() const {
      return !mIndexData.isEmpty();
    }

  private:
    Maildir mMaildir;

    typedef QHash<QString, KMIndexDataPtr> IndexDataHash;
    IndexDataHash mIndexData;
    bool mIndexDataLoaded;
};

void MaildirContext::readIndexData()
{
  if ( mIndexDataLoaded ) {
    return;
  }

  mIndexDataLoaded = true;

  const QFileInfo maildirFileInfo( mMaildir.path() );
  QFileInfo indexFileInfo;
  if ( !indexFileForFolder( maildirFileInfo, indexFileInfo ) ) {
    return;
  }

  const QDir maildirBaseDir( maildirFileInfo.absoluteFilePath() );
  const QFileInfo curDirFileInfo( maildirBaseDir, QLatin1String( "cur" ) );
  const QFileInfo newDirFileInfo( maildirBaseDir, QLatin1String( "new" ) );

  if ( curDirFileInfo.lastModified() > indexFileInfo.lastModified() ) {
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Maildir \"cur\" directory newer than the index: cur modified at"
                                     << curDirFileInfo.lastModified() << ", index modified at"
                                     << indexFileInfo.lastModified();
    return;
  }
  if ( newDirFileInfo.lastModified() > indexFileInfo.lastModified() ) {
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Maildir \"new\" directory newer than the index: cur modified at"
                                     << newDirFileInfo.lastModified() << ", index modified at"
                                     << indexFileInfo.lastModified();
    return;
  }


  KMIndexReader indexReader( indexFileInfo.absoluteFilePath() );
  if ( indexReader.error() || !indexReader.readIndex() ) {
    kError() << "Index file" << indexFileInfo.path() << "could not be read";
    return;
  }

  const QStringList entries = mMaildir.entryList();
  Q_FOREACH( const QString &entry, entries ) {
    const KMIndexDataPtr data = indexReader.dataByFileName( entry );
    if ( data != 0 ) {
      mIndexData.insert( entry, data );
    }
  }
}

typedef boost::shared_ptr<MaildirContext> MaildirPtr;

class MixedMaildirStore::Private : public FileStore::Job::Visitor
{
  MixedMaildirStore *const q;

  public:
    enum FolderType
    {
      InvalidFolder,
      TopLevelFolder,
      MaildirFolder,
      MBoxFolder
    };

    Private( MixedMaildirStore *parent ) : q( parent )
    {
    }

    FolderType folderForCollection( const Collection &col, QString &path, QString &errorText ) const;

    void fillMBoxCollectionDetails( const MBoxPtr &mbox, Collection &collection );
    void fillMaildirCollectionDetails( const Maildir &md, Collection &collection );
    void fillMaildirTreeDetails( const Maildir &md, const Collection &collection, Collection::List &collections, bool recurse );
    void listCollection( FileStore::Job *job, MBoxPtr &mbox, const Collection &collection, Item::List &items );
    void listCollection( FileStore::Job *job, MaildirPtr &md, const Collection &collection, Item::List &items );
    bool fillItem( MBoxPtr &mbox, bool includeBody, Item &item ) const;
    bool fillItem( const Maildir &md, bool includeBody, Item &item ) const;

  public: // visitor interface implementation
    bool visit( FileStore::Job *job );
    bool visit( FileStore::CollectionCreateJob *job );
    bool visit( FileStore::CollectionDeleteJob *job );
    bool visit( FileStore::CollectionFetchJob *job );
    bool visit( FileStore::CollectionModifyJob *job );
    bool visit( FileStore::CollectionMoveJob *job );
    bool visit( FileStore::ItemCreateJob *job );
    bool visit( FileStore::ItemDeleteJob *job );
    bool visit( FileStore::ItemFetchJob *job );
    bool visit( FileStore::ItemModifyJob *job );
    bool visit( FileStore::ItemMoveJob *job );
    bool visit( FileStore::StoreCompactJob *job );

  public:
    typedef QHash<QString, MBoxPtr> MBoxHash;
    MBoxHash mMBoxes;

    typedef QHash<QString, MaildirPtr> MaildirHash;
    MaildirHash mMaildirs;
};

MixedMaildirStore::Private::FolderType MixedMaildirStore::Private::folderForCollection( const Collection &col, QString &path, QString &errorText ) const
{
  path = QString();
  errorText = QString();

  if ( col.remoteId().isEmpty() ) {
    errorText = i18nc( "@info:status", "Given folder name is empty" );
    return InvalidFolder;
  }

  if ( col.parentCollection() == Collection::root() ) {
    kWarning( col.remoteId() != q->path() ) << "RID mismatch, is " << col.remoteId() << " expected " << q->path();
    path = q->path();
    return TopLevelFolder;
  }

  FolderType type = folderForCollection( col.parentCollection(), path, errorText );
  switch ( type ) {
    case InvalidFolder: return InvalidFolder;

    case TopLevelFolder: // fall through
    case MaildirFolder: {
      const Maildir parentMd( path, type == TopLevelFolder );
      const Maildir subFolder = parentMd.subFolder( col.remoteId() );
      if ( subFolder.isValid() ) {
        path = subFolder.path();
        return MaildirFolder;
      }

      const QString subDirPath =
        (type == TopLevelFolder ? path : Maildir::subDirPathForFolderPath( path ) );
      QFileInfo fileInfo( QDir( subDirPath ), col.remoteId() );
      if ( fileInfo.isFile() ) {
        path = fileInfo.absoluteFilePath();
        return MBoxFolder;
      }

      errorText = i18nc( "@info:status", "Folder %1 does not seem to be a valid email folder", fileInfo.absoluteFilePath() );
      return InvalidFolder;
    }

    case MBoxFolder: {
      const QString subDirPath = Maildir::subDirPathForFolderPath( path );
      QFileInfo fileInfo( QDir( subDirPath ), col.remoteId() );

      if ( fileInfo.isFile() ) {
        path = fileInfo.absoluteFilePath();
        return MBoxFolder;
      }

      const Maildir subFolder( fileInfo.absoluteFilePath(), false );
      if ( subFolder.isValid() ) {
        path = subFolder.path();
        return MaildirFolder;
      }

      errorText = i18nc( "@info:status", "Folder %1 does not seem to be a valid email folder", fileInfo.absoluteFilePath() );
      return InvalidFolder;
    }
  }
}

void MixedMaildirStore::Private::fillMBoxCollectionDetails( const MBoxPtr &mbox, Collection &collection )
{
  collection.setContentMimeTypes( QStringList() << Collection::mimeType()
                                                << KMime::Message::mimeType() );

  const QFileInfo fileInfo( mbox->fileName() );
  if ( fileInfo.isWritable() ) {
    collection.setRights( Collection::CanCreateItem |
                          Collection::CanChangeItem |
                          Collection::CanDeleteItem |
                          Collection::CanCreateCollection |
                          Collection::CanChangeCollection |
                          Collection::CanDeleteCollection );
  } else {
    collection.setRights( Collection::ReadOnly );
  }
}

void MixedMaildirStore::Private::fillMaildirCollectionDetails( const Maildir &md, Collection &collection )
{
  collection.setContentMimeTypes( QStringList() << Collection::mimeType()
                                                << KMime::Message::mimeType() );

  const QFileInfo fileInfo( md.path() );
  if ( fileInfo.isWritable() ) {
    collection.setRights( Collection::CanCreateItem |
                          Collection::CanChangeItem |
                          Collection::CanDeleteItem |
                          Collection::CanCreateCollection |
                          Collection::CanChangeCollection |
                          Collection::CanDeleteCollection );
  } else {
    collection.setRights( Collection::ReadOnly );
  }
}

void MixedMaildirStore::Private::fillMaildirTreeDetails( const Maildir &md, const Collection &collection, Collection::List &collections, bool recurse )
{
  if ( md.path().isEmpty() ) {
    return;
  }

  const QStringList maildirSubFolders = md.subFolderList();
  Q_FOREACH( const QString &subFolder, maildirSubFolders ) {
    const Maildir subMd = md.subFolder( subFolder );

    if ( !mMaildirs.contains( subMd.path() ) ) {
      const MaildirPtr mdPtr = MaildirPtr( new MaildirContext( subMd ) );
      mMaildirs.insert( subMd.path(), mdPtr );
    }

    Collection col;
    col.setRemoteId( subFolder );
    col.setName( subFolder );
    col.setParentCollection( collection );
    fillMaildirCollectionDetails( subMd, col );
    collections << col;

    if ( recurse ) {
      fillMaildirTreeDetails( subMd, col, collections, true );
    }
  }

  const QDir dir( md.isRoot() ? md.path() : Maildir::subDirPathForFolderPath( md.path() ) );
  const QFileInfoList fileInfos = dir.entryInfoList( QDir::Files );
  Q_FOREACH( const QFileInfo &fileInfo, fileInfos ) {
    if ( fileInfo.isHidden() || !fileInfo.isReadable() ) {
      continue;
    }

    const QString mboxPath = fileInfo.absoluteFilePath();

    MBoxPtr mbox;
    MBoxHash::const_iterator mboxIt = mMBoxes.constFind( mboxPath );
    if ( mboxIt == mMBoxes.constEnd() ) {
      mbox = MBoxPtr( new MBoxContext );
      mMBoxes.insert( mboxPath, mbox );
    } else {
      mbox = mboxIt.value();
    }

    if ( mbox->load( mboxPath ) ) {
      const QString subFolder = fileInfo.fileName();
      Collection col;
      col.setRemoteId( subFolder );
      col.setName( subFolder );
      col.setParentCollection( collection );
      mbox->mCollection = col;

      fillMBoxCollectionDetails( mbox, col );
      collections << col;

      if ( recurse ) {
        const QString subDirPath = Maildir::subDirPathForFolderPath( fileInfo.absoluteFilePath() );
        const Maildir subMd( subDirPath, true );
        fillMaildirTreeDetails( subMd, col, collections, true );
      }
    } else {
      mMBoxes.remove( fileInfo.absoluteFilePath() );
    }
  }
}

void MixedMaildirStore::Private::listCollection( FileStore::Job *job, MBoxPtr &mbox, const Collection &collection, Item::List &items )
{
  mbox->readIndexData();

  QHash<QString, QVariant> uidHash;
  QHash<QString, QVariant> tagListHash;

  const QList<MsgEntryInfo> entryList = mbox->entryList();
  Q_FOREACH( const MsgEntryInfo &entry, entryList ) {
    Item item;
    item.setMimeType( KMime::Message::mimeType() );
    item.setRemoteId( QString::number( entry.offset ) );
    item.setParentCollection( collection );

    if ( mbox->hasIndexData() ) {
      const KMIndexDataPtr indexData = mbox->indexData( entry.offset );
      if ( indexData != 0 ) {
        item.setFlags( indexData->status().getStatusFlags() );

        quint64 uid = indexData->uid();
        if ( uid != 0 ) {
          kDebug() << "item" << item.remoteId() << "has UID" << uid;
          uidHash.insert( item.remoteId(), QString::number( uid ) );
        }

        const QStringList tagList = indexData->tagList();
        if ( !tagList.isEmpty() ) {
          kDebug() << "item" << item.remoteId() << "has"
                   << tagList.count() << "tags:" << tagList;
          tagListHash.insert( item.remoteId(), tagList );
        }
      }
    }

    items << item;
  }

  if ( mbox->hasIndexData() ) {
    QVariant var = QVariant::fromValue< QHash<QString, QVariant> >( uidHash );
    job->setProperty( "remoteIdToIndexUid", var );

    var = QVariant::fromValue< QHash<QString, QVariant> >( tagListHash );
    job->setProperty( "remoteIdToTagList", var );
  }
}

void MixedMaildirStore::Private::listCollection( FileStore::Job *job, MaildirPtr &md, const Collection &collection, Item::List &items )
{
  md->readIndexData();

  QHash<QString, QVariant> uidHash;
  QHash<QString, QVariant> tagListHash;

  const QStringList entryList = md->entryList();
  Q_FOREACH( const QString &entry, entryList ) {
    Item item;
    item.setMimeType( KMime::Message::mimeType() );
    item.setRemoteId( entry );
    item.setParentCollection( collection );

    if ( md->hasIndexData() ) {
      const KMIndexDataPtr indexData = md->indexData( entry );
      if ( indexData != 0 ) {
        item.setFlags( indexData->status().getStatusFlags() );

        const quint64 uid = indexData->uid();
        if ( uid != 0 ) {
          kDebug() << "item" << item.remoteId() << "has UID" << uid;
          uidHash.insert( item.remoteId(), QString::number( uid ) );
        }

        const QStringList tagList = indexData->tagList();
        if ( !tagList.isEmpty() ) {
          kDebug() << "item" << item.remoteId() << "has"
                   << tagList.count() << "tags:" << tagList;
          tagListHash.insert( item.remoteId(), tagList );
        }
      }
    }

    items << item;
  }

  if ( md->hasIndexData() ) {
    QVariant var = QVariant::fromValue< QHash<QString, QVariant> >( uidHash );
    job->setProperty( "remoteIdToIndexUid", var );

    var = QVariant::fromValue< QHash<QString, QVariant> >( tagListHash );
    job->setProperty( "remoteIdToTagList", var );
  }
}

bool MixedMaildirStore::Private::fillItem( MBoxPtr &mbox, bool includeBody, Item &item ) const
{
//  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Filling item" << item.remoteId() << "from MBox: includeBody=" << includeBody;
  KMime::Message *message = 0;
  if ( includeBody ) {
    bool ok = false;
    message = mbox->readEntry( item.remoteId().toLongLong( &ok ) );
    if ( !ok || message == 0 ) {
      return false;
    }
  } else {
    bool ok = false;
    const QByteArray data = mbox->readEntryHeaders( item.remoteId().toLongLong( &ok ) );
    if ( !ok ) {
      return false;
    }
    message = new KMime::Message();
    message->setHead( KMime::CRLFtoLF( data ) );
  }

  KMime::Message::Ptr messagePtr( message );
  item.setPayload<KMime::Message::Ptr>( messagePtr );
  return true;
}

bool MixedMaildirStore::Private::fillItem( const Maildir &md, bool includeBody, Item &item ) const
{
//  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Filling item" << item.remoteId() << "from Maildir: includeBody=" << includeBody;
  KMime::Message::Ptr messagePtr( new KMime::Message() );

  if ( includeBody ) {
    const QByteArray data = md.readEntry( item.remoteId() );
    if ( data.isEmpty() ) {
      return false;
    }

    messagePtr->setContent( KMime::CRLFtoLF( data ) );
  } else {
    const QByteArray data = md.readEntryHeaders( item.remoteId() );
    if ( data.isEmpty() ) {
      return false;
    }

    messagePtr->setHead( KMime::CRLFtoLF( data ) );
  }

  item.setPayload<KMime::Message::Ptr>( messagePtr );
  return true;
}

bool MixedMaildirStore::Private::visit( FileStore::Job *job )
{
  const QString message = i18nc( "@info:status", "Unhandled operation %1", job->metaObject()->className() );
  kError() << message;
  q->notifyError( FileStore::Job::InvalidJobContext, message );
  return false;
}

bool MixedMaildirStore::Private::visit( FileStore::CollectionCreateJob *job )
{
  QString path;
  QString errorText;

  const FolderType folderType = folderForCollection( job->targetParent(), path, errorText );
  if ( folderType == InvalidFolder ) {
    errorText = i18nc( "@info:status", "Cannot create folder %1 inside folder %2",
                        job->collection().name(), job->targetParent().name() );
    kError() << errorText << "FolderType=" << folderType;
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

  if ( folderType == MBoxFolder ) {
    const QString subDirPath = Maildir::subDirPathForFolderPath( path );
    const QDir dir( subDirPath );
    const QFileInfo dirInfo( dir, job->collection().name() );
    if ( dirInfo.exists() && !dirInfo.isDir() ) {
      errorText = i18nc( "@info:status", "Cannot create folder %1 inside folder %2",
                          job->collection().name(), job->targetParent().name() );
      kError() << errorText << "FolderType=" << folderType << ", dirInfo exists and it not a dir";
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }

    if ( !dir.mkpath( job->collection().name() ) ) {
      errorText = i18nc( "@info:status", "Cannot create folder %1 inside folder %2",
                          job->collection().name(), job->targetParent().name() );
      kError() << errorText << "FolderType=" << folderType << ", mkpath failed";
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }

    Maildir md( dirInfo.absoluteFilePath(), false );
    if ( !md.create() ) {
      errorText = i18nc( "@info:status", "Cannot create folder %1 inside folder %2",
                          job->collection().name(), job->targetParent().name() );
      kError() << errorText << "FolderType=" << folderType << ", maildir create failed";
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }

    const MaildirPtr mdPtr( new MaildirContext( md ) );
    mMaildirs.insert( md.path(), mdPtr );
  } else {
    Maildir parentMd( path, folderType == TopLevelFolder );
    if ( parentMd.addSubFolder( job->collection().name() ).isEmpty() ) {
      errorText = i18nc( "@info:status", "Cannot create folder %1 inside folder %2",
                          job->collection().name(), job->targetParent().name() );
      kError() << errorText << "FolderType=" << folderType;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }

    const Maildir md = parentMd.subFolder( job->collection().name() );
    const MaildirPtr mdPtr( new MaildirContext( md ) );
    mMaildirs.insert( md.path(), mdPtr );
  }

  Collection collection = job->collection();
  collection.setRemoteId( collection.name() );
  collection.setParentCollection( job->targetParent() );
  q->notifyCollectionsProcessed( Collection::List() << collection );
  return true;
}

bool MixedMaildirStore::Private::visit( FileStore::CollectionDeleteJob *job )
{
  QString path;
  QString errorText;

  const FolderType folderType = folderForCollection( job->collection(), path, errorText );
  if ( folderType == InvalidFolder ) {
    errorText = i18nc( "@info:status", "Cannot remove folder %1 from folder %2",
                        job->collection().name(), job->collection().parentCollection().name() );
    kError() << errorText << "FolderType=" << folderType;
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

  QString parentPath;
  const FolderType parentFolderType = folderForCollection( job->collection().parentCollection(), parentPath, errorText );
  if ( parentFolderType == InvalidFolder ) {
    errorText = i18nc( "@info:status", "Cannot remove folder %1 from folder %2",
                        job->collection().name(), job->collection().parentCollection().name() );
    kError() << errorText << "Parent FolderType=" << parentFolderType;
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

  bool parentIndexInvalidated = false;
  if ( parentFolderType == MBoxFolder ) {
    MBoxPtr mboxPtr;
    MBoxHash::const_iterator mboxIt = mMBoxes.constFind( parentPath );
    if ( mboxIt == mMBoxes.constEnd() ) {
      kError() << "Deleting folder" << path << "from parent MBox" << parentPath
               << "but there is no context for that MBox yet";

      mboxPtr = MBoxPtr( new MBoxContext );
      if ( !mboxPtr->load( parentPath ) ) {
        kError() << "Loading or parent MBox" << parentPath << "failed";
      }

      mMBoxes.insert( parentPath, mboxPtr );
    }

    mboxPtr->readIndexData();
    parentIndexInvalidated = mboxPtr->hasIndexData();
  } else if ( parentFolderType == MaildirFolder ) {
    MaildirPtr mdPtr;
    MaildirHash::const_iterator mdIt = mMaildirs.constFind( parentPath );
    if ( mdIt == mMaildirs.constEnd() ) {
      kError() << "Deleting folder" << path << "from parent Maildir" << parentPath
               << "but there is no context for that Maildir yet";

      mdPtr = MaildirPtr( new MaildirContext( parentPath, false ) );
      mMaildirs.insert( parentPath, mdPtr );
    }

    mdPtr->readIndexData();
    parentIndexInvalidated = mdPtr->hasIndexData();
  }

  if ( folderType == MBoxFolder ) {
    if ( !QFile::remove( path ) ) {
      errorText = i18nc( "@info:status", "Cannot remove folder %1 from folder %2",
                          job->collection().name(), job->collection().parentCollection().name() );
      kError() << errorText << "FolderType=" << folderType;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }
  } else {
    if ( !KPIMUtils::removeDirAndContentsRecursively( path ) ) {
      errorText = i18nc( "@info:status", "Cannot remove folder %1 from folder %2",
                          job->collection().name(), job->collection().parentCollection().name() );
      kError() << errorText << "FolderType=" << folderType;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }
  }

  const QString subDirPath = Maildir::subDirPathForFolderPath( path );
  KPIMUtils::removeDirAndContentsRecursively( subDirPath );

  if ( parentIndexInvalidated ) {
    const QVariant var = QVariant::fromValue<Collection::List>( Collection::List() << job->collection().parentCollection() );
    job->setProperty( "onDiskIndexInvalidated", var );
  }

  q->notifyCollectionsProcessed( Collection::List() << job->collection() );
  return true;
}

bool MixedMaildirStore::Private::visit( FileStore::CollectionFetchJob *job )
{
  QString path;
  QString errorText;
  const FolderType folderType = folderForCollection( job->collection(), path, errorText );

  if ( folderType == InvalidFolder ) {
    kError() << errorText << "collection:" << job->collection();
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

  Collection::List collections;
  Collection collection = job->collection();
  if ( job->type() == FileStore::CollectionFetchJob::Base ) {
    collection.setName( collection.remoteId() );
    if ( folderType == MBoxFolder ) {
      MBoxPtr mbox;
      MBoxHash::const_iterator findIt = mMBoxes.constFind( path );
      if ( findIt == mMBoxes.constEnd() ) {
        mbox = MBoxPtr( new MBoxContext );
        if ( !mbox->load( path ) ) {
          errorText = i18nc( "@info:status", "Failed to load MBox folder %1", path );
          kError() << errorText << "collection=" << collection;
          q->notifyError( FileStore::Job::InvalidJobContext, errorText ); // TODO should be a different error code
          return false;
        }

        mbox->mCollection = collection;
        mMBoxes.insert( path, mbox );
      } else {
        mbox = findIt.value();
      }

      fillMBoxCollectionDetails( mbox, collection );
    } else {
      const Maildir md( path, folderType == TopLevelFolder );
      fillMaildirCollectionDetails( md, collection );
    }
    collections << collection;
  } else {
    const Maildir md( path, folderType == TopLevelFolder );
    fillMaildirTreeDetails( md, collection, collections,
                            job->type() == FileStore::CollectionFetchJob::Recursive );
  }

  q->notifyCollectionsProcessed( collections );
  return true;
}

static Collection updateMBoxCollectionTree( const Collection &collection, const Collection &oldParent, const Collection &newParent )
{
  if ( collection == oldParent ) {
    return newParent;
  }

  if ( collection == Collection::root() ) {
    return collection;
  }

  Collection updatedCollection = collection;
  updatedCollection.setParentCollection( updateMBoxCollectionTree( collection.parentCollection(), oldParent, newParent ) );

  return updatedCollection;
}

bool MixedMaildirStore::Private::visit( FileStore::CollectionModifyJob *job )
{
  const Collection collection = job->collection();

  // we don't change our top level collection
  if ( job->collection().remoteId() == q->topLevelCollection().remoteId() ) {
    kWarning() << "CollectionModifyJob for TopLevelCollection. Ignoring";
    return true;
  }

  // we also only do renames
  if ( collection.remoteId() == collection.name() ) {
    kWarning() << "CollectionModifyJob with name still identical to remoteId. Ignoring";
    return true;
  }

  QString path;
  QString errorText;
  const FolderType folderType = folderForCollection( collection, path, errorText );
  if ( folderType == InvalidFolder || folderType == TopLevelFolder ) {
    errorText = i18nc( "@info:status", "Cannot rename folder %1",
                        collection.name() );
    kError() << errorText << "FolderType=" << folderType;
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

  const QFileInfo fileInfo( path );
  const QFileInfo subDirInfo = Maildir::subDirPathForFolderPath( path );

  QDir parentDir( path );
  parentDir.cdUp();

  const QFileInfo targetFileInfo( parentDir, collection.name() );
  const QFileInfo targetSubDirInfo = Maildir::subDirPathForFolderPath( targetFileInfo.absoluteFilePath() );

  if ( targetFileInfo.exists() || ( subDirInfo.exists() && targetSubDirInfo.exists() ) ) {
    errorText = i18nc( "@info:status", "Cannot rename folder %1",
                        collection.name() );
    kError() << errorText << "FolderType=" << folderType;
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

  if ( !parentDir.rename( fileInfo.absoluteFilePath(), targetFileInfo.fileName() ) ) {
    errorText = i18nc( "@info:status", "Cannot rename folder %1",
                        collection.name() );
    kError() << errorText << "FolderType=" << folderType;
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

  if ( subDirInfo.exists() ) {
    if ( !parentDir.rename( subDirInfo.absoluteFilePath(), targetSubDirInfo.fileName() ) ) {
      errorText = i18nc( "@info:status", "Cannot rename folder %1",
                          collection.name() );
      kError() << errorText << "FolderType=" << folderType;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );

      // try to recover the previous rename
      parentDir.rename( targetFileInfo.absoluteFilePath(), fileInfo.fileName() );
      return false;
    }
  }

  Collection renamedCollection = collection;
  renamedCollection.setRemoteId( collection.name() );

  // update collections in MBox contexts so they stay usable for purge
  Q_FOREACH( const MBoxPtr &mbox, mMBoxes ) {
    if ( mbox->mCollection.isValid() ) {
      MBoxPtr updatedMBox = mbox;
      updatedMBox->mCollection = updateMBoxCollectionTree( mbox->mCollection, collection, renamedCollection );
    }
  }

  q->notifyCollectionsProcessed( Collection::List() << renamedCollection );
  return true;
}

bool MixedMaildirStore::Private::visit( FileStore::CollectionMoveJob *job )
{
  QString errorText;

  Collection moveCollection = job->collection();
  const Collection targetCollection = job->targetParent();

  QString movePath;
  const FolderType moveFolderType = folderForCollection( moveCollection, movePath, errorText );
  if ( moveFolderType == InvalidFolder || moveFolderType == TopLevelFolder ) {
    errorText = i18nc( "@info:status", "Cannot move folder %1 from folder %2 to folder %3",
                        moveCollection.name(), moveCollection.parentCollection().name(), targetCollection.name() );
    kError() << errorText << "FolderType=" << moveFolderType;
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

//   kDebug( KDE_DEFAULT_DEBUG_AREA ) << "moveCollection" << moveCollection.remoteId()
//                                    << "movePath=" << movePath
//                                    << "moveType=" << moveFolderType;

  QString targetPath;
  const FolderType targetFolderType = folderForCollection( targetCollection, targetPath, errorText );
  if ( targetFolderType == InvalidFolder || targetFolderType == TopLevelFolder ) {
    errorText = i18nc( "@info:status", "Cannot move folder %1 from folder %2 to folder %3",
                        moveCollection.name(), moveCollection.parentCollection().name(), targetCollection.name() );
    kError() << errorText << "FolderType=" << targetFolderType;
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

//   kDebug( KDE_DEFAULT_DEBUG_AREA ) << "targetCollection" << targetCollection.remoteId()
//                                    << "targetPath=" << targetPath
//                                    << "targetType=" << targetFolderType;

  if ( moveFolderType == MBoxFolder ) {
    const QFileInfo moveFileInfo( movePath );
    const QFileInfo moveSubDirInfo( Maildir::subDirPathForFolderPath( movePath ) );
    const QFileInfo targetFileInfo( targetPath );
    const QFileInfo targetSubDirInfo( Maildir::subDirPathForFolderPath( targetPath ) );

    QDir targetDir( Maildir::subDirPathForFolderPath( targetPath ) );
    if ( targetDir.exists( moveFileInfo.fileName() ) ||
         !targetDir.rename( moveFileInfo.absoluteFilePath(), moveFileInfo.fileName() ) ) {
      errorText = i18nc( "@info:status", "Cannot move folder %1 from folder %2 to folder %3",
                          moveCollection.name(), moveCollection.parentCollection().name(), targetCollection.name() );
      kError() << errorText << "MoveFolderType=" << moveFolderType
               << "TargetFolderType=" << targetFolderType;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }

    if ( moveSubDirInfo.exists() ) {
      if ( targetDir.exists( moveSubDirInfo.fileName() ) ||
          !targetDir.rename( moveSubDirInfo.absoluteFilePath(), moveSubDirInfo.fileName() ) ) {
        errorText = i18nc( "@info:status", "Cannot move folder %1 from folder %2 to folder %3",
                            moveCollection.name(), moveCollection.parentCollection().name(), targetCollection.name() );
        kError() << errorText << "MoveFolderType=" << moveFolderType
                << "TargetFolderType=" << targetFolderType;

        // try to revert the other rename
        QDir sourceDir( moveFileInfo.absolutePath() );
        sourceDir.cdUp();
        sourceDir.rename( targetFileInfo.absoluteFilePath(), moveFileInfo.fileName() );
        q->notifyError( FileStore::Job::InvalidJobContext, errorText );
        return false;
      }
    }
  } else {
    Maildir moveMd( movePath, false );

    // for moving purpose we can treat the MBox target's subDirPath like a top level maildir
    Maildir targetMd;
    if ( targetFolderType == MBoxFolder ) {
      targetMd = Maildir( Maildir::subDirPathForFolderPath( targetPath ), true );
    } else {
      targetMd = Maildir( targetPath, false );
    }

    if ( !moveMd.moveTo( targetMd ) ) {
      errorText = i18nc( "@info:status", "Cannot move folder %1 from folder %2 to folder %3",
                          moveCollection.name(), moveCollection.parentCollection().name(), targetCollection.name() );
      kError() << errorText << "MoveFolderType=" << moveFolderType
               << "TargetFolderType=" << targetFolderType;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }
  }

  // update collections in MBox contexts so they stay usable for purge
  Q_FOREACH( const MBoxPtr &mbox, mMBoxes ) {
    if ( mbox->mCollection.isValid() ) {
      MBoxPtr updatedMBox = mbox;
      updatedMBox->mCollection = updateMBoxCollectionTree( mbox->mCollection, moveCollection.parentCollection(), targetCollection );
    }
  }

  moveCollection.setParentCollection( targetCollection );
  q->notifyCollectionsProcessed( Collection::List() << moveCollection );
  return true;
}

bool MixedMaildirStore::Private::visit( FileStore::ItemCreateJob *job )
{
  QString path;
  QString errorText;

  const FolderType folderType = folderForCollection( job->collection(), path, errorText );
  if ( folderType == InvalidFolder || folderType == TopLevelFolder ) {
    errorText = i18nc( "@info:status", "Cannot add emails to folder %1",
                        job->collection().name() );
    kError() << errorText << "FolderType=" << folderType;
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

  Item item = job->item();

  if ( folderType == MBoxFolder ) {
    MBoxPtr mbox;
    MBoxHash::const_iterator findIt = mMBoxes.constFind( path );
    if ( findIt == mMBoxes.constEnd() ) {
      mbox = MBoxPtr( new MBoxContext );
      if ( !mbox->load( path ) ) {
        errorText = i18nc( "@info:status", "Cannot add emails to folder %1",
                            job->collection().name() );
        kError() << errorText << "FolderType=" << folderType;
        q->notifyError( FileStore::Job::InvalidJobContext, errorText );
        return false;
      }

      mbox->mCollection = job->collection();
      mMBoxes.insert( path, mbox );
    } else {
      mbox = findIt.value();
    }

    // make sure to read the index (if available) before modifying the data, which would
    // make the index invalid
    mbox->readIndexData();

    // if there is index data now, we let the job creator know that the on-disk index
    // became invalid
    if ( mbox->hasIndexData() ) {
      const QVariant var = QVariant::fromValue<Collection::List>( Collection::List() << job->collection() );
      job->setProperty( "onDiskIndexInvalidated", var );
    }

    qint64 result = mbox->appendEntry( item.payload<KMime::Message::Ptr>() );
    if ( result < 0 ) {
      errorText = i18nc( "@info:status", "Cannot add emails to folder %1",
                          job->collection().name() );
      kError() << errorText << "FolderType=" << folderType;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }
    mbox->save();
    item.setRemoteId( QString::number( result ) );
  } else {
    MaildirPtr mdPtr;
    MaildirHash::const_iterator findIt = mMaildirs.constFind( path );
    if ( findIt == mMaildirs.constEnd() ) {
      mdPtr = MaildirPtr( new MaildirContext( path, false ) );
      mMaildirs.insert( path, mdPtr );
    } else {
      mdPtr = findIt.value();
    }

    // make sure to read the index (if available) before modifying the data, which would
    // make the index invalid
    mdPtr->readIndexData();

    // if there is index data now, we let the job creator know that the on-disk index
    // became invalid
    if ( mdPtr->hasIndexData() ) {
      const QVariant var = QVariant::fromValue<Collection::List>( Collection::List() << job->collection() );
      job->setProperty( "onDiskIndexInvalidated", var );
    }

    QString result = mdPtr->mailDir().addEntry( item.payload<KMime::Message::Ptr>()->encodedContent() );
    if ( result.isEmpty() ) {
      errorText = i18nc( "@info:status", "Cannot add emails to folder %1",
                          job->collection().name() );
      kError() << errorText << "FolderType=" << folderType;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }

    item.setRemoteId( result );
  }

  item.setParentCollection( job->collection() );
  q->notifyItemsProcessed( Item::List() << item );
  return true;
}

bool MixedMaildirStore::Private::visit( FileStore::ItemDeleteJob *job )
{
  const Item item = job->item();
  const Collection collection = item.parentCollection();
  QString path;
  QString errorText;

  const FolderType folderType = folderForCollection( collection, path, errorText );
  if ( folderType == InvalidFolder || folderType == TopLevelFolder ) {
    errorText = i18nc( "@info:status", "Cannot remove emails from folder %1",
                        collection.name() );
    kError() << errorText << "FolderType=" << folderType;
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

  if ( folderType == MBoxFolder ) {
    MBoxPtr mbox;
    MBoxHash::const_iterator findIt = mMBoxes.constFind( path );
    if ( findIt == mMBoxes.constEnd() ) {
      mbox = MBoxPtr( new MBoxContext );
      if ( !mbox->load( path ) ) {
        errorText = i18nc( "@info:status", "Cannot remove emails from folder %1",
                            collection.name() );
        kError() << errorText << "FolderType=" << folderType;
        q->notifyError( FileStore::Job::InvalidJobContext, errorText );
        return false;
      }

      mMBoxes.insert( path, mbox );
    } else {
      mbox = findIt.value();
    }

    bool ok = false;
    qint64 offset = item.remoteId().toLongLong( &ok );
    if ( !ok || offset < 0 ) {
      errorText = i18nc( "@info:status", "Cannot remove emails from folder %1",
                          collection.name() );
      kError() << errorText << "FolderType=" << folderType;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }

    mbox->mCollection = collection;
    mbox->deleteEntry( offset );
  } else {
    MaildirPtr mdPtr;
    MaildirHash::const_iterator findIt = mMaildirs.constFind( path );
    if ( findIt == mMaildirs.constEnd() ) {
      mdPtr = MaildirPtr( new MaildirContext( path, false ) );

      mMaildirs.insert( path, mdPtr );
    } else {
      mdPtr = findIt.value();
    }

    // make sure to read the index (if available) before modifying the data, which would
    // make the index invalid
    mdPtr->readIndexData();

    // if there is index data now, we let the job creator know that the on-disk index
    // became invalid
    if ( mdPtr->hasIndexData() ) {
      const QVariant var = QVariant::fromValue<Collection::List>( Collection::List() << collection );
      job->setProperty( "onDiskIndexInvalidated", var );
    }

    if ( !mdPtr->mailDir().removeEntry( item.remoteId() ) ) {
      errorText = i18nc( "@info:status", "Cannot remove emails from folder %1",
                          collection.name() );
      kError() << errorText << "FolderType=" << folderType;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }
  }

  q->notifyItemsProcessed( Item::List() << item );
  return true;
}

bool MixedMaildirStore::Private::visit( FileStore::ItemFetchJob *job )
{
  ItemFetchScope scope = job->fetchScope();
  const bool includeBody = scope.fullPayload() ||
                           scope.payloadParts().contains( MessagePart::Body );

  const bool fetchSingleItem = job->collection().remoteId().isEmpty();
  const Collection collection = fetchSingleItem ? job->item().parentCollection() : job->collection();

  QString path;
  QString errorText;
  const FolderType folderType = folderForCollection( collection, path, errorText );

  if ( folderType == InvalidFolder ) {
    kError() << errorText << "collection:" << job->collection();
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

  if ( folderType == MBoxFolder ) {
    MBoxHash::iterator findIt = mMBoxes.find( path );
    if ( findIt == mMBoxes.end() || !fetchSingleItem ) {
      MBoxPtr mbox = findIt != mMBoxes.end() ? findIt.value() : MBoxPtr( new MBoxContext );
      if ( !mbox->load( path ) ) {
        errorText = i18nc( "@info:status", "Failed to load MBox folder %1", path );
        kError() << errorText << "collection=" << collection;
        q->notifyError( FileStore::Job::InvalidJobContext, errorText ); // TODO should be a different error code
        if ( findIt != mMBoxes.end() ) {
          mMBoxes.erase( findIt );
        }
        return false;
      }

      if ( findIt == mMBoxes.end() ) {
        findIt = mMBoxes.insert( path, mbox );
      }
    }

    Item::List items;
    if ( fetchSingleItem ) {
      items << job->item();
    } else {
      listCollection( job, findIt.value(), collection, items );
    }

    Item::List::iterator it    = items.begin();
    Item::List::iterator endIt = items.end();
    for ( ; it != endIt; ++it ) {
      if ( !fillItem( findIt.value(), includeBody, *it ) ) {
        kWarning() << "Failed to read item" << (*it).remoteId() << "in MBox file" << path;
      }
    }

    q->notifyItemsProcessed( items );
  } else {
    MaildirPtr mdPtr;
    MaildirHash::const_iterator mdIt = mMaildirs.constFind( path );
    if ( mdIt == mMaildirs.constEnd() ) {
      mdPtr = MaildirPtr( new MaildirContext( path, folderType == TopLevelFolder ) );
      mMaildirs.insert( path, mdPtr );
    } else {
      mdPtr = mdIt.value();
    }

    if ( !mdPtr->mailDir().isValid( errorText ) ) {
      errorText = i18nc( "@info:status", "Failed to load Maildirs folder %1", path );
      kError() << errorText << "collection=" << collection;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText ); // TODO should be a different error code
      return false;
    }

    Item::List items;
    if ( fetchSingleItem ) {
      items << job->item();
    } else {
      listCollection( job, mdPtr, collection, items );
    }

    Item::List::iterator it    = items.begin();
    Item::List::iterator endIt = items.end();
    for ( ; it != endIt; ++it ) {
      if ( !fillItem( mdPtr->mailDir(), includeBody, *it ) ) {
        kWarning() << "Failed to read item" << (*it).remoteId() << "in Maildir" << path;
      }
    }

    q->notifyItemsProcessed( items );
  }

  return true;
}

bool MixedMaildirStore::Private::visit( FileStore::ItemModifyJob *job )
{
  // if we can ignore payload, we have nothing to do
  if ( job->ignorePayload() ) {
    return true;
  }

  Item item = job->item();
  const Collection collection = item.parentCollection();
  QString path;
  QString errorText;

  const FolderType folderType = folderForCollection( collection, path, errorText );
  if ( folderType == InvalidFolder || folderType == TopLevelFolder ) {
    errorText = i18nc( "@info:status", "Cannot modify emails in folder %1",
                        collection.name() );
    kError() << errorText << "FolderType=" << folderType;
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

  if ( folderType == MBoxFolder ) {
    MBoxPtr mbox;
    MBoxHash::const_iterator findIt = mMBoxes.constFind( path );
    if ( findIt == mMBoxes.constEnd() ) {
      mbox = MBoxPtr( new MBoxContext );
      if ( !mbox->load( path ) ) {
        errorText = i18nc( "@info:status", "Cannot modify emails in folder %1",
                            collection.name() );
        kError() << errorText << "FolderType=" << folderType;
        q->notifyError( FileStore::Job::InvalidJobContext, errorText );
        return false;
      }

      mMBoxes.insert( path, mbox );
    } else {
      mbox = findIt.value();
    }

    bool ok = false;
    qint64 offset = item.remoteId().toLongLong( &ok );
    if ( !ok || offset < 0 ) {
      errorText = i18nc( "@info:status", "Cannot modify emails in folder %1",
                          collection.name() );
      kError() << errorText << "FolderType=" << folderType;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }

    // make sure to read the index (if available) before modifying the data, which would
    // make the index invalid
    mbox->readIndexData();

    // if there is index data now, we let the job creator know that the on-disk index
    // became invalid
    if ( mbox->hasIndexData() ) {
      const QVariant var = QVariant::fromValue<Collection::List>( Collection::List() << collection );
      job->setProperty( "onDiskIndexInvalidated", var );
    }

    qint64 newOffset = mbox->appendEntry( item.payload<KMime::Message::Ptr>() );
    if ( offset < 0 ) {
      errorText = i18nc( "@info:status", "Cannot modify emails in folder %1",
                          collection.name() );
      kError() << errorText << "FolderType=" << folderType;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }

    if ( newOffset > 0 ) {
      mbox->mCollection = collection;
      mbox->deleteEntry( offset );
    }
    mbox->save();
    item.setRemoteId( QString::number( newOffset ) );
  } else {
    MaildirPtr mdPtr;
    MaildirHash::const_iterator findIt = mMaildirs.constFind( path );
    if ( findIt == mMaildirs.constEnd() ) {
      mdPtr = MaildirPtr( new MaildirContext( path, false ) );
      mMaildirs.insert( path, mdPtr );
    } else {
      mdPtr = findIt.value();
    }

    // make sure to read the index (if available) before modifying the data, which would
    // make the index invalid
    mdPtr->readIndexData();

    // if there is index data now, we let the job creator know that the on-disk index
    // became invalid
    if ( mdPtr->hasIndexData() ) {
      const QVariant var = QVariant::fromValue<Collection::List>( Collection::List() << collection );
      job->setProperty( "onDiskIndexInvalidated", var );
    }

    mdPtr->mailDir().writeEntry( item.remoteId(), item.payload<KMime::Message::Ptr>()->encodedContent() );
  }

  q->notifyItemsProcessed( Item::List() << item );
  return true;
}

bool MixedMaildirStore::Private::visit( FileStore::ItemMoveJob *job )
{
  QString errorText;

  QString sourcePath;
  const Collection sourceCollection = job->item().parentCollection();
  const FolderType sourceFolderType = folderForCollection( sourceCollection, sourcePath, errorText );
  if ( sourceFolderType == InvalidFolder || sourceFolderType == TopLevelFolder ) {
    errorText = i18nc( "@info:status", "Cannot move emails from folder %1",
                        sourceCollection.name() );
    kError() << errorText << "FolderType=" << sourceFolderType;
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

//   kDebug( KDE_DEFAULT_DEBUG_AREA ) << "sourceCollection" << sourceCollection.remoteId()
//                                    << "sourcePath=" << sourcePath
//                                    << "sourceType=" << sourceFolderType;

  QString targetPath;
  const Collection targetCollection = job->targetParent();
  const FolderType targetFolderType = folderForCollection( targetCollection, targetPath, errorText );
  if ( targetFolderType == InvalidFolder || targetFolderType == TopLevelFolder ) {
    errorText = i18nc( "@info:status", "Cannot move emails to folder %1",
                        targetCollection.name() );
    kError() << errorText << "FolderType=" << targetFolderType;
    q->notifyError( FileStore::Job::InvalidJobContext, errorText );
    return false;
  }

//   kDebug( KDE_DEFAULT_DEBUG_AREA ) << "targetCollection" << targetCollection.remoteId()
//                                    << "targetPath=" << targetPath
//                                    << "targetType=" << targetFolderType;

  Item item = job->item();

  if ( sourceFolderType == MBoxFolder ) {
/*    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "source is MBox";*/
    bool ok= false;
    quint64 offset = item.remoteId().toULongLong( &ok );
    if ( !ok ) {
      errorText = i18nc( "@info:status", "Cannot move emails from folder %1",
                          sourceCollection.name() );
      kError() << errorText << "FolderType=" << sourceFolderType;
      q->notifyError( FileStore::Job::InvalidJobContext, errorText );
      return false;
    }

    MBoxPtr mbox;
    MBoxHash::const_iterator findIt = mMBoxes.constFind( sourcePath );
    if ( findIt == mMBoxes.constEnd() ) {
      mbox = MBoxPtr( new MBoxContext );
      if ( !mbox->load( sourcePath ) ) {
        errorText = i18nc( "@info:status", "Cannot move emails to folder %1",
                            sourceCollection.name() );
        kError() << errorText << "FolderType=" << sourceFolderType;
        q->notifyError( FileStore::Job::InvalidJobContext, errorText );
        return false;
      }

      mbox->mCollection = sourceCollection;
      mMBoxes.insert( sourcePath, mbox );
    } else {
      mbox = findIt.value();
    }

    if ( !item.payload<KMime::Message::Ptr>() ) {
      if ( !fillItem( mbox, true, item ) ) {
        errorText = i18nc( "@info:status", "Cannot move email from folder %1",
                            sourceCollection.name() );
        kError() << errorText << "FolderType=" << sourceFolderType;
        q->notifyError( FileStore::Job::InvalidJobContext, errorText );
        return false;
      }
    }

    if ( targetFolderType == MBoxFolder ) {
/*      kDebug( KDE_DEFAULT_DEBUG_AREA ) << "target is MBox";*/
      MBoxPtr targetMBox;
      MBoxHash::const_iterator findIt = mMBoxes.constFind( targetPath );
      if ( findIt == mMBoxes.constEnd() ) {
        targetMBox = MBoxPtr( new MBoxContext );
        if ( !targetMBox->load( targetPath ) ) {
          errorText = i18nc( "@info:status", "Cannot move emails to folder %1",
                              targetCollection.name() );
          kError() << errorText << "FolderType=" << targetFolderType;
          q->notifyError( FileStore::Job::InvalidJobContext, errorText );
          return false;
        }

        targetMBox->mCollection = targetCollection;
        mMBoxes.insert( targetPath, targetMBox );
      } else {
        targetMBox = findIt.value();
      }

      // make sure to read the index (if available) before modifying the data, which would
      // make the index invalid
      targetMBox->readIndexData();

      // if there is index data now, we let the job creator know that the on-disk index
      // became invalid
      if ( targetMBox->hasIndexData() ) {
        const QVariant var = QVariant::fromValue<Collection::List>( Collection::List() << targetCollection );
        job->setProperty( "onDiskIndexInvalidated", var );
      }

      qint64 remoteId = targetMBox->appendEntry( item.payload<KMime::Message::Ptr>() );
      if ( remoteId < 0 ) {
        errorText = i18nc( "@info:status", "Cannot move emails to folder %1",
                            targetCollection.name() );
        kError() << errorText << "FolderType=" << targetFolderType;
        q->notifyError( FileStore::Job::InvalidJobContext, errorText );
        return false;
      }

      if ( !targetMBox->save() ) {
        errorText = i18nc( "@info:status", "Cannot move emails to folder %1",
                            targetCollection.name() );
        kError() << errorText << "FolderType=" << targetFolderType;
        q->notifyError( FileStore::Job::InvalidJobContext, errorText );
        return false;
      }

      item.setRemoteId( QString::number( remoteId ) );
    } else {
/*      kDebug( KDE_DEFAULT_DEBUG_AREA ) << "target is Maildir";*/
      MaildirPtr targetMdPtr;
      MaildirHash::const_iterator findIt = mMaildirs.constFind( targetPath );
      if ( findIt == mMaildirs.constEnd() ) {
        targetMdPtr = MaildirPtr( new MaildirContext( targetPath, false ) );
        mMaildirs.insert( targetPath, targetMdPtr );
      } else {
        targetMdPtr = findIt.value();
      }

      // make sure to read the index (if available) before modifying the data, which would
      // make the index invalid
      targetMdPtr->readIndexData();

      // if there is index data now, we let the job creator know that the on-disk index
      // became invalid
      if ( targetMdPtr->hasIndexData() ) {
        const QVariant var = QVariant::fromValue<Collection::List>( Collection::List() << targetCollection );
        job->setProperty( "onDiskIndexInvalidated", var );
      }

      const QString remoteId = targetMdPtr->mailDir().addEntry( mbox->readRawEntry( offset ) );
      if ( remoteId.isEmpty() ) {
        errorText = i18nc( "@info:status", "Cannot move email from folder %1 to folder %2",
                            sourceCollection.name(), targetCollection.name() );
        kError() << errorText << "SourceFolderType=" << sourceFolderType << "TargetFolderType=" << targetFolderType;
        q->notifyError( FileStore::Job::InvalidJobContext, errorText );
        return false;
      }

      item.setRemoteId( remoteId );
    }

    mbox->mCollection = sourceCollection;
    mbox->deleteEntry( offset );
  } else {
/*    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "source is Maildir";*/
    MaildirPtr sourceMdPtr;
    MaildirHash::const_iterator mdIt = mMaildirs.constFind( sourcePath );
    if ( mdIt != mMaildirs.end() ) {
      sourceMdPtr = MaildirPtr( new MaildirContext( sourcePath, false ) );
      mMaildirs.insert( sourcePath, sourceMdPtr );
    } else {
      sourceMdPtr = mdIt.value();
    }

    Collection::List collections;

    // make sure to read the index (if available) before modifying the data, which would
    // make the index invalid
    sourceMdPtr->readIndexData();

    // if there is index data now, we let the job creator know that the on-disk index
    // became invalid
    if ( sourceMdPtr->hasIndexData() ) {
      collections << sourceCollection;
    }

    if ( targetFolderType == MBoxFolder ) {
/*      kDebug( KDE_DEFAULT_DEBUG_AREA ) << "target is MBox";*/
      if ( !item.payload<KMime::Message::Ptr>() ) {
        if ( !fillItem( sourceMdPtr->mailDir(), true, item ) ) {
          errorText = i18nc( "@info:status", "Cannot move email from folder %1",
                              sourceCollection.name() );
          kError() << errorText << "FolderType=" << sourceFolderType;
          q->notifyError( FileStore::Job::InvalidJobContext, errorText );
          return false;
        }
      }

      MBoxPtr mbox;
      MBoxHash::const_iterator findIt = mMBoxes.constFind( targetPath );
      if ( findIt == mMBoxes.constEnd() ) {
        mbox = MBoxPtr( new MBoxContext );
        if ( !mbox->load( targetPath ) ) {
          errorText = i18nc( "@info:status", "Cannot move emails to folder %1",
                              targetCollection.name() );
          kError() << errorText << "FolderType=" << targetFolderType;
          q->notifyError( FileStore::Job::InvalidJobContext, errorText );
          return false;
        }

        mbox->mCollection = targetCollection;
        mMBoxes.insert( targetPath, mbox );
      } else {
        mbox = findIt.value();
      }

      // make sure to read the index (if available) before modifying the data, which would
      // make the index invalid
      mbox->readIndexData();

      // if there is index data now, we let the job creator know that the on-disk index
      // became invalid
      if ( mbox->hasIndexData() ) {
        collections << targetCollection;
      }

      sourceMdPtr->mailDir().removeEntry( item.remoteId() );

      qint64 remoteId = mbox->appendEntry( item.payload<KMime::Message::Ptr>() );
      if ( remoteId < 0 ) {
        errorText = i18nc( "@info:status", "Cannot move emails to folder %1",
                            targetCollection.name() );
        kError() << errorText << "FolderType=" << targetFolderType;
        q->notifyError( FileStore::Job::InvalidJobContext, errorText );
        return false;
      }
      mbox->save();
      item.setRemoteId( QString::number( remoteId ) );
    } else {
/*      kDebug( KDE_DEFAULT_DEBUG_AREA ) << "target is Maildir";*/
      MaildirPtr targetMdPtr;
      MaildirHash::const_iterator findIt = mMaildirs.constFind( targetPath );
      if ( findIt == mMaildirs.constEnd() ) {
        targetMdPtr = MaildirPtr( new MaildirContext( targetPath, false ) );
        mMaildirs.insert( targetPath, targetMdPtr );
      } else {
        targetMdPtr = findIt.value();
      }

      // make sure to read the index (if available) before modifying the data, which would
      // make the index invalid
      targetMdPtr->readIndexData();

      // if there is index data now, we let the job creator know that the on-disk index
      // became invalid
      if ( targetMdPtr->hasIndexData() ) {
        collections << targetCollection;
      }

      const QString remoteId = sourceMdPtr->mailDir().moveEntryTo( item.remoteId(), targetMdPtr->mailDir() );
      if ( remoteId.isEmpty() ) {
        errorText = i18nc( "@info:status", "Cannot move email from folder %1 to folder %2",
                            sourceCollection.name(), targetCollection.name() );
        kError() << errorText << "SourceFolderType=" << sourceFolderType << "TargetFolderType=" << targetFolderType;
        q->notifyError( FileStore::Job::InvalidJobContext, errorText );
        return false;
      }

      item.setRemoteId( remoteId );
    }

    if ( !collections.isEmpty() ) {
      const QVariant var = QVariant::fromValue<Collection::List>( collections );
      job->setProperty( "onDiskIndexInvalidated", var );
    }
  }

  item.setParentCollection( targetCollection );
  q->notifyItemsProcessed( Item::List() << item );
  return true;
}

bool MixedMaildirStore::Private::visit( FileStore::StoreCompactJob *job )
{
  Q_UNUSED( job );

  Collection::List collections;

  MBoxHash::const_iterator it    = mMBoxes.constBegin();
  MBoxHash::const_iterator endIt = mMBoxes.constEnd();
  for ( ; it != endIt; ++it ) {
    MBoxPtr mbox = it.value();

    // make sure to read the index (if available) before modifying the data, which would
    // make the index invalid
    mbox->readIndexData();

    QList<MsgInfo> movedEntries;
    const int result = mbox->purge( movedEntries );
    if ( result > 0 ) {
      Item::List items;
      Q_FOREACH( const MsgInfo &offsetPair, movedEntries ) {
        const QString oldRemoteId( QString::number( offsetPair.first ) );
        const QString newRemoteId( QString::number( offsetPair.second ) );

        Item item;
        item.setRemoteId( oldRemoteId );
        item.setParentCollection( mbox->mCollection );
        item.attribute<FileStore::EntityCompactChangeAttribute>( Entity::AddIfMissing )->setRemoteId( newRemoteId );

        items << item;
      }

      // if there is index data, we let the job creator know that the on-disk index
      // became invalid
      if ( mbox->hasIndexData() ) {
        collections << mbox->mCollection;
      }

      if ( !items.isEmpty() ) {
        q->notifyItemsProcessed( items );
      }
    }
  }

  if ( !collections.isEmpty() ) {
    const QVariant var = QVariant::fromValue<Collection::List>( collections );
    job->setProperty( "onDiskIndexInvalidated", var );
  }

  return true;
}

MixedMaildirStore::MixedMaildirStore() : FileStore::AbstractLocalStore(), d( new Private( this ) )
{
}

MixedMaildirStore::~MixedMaildirStore()
{
  delete d;
}

void MixedMaildirStore::setTopLevelCollection( const Collection &collection )
{
  QStringList contentMimeTypes;
  contentMimeTypes << Collection::mimeType();

  Collection::Rights rights;
  // TODO check if read-only?
  rights = Collection::CanCreateCollection | Collection::CanChangeCollection | Collection::CanDeleteCollection;

  CachePolicy cachePolicy;
  cachePolicy.setInheritFromParent( false );
  cachePolicy.setLocalParts( QStringList() << MessagePart::Header );
  cachePolicy.setSyncOnDemand( true );

  Collection modifiedCollection = collection;
  modifiedCollection.setContentMimeTypes( contentMimeTypes );
  modifiedCollection.setRights( rights );
  modifiedCollection.setParentCollection( Collection::root() );
  modifiedCollection.setCachePolicy( cachePolicy );

  // clear caches
  d->mMBoxes.clear();

  FileStore::AbstractLocalStore::setTopLevelCollection( modifiedCollection );
}

void MixedMaildirStore::processJob( FileStore::Job *job )
{
  if ( !job->accept( d ) ) {
    // check that an error has been set
    if ( job->error() == 0 || job->errorString().isEmpty() ) {
      kError() << "visitor did not set either error code or error string when returning false";
      Q_ASSERT( job->error() == 0 || job->errorString().isEmpty() );
    }
  } else {
    // check that no error has been set
    if ( job->error() != 0 || !job->errorString().isEmpty() ) {
      kError() << "visitor did set either error code or error string when returning true";
      Q_ASSERT( job->error() != 0 || !job->errorString().isEmpty() );
    }
  }
}

void MixedMaildirStore::checkCollectionMove( FileStore::CollectionMoveJob *job, int &errorCode, QString &errorText ) const
{
  // check if the target is not the collection itself or one if its children
  Collection targetCollection = job->targetParent();
  while ( targetCollection.isValid() ) {
    if ( targetCollection == job->collection() ) {
      errorCode = FileStore::Job::InvalidJobContext;
      errorText = i18nc( "@info:status", "Cannot move folder %1 into one of its own subfolder tree", job->collection().name() );
      return;
    }

    targetCollection = targetCollection.parentCollection();
  }
}

void MixedMaildirStore::checkItemCreate( FileStore::ItemCreateJob *job, int &errorCode, QString &errorText ) const
{
  if ( !job->item().hasPayload<KMime::Message::Ptr>() ) {
    errorCode = FileStore::Job::InvalidJobContext;
    errorText = i18nc( "@info:status", "Cannot add email to folder %1 because there is no email content", job->collection().name() );
  }
}

void MixedMaildirStore::checkItemModify( FileStore::ItemModifyJob *job, int &errorCode, QString &errorText ) const
{
  if ( !job->ignorePayload() && !job->item().hasPayload<KMime::Message::Ptr>() ) {
    errorCode = FileStore::Job::InvalidJobContext;
    errorText = i18nc( "@info:status", "Cannot modify email in folder %1 because there is no email content", job->item().parentCollection().name() );
  }
}

#include "mixedmaildirstore.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
