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

#include "filestore/collectioncreatejob.h"
#include "filestore/collectiondeletejob.h"
#include "filestore/collectionfetchjob.h"
#include "filestore/itemfetchjob.h"

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
using namespace Akonadi::FileStore;
using KPIM::Maildir;

static void fillIndexCollectionDetails( const QString &path, Collection &collection )
{
  const QFileInfo dirInfo( path );
  const QFileInfo fileInfo( dirInfo.dir(), QString::fromUtf8( ".%1.index" ).arg( dirInfo.fileName() ) );

  if ( !fileInfo.exists() || !fileInfo.isReadable() ) {
    kDebug() << "No index file" << fileInfo.absoluteFilePath() << "or not readable";
    return;
  }

  // TODO
}

static void fillMBoxCollectionDetails( const MBox &mbox, Collection &collection )
{
  collection.setContentMimeTypes( QStringList() << Collection::mimeType()
                                                << KMime::Message::mimeType() );

  const QFileInfo fileInfo( mbox.fileName() );
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

  fillIndexCollectionDetails( fileInfo.absoluteFilePath(), collection );
}

static void fillMaildirCollectionDetails( const Maildir &md, Collection &collection )
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

  fillIndexCollectionDetails( fileInfo.absoluteFilePath(), collection );
}

static void fillMaildirTreeDetails( const Maildir &md, const Collection &collection, Collection::List &collections, bool recurse )
{
  if ( md.path().isEmpty() ) {
    return;
  }

  const QStringList maildirSubFolders = md.subFolderList();
  Q_FOREACH( const QString &subFolder, maildirSubFolders ) {
    const Maildir subMd = md.subFolder( subFolder );

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

  const QDir dir( Maildir::subDirPathForFolderPath( md.path() ) );
  const QFileInfoList fileInfos = dir.entryInfoList( QDir::Files );
  Q_FOREACH( const QFileInfo &fileInfo, fileInfos ) {
    if ( fileInfo.isHidden() || !fileInfo.isReadable() ) {
      continue;
    }
    MBox mbox;
    if ( mbox.load( fileInfo.absoluteFilePath() ) ) {
      const QString subFolder = fileInfo.fileName();
      Collection col;
      col.setRemoteId( subFolder );
      col.setName( subFolder );
      col.setParentCollection( collection );

      fillMBoxCollectionDetails( mbox, col );
      collections << col;

      if ( recurse ) {
        const QString subDirPath = Maildir::subDirPathForFolderPath( fileInfo.absoluteFilePath() );
        const Maildir subMd( subDirPath, true );
        fillMaildirTreeDetails( subMd, col, collections, true );
      }
    }
  }
}

static void listCollection( const MBox &mbox, const Collection &collection, Item::List &items )
{
  const QList<MsgInfo> entryList = mbox.entryList();
  Q_FOREACH( const MsgInfo &entry, entryList ) {
    Item item;
    item.setMimeType( KMime::Message::mimeType() );
    item.setRemoteId( QString::number( entry.first ) );
    item.setParentCollection( collection );

    items << item;
  }
}

static void listCollection( const Maildir &md, const Collection &collection, Item::List &items )
{
  const QStringList entryList = md.entryList();
  Q_FOREACH( const QString &entry, entryList ) {
    Item item;
    item.setMimeType( KMime::Message::mimeType() );
    item.setRemoteId( entry );
    item.setParentCollection( collection );

    items << item;
  }
}

static bool fillItem( MBox &mbox, bool includeBody, Item &item )
{
//  kDebug() << "Filling item" << item.remoteId() << "from MBox: includeBody=" << includeBody;
  KMime::Message *message = 0;
  if ( includeBody ) {
    message = mbox.readEntry( item.remoteId().toLongLong() );
    if ( message == 0 ) {
      return false;
    }
  } else {
    message = new KMime::Message();
    message->setHead( KMime::CRLFtoLF( mbox.readEntryHeaders( item.remoteId().toLongLong() ) ) );
  }

  KMime::Message::Ptr messagePtr( message );
  item.setPayload<KMime::Message::Ptr>( messagePtr );
  return true;
}

static bool fillItem( const Maildir &md, bool includeBody, Item &item )
{
//  kDebug() << "Filling item" << item.remoteId() << "from Maildir: includeBody=" << includeBody;
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

class MixedMaildirStore::Private : public Job::Visitor
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

    FolderType folderForCollection( const Collection &col, QString &path, QString &errorText ) const
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

          const QString subDirPath = Maildir::subDirPathForFolderPath( path );
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

  public: // visitor interface implementation
    bool visit( Job *job );
    bool visit( CollectionCreateJob *job );
    bool visit( CollectionDeleteJob *job );
    bool visit( CollectionFetchJob *job );
    bool visit( CollectionModifyJob *job );
    bool visit( CollectionMoveJob *job );
    bool visit( ItemCreateJob *job );
    bool visit( ItemDeleteJob *job );
    bool visit( ItemFetchJob *job );
    bool visit( ItemModifyJob *job );
    bool visit( ItemMoveJob *job );
    bool visit( StoreCompactJob *job );
};

bool MixedMaildirStore::Private::visit( Job *job )
{
  const QString message = i18nc( "@info:status", "Unhandled operation %1", job->metaObject()->className() );
  kError() << message;
  q->notifyError( Job::InvalidJobContext, message );
  return false;
}

bool MixedMaildirStore::Private::visit( CollectionCreateJob *job )
{
  QString path;
  QString errorText;

  const FolderType folderType = folderForCollection( job->targetParent(), path, errorText );
  if ( folderType == InvalidFolder ) {
    errorText = i18nc( "@info:status", "Cannot create folder %1 inside folder %2",
                        job->collection().name(), job->targetParent().name() );
    kError() << errorText << "FolderType=" << folderType;
    q->notifyError( Job::InvalidJobContext, errorText );
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
      q->notifyError( Job::InvalidJobContext, errorText );
      return false;
    }

    if ( !dir.mkpath( job->collection().name() ) ) {
      errorText = i18nc( "@info:status", "Cannot create folder %1 inside folder %2",
                          job->collection().name(), job->targetParent().name() );
      kError() << errorText << "FolderType=" << folderType << ", mkpath failed";
      q->notifyError( Job::InvalidJobContext, errorText );
      return false;
    }

    Maildir md( dirInfo.absoluteFilePath(), false );
    if ( !md.create() ) {
      errorText = i18nc( "@info:status", "Cannot create folder %1 inside folder %2",
                          job->collection().name(), job->targetParent().name() );
      kError() << errorText << "FolderType=" << folderType << ", maildir create failed";
      q->notifyError( Job::InvalidJobContext, errorText );
      return false;
    }
  } else {
    Maildir parentMd( path, folderType == TopLevelFolder );
    if ( parentMd.addSubFolder( job->collection().name() ).isEmpty() ) {
      errorText = i18nc( "@info:status", "Cannot create folder %1 inside folder %2",
                          job->collection().name(), job->targetParent().name() );
      kError() << errorText << "FolderType=" << folderType;
      q->notifyError( Job::InvalidJobContext, errorText );
      return false;
    }
  }

  Collection collection = job->collection();
  collection.setRemoteId( collection.name() );
  collection.setParentCollection( job->targetParent() );
  q->notifyCollectionsProcessed( Collection::List() << collection );
  return true;
}

bool MixedMaildirStore::Private::visit( CollectionDeleteJob *job )
{
  QString path;
  QString errorText;

  const FolderType folderType = folderForCollection( job->collection(), path, errorText );
  if ( folderType == InvalidFolder ) {
    errorText = i18nc( "@info:status", "Cannot remove folder %1 from folder %2",
                        job->collection().name(), job->collection().parentCollection().name() );
    kError() << errorText << "FolderType=" << folderType;
    q->notifyError( Job::InvalidJobContext, errorText );
    return false;
  }

  if ( folderType == MBoxFolder ) {
    if ( !QFile::remove( path ) ) {
      errorText = i18nc( "@info:status", "Cannot remove folder %1 from folder %2",
                          job->collection().name(), job->collection().parentCollection().name() );
      kError() << errorText << "FolderType=" << folderType;
      q->notifyError( Job::InvalidJobContext, errorText );
      return false;
    }
  } else {
    if ( !KPIMUtils::removeDirAndContentsRecursively( path ) ) {
      errorText = i18nc( "@info:status", "Cannot remove folder %1 from folder %2",
                          job->collection().name(), job->collection().parentCollection().name() );
      kError() << errorText << "FolderType=" << folderType;
      q->notifyError( Job::InvalidJobContext, errorText );
      return false;
    }
  }

  const QString subDirPath = Maildir::subDirPathForFolderPath( path );
  KPIMUtils::removeDirAndContentsRecursively( subDirPath );

  q->notifyCollectionsProcessed( Collection::List() << job->collection() );
  return true;
}

bool MixedMaildirStore::Private::visit( CollectionFetchJob *job )
{
  QString path;
  QString errorText;
  const FolderType folderType = folderForCollection( job->collection(), path, errorText );

  if ( folderType == InvalidFolder ) {
    errorText = i18nc( "@info:status", "Folder %1 does not seem to be a valid email folder",
                       path );
    kError() << errorText << "collection:" << job->collection();
    q->notifyError( Job::InvalidJobContext, errorText );
    return false;
  }

  Collection::List collections;
  Collection collection = job->collection();
  if ( job->type() == CollectionFetchJob::Base ) {
    collection.setName( collection.remoteId() );
    if ( folderType == MBoxFolder ) {
      MBox mbox;
      if ( !mbox.load( path ) ) {
        errorText = i18nc( "@info:status", "Failed to load MBox folder %1", path );
        kError() << errorText << "collection=" << collection;
        q->notifyError( Job::InvalidJobContext, errorText ); // TODO should be a different error code
        return false;
      }

      // TODO maybe we should cache the loaded MBox object for better performance
      fillMBoxCollectionDetails( mbox, collection );
    } else {
      const Maildir md( path, folderType == TopLevelFolder );
      fillMaildirCollectionDetails( md, collection );
    }
    collections << collection;
  } else {
    const Maildir md( path, folderType == TopLevelFolder );
    fillMaildirTreeDetails( md, collection, collections,
                            job->type() == CollectionFetchJob::Recursive );
  }

  q->notifyCollectionsProcessed( collections );
  return true;
}

bool MixedMaildirStore::Private::visit( CollectionModifyJob *job )
{
}

bool MixedMaildirStore::Private::visit( CollectionMoveJob *job )
{
}

bool MixedMaildirStore::Private::visit( ItemCreateJob *job )
{
}

bool MixedMaildirStore::Private::visit( ItemDeleteJob *job )
{
}

bool MixedMaildirStore::Private::visit( ItemFetchJob *job )
{
  ItemFetchScope scope = job->fetchScope();
  const bool includeBody = scope.fullPayload() ||
                           scope.payloadParts().contains( MessagePart::Body );

  Collection collection = job->collection();
  if ( collection.remoteId().isEmpty() ) {
    collection = job->item().parentCollection();
  }

  QString path;
  QString errorText;
  const FolderType folderType = folderForCollection( collection, path, errorText );

  if ( folderType == InvalidFolder ) {
    errorText = i18nc( "@info:status", "Folder %1 does not seem to be a valid email folder",
                       path );
    kError() << errorText << "collection:" << job->collection();
    q->notifyError( Job::InvalidJobContext, errorText );
    return false;
  }

  if ( folderType == MBoxFolder ) {
    MBox mbox;
    if ( !mbox.load( path ) ) {
      errorText = i18nc( "@info:status", "Failed to load MBox folder %1", path );
      kError() << errorText << "collection=" << collection;
      q->notifyError( Job::InvalidJobContext, errorText ); // TODO should be a different error code
      return false;
    }

    Item::List items;
    if ( job->collection().remoteId().isEmpty() ) {
      items << job->item();
    } else {
      listCollection( mbox, collection, items );
    }

    Item::List::iterator it    = items.begin();
    Item::List::iterator endIt = items.end();
    for ( ; it != endIt; ++it ) {
      if ( !fillItem( mbox, includeBody, *it ) ) {
        kWarning() << "Failed to read item" << (*it).remoteId() << "in MBox file" << path;
      }
    }

    q->notifyItemsProcessed( items );
  } else {
    Maildir md( path, folderType == TopLevelFolder );
    if ( !md.isValid( errorText ) ) {
      errorText = i18nc( "@info:status", "Failed to load Maildirs folder %1", path );
      kError() << errorText << "collection=" << collection;
      q->notifyError( Job::InvalidJobContext, errorText ); // TODO should be a different error code
      return false;
    }

    Item::List items;
    if ( job->collection().remoteId().isEmpty() ) {
      items << job->item();
    } else {
      listCollection( md, collection, items );
    }

    Item::List::iterator it    = items.begin();
    Item::List::iterator endIt = items.end();
    for ( ; it != endIt; ++it ) {
      if ( !fillItem( md, includeBody, *it ) ) {
        kWarning() << "Failed to read item" << (*it).remoteId() << "in Maildir" << path;
      }
    }

    q->notifyItemsProcessed( items );
  }

  return true;
}

bool MixedMaildirStore::Private::visit( ItemModifyJob *job )
{
}

bool MixedMaildirStore::Private::visit( ItemMoveJob *job )
{
}

bool MixedMaildirStore::Private::visit( StoreCompactJob *job )
{
}

MixedMaildirStore::MixedMaildirStore() : AbstractLocalStore(), d( new Private( this ) )
{
}

MixedMaildirStore::~MixedMaildirStore()
{
  delete d;
}

void MixedMaildirStore::setTopLevelCollection( const Akonadi::Collection &collection )
{
  QStringList contentMimeTypes;
  contentMimeTypes << Collection::mimeType();

  Collection::Rights rights;
  // TODO check if read-only?
  rights = Collection::CanCreateCollection | Collection::CanChangeCollection | Collection::CanDeleteCollection;

  CachePolicy cachePolicy;
  cachePolicy.setInheritFromParent( false );
  cachePolicy.setLocalParts( QStringList() << MessagePart::Header );

  Collection modifiedCollection = collection;
  modifiedCollection.setContentMimeTypes( contentMimeTypes );
  modifiedCollection.setRights( rights );
  modifiedCollection.setParentCollection( Collection::root() );
  modifiedCollection.setCachePolicy( cachePolicy );

  AbstractLocalStore::setTopLevelCollection( modifiedCollection );
}

void MixedMaildirStore::processJob( Job *job )
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

#include "mixedmaildirstore.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
