/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

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

#include "maildirresource.h"
#include "settings.h"
#include "maildirsettingsadaptor.h"
#include "configdialog.h"
#include "retrieveitemsjob.h"

#include <QtCore/QDir>
#include <QtDBus/QDBusConnection>

#include <akonadi/kmime/messageparts.h>
#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/cachepolicy.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/dbusconnectionpool.h>

#include <kmime/kmime_message.h>

#include <kdebug.h>
#include <kdirwatch.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kwindowsystem.h>

#include "libmaildir/maildir.h"

using namespace Akonadi;
using KPIM::Maildir;
using namespace Akonadi_Maildir_Resource;

Maildir MaildirResource::maildirForCollection( const Collection &col ) const
{
  if ( col.remoteId().isEmpty() ) {
    kWarning() << "Got incomplete ancestor chain:" << col;
    return Maildir();
  }

  if ( col.parentCollection() == Collection::root() ) {
    kWarning( col.remoteId() != mSettings->path() ) << "RID mismatch, is " << col.remoteId() << " expected " << mSettings->path();
    return Maildir( col.remoteId(), mSettings->topLevelIsContainer() );
  }
  Maildir parentMd = maildirForCollection( col.parentCollection() );
  return parentMd.subFolder( col.remoteId() );
}

Collection MaildirResource::collectionForMaildir(const Maildir& md) const
{
  if ( !md.isValid() )
    return Collection();

  Collection col;
  if ( md.path() == mSettings->path() ) {
    col.setRemoteId( md.path() );
    col.setParentCollection( Collection::root() );
  } else {
    const Collection parent = collectionForMaildir( md.parent() );
    col.setRemoteId( md.name() );
    col.setParentCollection( parent );
  }

  return col;
}

MaildirResource::MaildirResource( const QString &id )
    :ResourceBase( id ),
    mSettings( new MaildirSettings( componentData().config() ) ),
    m_fsWatcher( new KDirWatch( this ) )
{
  new MaildirSettingsAdaptor( mSettings );
  DBusConnectionPool::threadConnection().registerObject( QLatin1String( "/Settings" ),
                              mSettings, QDBusConnection::ExportAdaptors );
  connect( this, SIGNAL(reloadConfiguration()), SLOT(configurationChanged()) );

  // We need to enable this here, otherwise we neither get the remote ID of the
  // parent collection when a collection changes, nor the full item when an item
  // is added.
  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().fetchFullPayload( true );
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::All );
  changeRecorder()->itemFetchScope().setFetchModificationTime( false );
  changeRecorder()->collectionFetchScope().setAncestorRetrieval( CollectionFetchScope::All );
  changeRecorder()->fetchChangedOnly( true );

  setHierarchicalRemoteIdentifiersEnabled( true );

  ItemFetchScope scope( changeRecorder()->itemFetchScope() );
  scope.fetchFullPayload( false );
  scope.fetchPayloadPart( MessagePart::Header );
  scope.setAncestorRetrieval( ItemFetchScope::None );
  setItemSynchronizationFetchScope( scope );

  connect( m_fsWatcher, SIGNAL(dirty(QString)), SLOT(slotDirChanged(QString)) );

  synchronizeCollectionTree();
}

MaildirResource::~ MaildirResource()
{
  delete mSettings;
}

bool MaildirResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );

  const Maildir md = maildirForCollection( item.parentCollection() );
  if ( !md.isValid() ) {
    cancelTask( i18n( "Unable to fetch item: The maildir folder \"%1\" is not valid.",
                      md.path() ) );
    return false;
  }

  const QByteArray data = md.readEntry( item.remoteId() );
  KMime::Message *mail = new KMime::Message();
  mail->setContent( KMime::CRLFtoLF( data ) );
  mail->parse();

  Item i( item );
  i.setPayload( KMime::Message::Ptr( mail ) );
  itemRetrieved( i );
  return true;
}

QString MaildirResource::itemMimeType()
{
  return KMime::Message::mimeType();
}

void MaildirResource::configurationChanged()
{
  mSettings->writeConfig();
  ensureSaneConfiguration();
  ensureDirExists();
}


void MaildirResource::aboutToQuit()
{
  // The settings may not have been saved if e.g. they have been modified via
  // DBus instead of the config dialog.
  mSettings->writeConfig();
}

void MaildirResource::configure( WId windowId )
{
  ConfigDialog dlg( mSettings );
  if ( windowId )
    KWindowSystem::setMainWindow( &dlg, windowId );
  if ( dlg.exec() ) {
      // if we have no name, or the default one,
      // better use the name of the top level collection
      // that looks nicer
      if ( name().isEmpty() || name() == identifier() ) {
        Maildir md( mSettings->path() );
        setName( md.name() );
      }
    emit configurationDialogAccepted();
  } else {
    emit configurationDialogRejected();
  }

  ensureDirExists();
  synchronizeCollectionTree();
}

void MaildirResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& collection )
{
    if ( !ensureSaneConfiguration() ) {
      cancelTask( i18n("Unusable configuration.") );
      return;
    }
    Maildir dir = maildirForCollection( collection );
    QString errMsg;
    if ( mSettings->readOnly() || !dir.isValid( errMsg ) ) {
      cancelTask( errMsg );
      return;
    }

    // we can only deal with mail
    if ( !item.hasPayload<KMime::Message::Ptr>() ) {
      cancelTask( i18n("Error: Unsupported type.") );
      return;
    }
    const KMime::Message::Ptr mail = item.payload<KMime::Message::Ptr>();

    QString path = dir.path();
    m_fsWatcher->removeDir( path + QLatin1Literal("/new") );
    m_fsWatcher->removeDir( path + QLatin1Literal("/cur") );

    const QString rid = dir.addEntry( mail->encodedContent() );

    m_fsWatcher->addDir( path + QLatin1Literal("/new") );
    m_fsWatcher->addDir( path + QLatin1Literal("/cur") );

    Item i( item );
    i.setRemoteId( rid );
    changeCommitted( i );
}

void MaildirResource::itemChanged( const Akonadi::Item& item, const QSet<QByteArray>& parts )
{
    if ( !ensureSaneConfiguration() ) {
      cancelTask( i18n("Unusable configuration.") );
      return;
    }
    

    bool bodyChanged = false;
    bool flagsChanged = false;
    bool headChanged = false;
    Q_FOREACH( const QByteArray &part, parts )  {
      if( part.startsWith("PLD:RFC822") ) {
        bodyChanged = true;
      }
      if( part.startsWith("PLD:HEAD") ) {
        headChanged = true;
      }
      if ( part.contains( "FLAGS" ) ) {
        flagsChanged = true;
      }
    }

    if ( mSettings->readOnly() || ( !bodyChanged && !flagsChanged && !headChanged ) ) {
      changeProcessed();
      return;
    }

    Maildir dir = maildirForCollection( item.parentCollection() );
    QString errMsg;
    if ( !dir.isValid( errMsg ) ) {
        cancelTask( errMsg );
        return;
    }

    Item newItem( item );

    if ( flagsChanged || bodyChanged || headChanged ) { //something has changed that we can deal with
      const QString path = dir.path();
      m_fsWatcher->removeDir( path + QLatin1Literal("/new") );
      m_fsWatcher->removeDir( path + QLatin1Literal("/cur") );

      if ( flagsChanged ) { //flags changed, store in file name and get back the new filename (id)
        const QString newKey = dir.changeEntryFlags( item.remoteId(), item.flags() );
        newItem.setRemoteId( newKey );
      }

      if ( bodyChanged || headChanged ) { //head or body changed
        // we can only deal with mail
        if ( item.hasPayload<KMime::Message::Ptr>() ) {
          const KMime::Message::Ptr mail = item.payload<KMime::Message::Ptr>();
          QByteArray data = mail->encodedContent();
          if ( headChanged && !bodyChanged ) { 
            //only the head has changed, get the current version of the mail
            //replace the head and store the new mail in the file
            QByteArray currentData = dir.readEntry( newItem.remoteId() );
            QByteArray newHead = mail->head();
            mail->setContent( currentData );
            mail->setHead( newHead );
            mail->parse();
            data = mail->encodedContent();
          }
          dir.writeEntry( newItem.remoteId(), data );
        }
      }

      m_fsWatcher->addDir( path + QLatin1Literal("/new") );
      m_fsWatcher->addDir( path + QLatin1Literal("/cur") );

      changeCommitted( newItem );
    } else {
      emit changeProcessed();
    }
}

void MaildirResource::itemMoved( const Item &item, const Collection &source, const Collection &destination )
{
  if ( source == destination ) { // should not happen but would confuse Maildir::moveEntryTo
    changeProcessed();
    return;
  }

  if ( !ensureSaneConfiguration() ) {
    cancelTask( i18n("Unusable configuration.") );
    return;
  }

  Maildir sourceDir = maildirForCollection( source );
  QString errMsg;
  if ( !sourceDir.isValid( errMsg ) ) {
    cancelTask( i18n( "Source folder is invalid: '%1'.", errMsg ) );
    return;
  }

  Maildir destDir = maildirForCollection( destination );
  if ( !destDir.isValid( errMsg ) ) {
    cancelTask( i18n( "Destination folder is invalid: '%1'.", errMsg ) );
    return;
  }

  QString sourceDirPath = sourceDir.path();
  QString destDirPath = destDir.path();
  m_fsWatcher->removeDir( sourceDirPath + QLatin1Literal("/new") );
  m_fsWatcher->removeDir( sourceDirPath + QLatin1Literal("/cur") );
  m_fsWatcher->removeDir( destDirPath + QLatin1Literal("/new") );
  m_fsWatcher->removeDir( destDirPath + QLatin1Literal("/cur") );

  const QString newRid = sourceDir.moveEntryTo( item.remoteId(), destDir );

  m_fsWatcher->addDir( sourceDirPath + QLatin1Literal("/new") );
  m_fsWatcher->addDir( sourceDirPath + QLatin1Literal("/cur") );
  m_fsWatcher->addDir( destDirPath + QLatin1Literal("/new") );
  m_fsWatcher->addDir( destDirPath + QLatin1Literal("/cur") );

  if ( newRid.isEmpty() ) {
    cancelTask( i18n( "Could not move message '%1'.", item.remoteId() ) );
    return;
  }

  Item i( item );
  i.setRemoteId( newRid );
  changeCommitted( i );
}

void MaildirResource::itemRemoved(const Akonadi::Item & item)
{
  if ( !ensureSaneConfiguration() ) {
    cancelTask( i18n("Unusable configuration.") );
    return;
  }

  if ( !mSettings->readOnly() ) {
    Maildir dir = maildirForCollection( item.parentCollection() );
    // !dir.isValid() means that our parent folder has been deleted already,
    // so we don't care at all as that one will be recursive anyway
    QString dirPath = dir.path();
    m_fsWatcher->removeDir( dirPath + QLatin1Literal("/new") );
    m_fsWatcher->removeDir( dirPath + QLatin1Literal("/cur") );
    if ( dir.isValid() && !dir.removeEntry( item.remoteId() ) ) {
      emit error( i18n("Failed to delete message: %1", item.remoteId()) );
    }
    m_fsWatcher->addDir( dirPath + QLatin1Literal("/new") );
    m_fsWatcher->addDir( dirPath + QLatin1Literal("/cur") );
  }
  kDebug() << "Item removed";
  changeProcessed();
}

Collection::List MaildirResource::listRecursive( const Collection &root, const Maildir &dir )
{
  if ( mSettings->monitorFilesystem() ) {
    m_fsWatcher->addDir( dir.path() + QDir::separator() + QLatin1String( "new" ) );
    m_fsWatcher->addDir( dir.path() + QDir::separator() + QLatin1String( "cur" ) );
    m_fsWatcher->addDir( dir.subDirPath() );
    if ( dir.isRoot() ) {
      m_fsWatcher->addDir( dir.path() );
    }
  }

  Collection::List list;
  const QStringList mimeTypes = QStringList() << itemMimeType() << Collection::mimeType();
  foreach ( const QString &sub, dir.subFolderList() ) {
    Collection c;
    c.setName( sub );
    c.setRemoteId( sub );
    c.setParentCollection( root );
    c.setContentMimeTypes( mimeTypes );

    const Maildir md = maildirForCollection( c );
    if ( !md.isValid() )
      continue;

    list << c;
    list += listRecursive( c, md );
  }
  return list;
}

void MaildirResource::retrieveCollections()
{
  Maildir dir( mSettings->path(), mSettings->topLevelIsContainer() );
  QString errMsg;
  if ( !dir.isValid( errMsg ) ) {
    emit error( errMsg );
    collectionsRetrieved( Collection::List() );
    return;
  }

  Collection root;
  root.setParentCollection( Collection::root() );
  root.setRemoteId( mSettings->path() );
  root.setName( name() );
  root.setRights( Collection::CanChangeItem | Collection::CanCreateItem | Collection::CanDeleteItem
                | Collection::CanCreateCollection );

  CachePolicy policy;
  policy.setInheritFromParent( false );
  policy.setSyncOnDemand( true );
  policy.setLocalParts( QStringList() << MessagePart::Envelope );
  policy.setCacheTimeout( 1 );
  policy.setIntervalCheckTime( -1 );
  root.setCachePolicy( policy );

  QStringList mimeTypes;
  mimeTypes << Collection::mimeType();
  if ( !mSettings->topLevelIsContainer() )
    mimeTypes << itemMimeType();
  root.setContentMimeTypes( mimeTypes );

  Collection::List list;
  list << root;
  list += listRecursive( root, dir );
  collectionsRetrieved( list );
}

void MaildirResource::retrieveItems( const Akonadi::Collection & col )
{
  const Maildir md = maildirForCollection( col );
  if ( !md.isValid() ) {
    cancelTask( i18n("Maildir '%1' for collection '%2' is invalid.", md.path(), col.remoteId() ) );
    return;
  }

  RetrieveItemsJob *job = new RetrieveItemsJob( col, md, this );
  job->setMimeType( itemMimeType() );
  connect( job, SIGNAL(result(KJob*)), SLOT(slotItemsRetrievalResult(KJob*)) );
}

void MaildirResource::slotItemsRetrievalResult ( KJob* job )
{
  if ( job->error() )
    cancelTask( job->errorString() );
  else
    itemsRetrievalDone();
}

void MaildirResource::collectionAdded(const Collection & collection, const Collection &parent)
{
  if ( !ensureSaneConfiguration() ) {
    emit error( i18n("Unusable configuration.") );
    changeProcessed();
    return;
  }

  Maildir md = maildirForCollection( parent );
  kDebug( 5254 ) << md.subFolderList() << md.entryList();
  if ( mSettings->readOnly() || !md.isValid() ) {
    changeProcessed();
    return;
  }
  else {

    const QString collectionName( collection.name().replace( QDir::separator(), QString() ) );
    const QString newFolderPath = md.addSubFolder( collectionName );
    if ( newFolderPath.isEmpty() ) {
      changeProcessed();
      return;
    }

    kDebug( 5254 ) << md.subFolderList() << md.entryList();

    Collection col = collection;
    col.setRemoteId( collectionName );
    col.setName( collectionName );
    changeCommitted( col );
  }

}

void MaildirResource::collectionChanged(const Collection & collection)
{
  if ( !ensureSaneConfiguration() ) {
    emit error( i18n("Unusable configuration.") );
    changeProcessed();
    return;
  }

  if ( collection.parentCollection() == Collection::root() ) {
    if ( collection.name() != name() )
      setName( collection.name() );
    changeProcessed();
    return;
  }

  if ( collection.remoteId() == collection.name() ) {
    changeProcessed();
    return;
  }

  Maildir md = maildirForCollection( collection );
  if ( !md.isValid() ) {
    assert( !collection.remoteId().isEmpty() ); // caught in resourcebase
    // we don't have a maildir for this collection yet, probably due to a race
    // make one, otherwise the rename below will fail
    md.create();
  }

  const QString collectionName( collection.name().replace( QDir::separator(), QString() ) );
  if ( !md.rename( collectionName ) ) {
    emit error( i18n("Unable to rename maildir folder '%1'.", collection.name() ) );
    changeProcessed();
    return;
  }
  Collection c( collection );
  c.setRemoteId( collectionName );
  c.setName( collectionName );
  changeCommitted( c );
}

void MaildirResource::collectionMoved( const Collection &collection, const Collection &source, const Collection &dest )
{
  kDebug() << collection << source << dest;

  if ( !ensureSaneConfiguration() ) {
    emit error( i18n("Unusable configuration.") );
    changeProcessed();
    return;
  }

  if ( collection.parentCollection() == Collection::root() ) {
    emit error( i18n( "Cannot move root maildir folder '%1'." ,collection.remoteId() ) );
    changeProcessed();
    return;
  }

  if ( source == dest ) { // should not happen, but who knows...
    changeProcessed();
    return;
  }

  Collection c( collection );
  c.setParentCollection( source );
  Maildir md = maildirForCollection( c );
  Maildir destMd = maildirForCollection( dest );
  if ( !md.moveTo( destMd ) ) {
    emit error( i18n("Unable to move maildir folder '%1' from '%2' to '%3'.", collection.remoteId(), source.remoteId(), dest.remoteId() ) );
    changeProcessed();
  } else {
    changeCommitted( collection );
  }
}

void MaildirResource::collectionRemoved( const Akonadi::Collection &collection )
{
   if ( !ensureSaneConfiguration() ) {
    emit error( i18n("Unusable configuration.") );
    changeProcessed();
    return;
  }

  if ( collection.parentCollection() == Collection::root() ) {
    emit error( i18n("Cannot delete top-level maildir folder '%1'.", mSettings->path() ) );
    changeProcessed();
    return;
  }

  Maildir md = maildirForCollection( collection.parentCollection() );
  // !md.isValid() means that our parent folder has been deleted already,
  // so we don't care at all as that one will be recursive anyway
  if ( md.isValid() && !md.removeSubFolder( collection.remoteId() ) )
    emit error( i18n("Failed to delete sub-folder '%1'.", collection.remoteId() ) );
  changeProcessed();
}

void MaildirResource::ensureDirExists()
{
  Maildir root( mSettings->path() );
  if ( !root.isValid() && !mSettings->topLevelIsContainer() ) {
    if ( !root.create() )
      emit status( Broken, i18n( "Unable to create maildir '%1'.", mSettings->path() ) );
  }
}

bool MaildirResource::ensureSaneConfiguration()
{
  if ( mSettings->path().isEmpty() ) {
    emit status( Broken, i18n( "No usable storage location configured.") );
    setOnline( false );
    return false;
  }
  return true;
}

void MaildirResource::slotDirChanged(const QString& dir)
{  
  QFileInfo fileInfo(dir);
  if (fileInfo.isFile()) {
    slotFileChanged(dir);
    return;
  }
  
  if (dir == mSettings->path() ) {
    synchronizeCollectionTree();
   synchronizeCollection( Collection::root().id() );
    return;
  }

  if ( dir.endsWith( QLatin1String( ".directory") ) ) {
    synchronizeCollectionTree(); //might be too much, but this is not a common case anyway
    return;
  }
  
  QDir d( dir );
  if ( !d.cdUp() )
    return;

  const Maildir md( d.path() );
  if ( !md.isValid() )
    return;

  const Collection col = collectionForMaildir( md );
  if ( col.remoteId().isEmpty() ) {
    kDebug() << "unable to find collection for path" << dir;
    return;
  }
 
  CollectionFetchJob *job = new CollectionFetchJob( col, Akonadi::CollectionFetchJob::Base, this );
  connect( job, SIGNAL(result(KJob*)), SLOT(fsWatchDirFetchResult(KJob*)) );
}

void MaildirResource::fsWatchDirFetchResult(KJob* job)
{
  if ( job->error() ) {
    kDebug() << job->errorString();
    return;
  }
  const Collection::List cols = qobject_cast<CollectionFetchJob*>( job )->collections();
  if ( cols.isEmpty() )
    return;
  
  synchronizeCollection( cols.first().id() );
}

void MaildirResource::slotFileChanged( const QString& fileName )
{
  QFileInfo fileInfo( fileName );
  QString key = fileInfo.fileName();
  QString path = fileInfo.path();
  if ( path.endsWith( QLatin1String("/new") ) ) {
    path.remove( path.length() - 4, 4 );
  } else if ( path.endsWith( QLatin1String("/cur") ) ) {
    path.remove( path.length() - 4, 4 );
  }
  
  const Maildir md( path );
  if ( !md.isValid() )
    return;
      
  const Collection col = collectionForMaildir( md );
  if ( col.remoteId().isEmpty() ) {
    kDebug() << "unable to find collection for path" << fileInfo.path();
    return;
  }

  Item item;
  item.setRemoteId( key );
  item.setParentCollection( col );

  ItemFetchJob *job = new ItemFetchJob( item, this );
  job->setProperty("entry", key );
  job->setProperty("dir", path );
  connect( job, SIGNAL(result(KJob*)), SLOT(fsWatchFileFetchResult(KJob*)) );
}

void MaildirResource::fsWatchFileFetchResult( KJob* job )
{
  if ( job->error() ) {
    kDebug() << job->errorString();
    return;
  }
  Item::List items = qobject_cast<ItemFetchJob*>( job )->items();
  if ( items.isEmpty() )
    return;

  QString fileName = job->property( "entry" ).toString();
  QString path = job->property( "dir" ).toString();

  const Maildir md( path );

  QString entry = fileName;
  Item item ( items.at(0) );
  const qint64 entrySize = md.size( entry );
  if ( entrySize >= 0 )
    item.setSize( entrySize );

  Item::Flags flags = md.readEntryFlags( entry );
  Q_FOREACH( Item::Flag flag, flags ) {
    item.setFlag(flag);      
  }
    
  const QByteArray data = md.readEntry( entry );
  KMime::Message *mail = new KMime::Message();
  mail->setContent( KMime::CRLFtoLF( data ) );
  mail->parse();

  item.setPayload( KMime::Message::Ptr( mail ) );
    
  ItemModifyJob *mjob = new ItemModifyJob( item );
  connect( mjob, SIGNAL(result(KJob*)), SLOT(fsWatchFileModifyResult(KJob*)) );
}

void MaildirResource::fsWatchFileModifyResult(KJob* job)
{
  if ( job->error() ) {
    kDebug() << job->errorString();
    return;
  }
}


#include "maildirresource.moc"
