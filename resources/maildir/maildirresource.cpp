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

#include <QtCore/QDir>
#include <QtDBus/QDBusConnection>

#include <akonadi/kmime/messageparts.h>
#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/cachepolicy.h>
#include <akonadi/dbusconnectionpool.h>

#include <kdebug.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <klocale.h>

#include <KWindowSystem>

#include "libmaildir/maildir.h"

#include <kmime/kmime_message.h>

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

MaildirResource::MaildirResource( const QString &id )
    :ResourceBase( id ),
    mSettings( new MaildirSettings( componentData().config() ) )
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

  setHierarchicalRemoteIdentifiersEnabled( true );

  ItemFetchScope scope( changeRecorder()->itemFetchScope() );
  scope.fetchFullPayload( false );
  scope.fetchPayloadPart( MessagePart::Header );
  scope.setAncestorRetrieval( ItemFetchScope::None );
  setItemSynchronizationFetchScope( scope );
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
    const QString rid = dir.addEntry( mail->encodedContent() );

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

    bool payloadChanged = false;
    Q_FOREACH( const QByteArray &part, parts )  {
      if( part.startsWith("PLD:") ) {
        payloadChanged = true;
        break;
      }
    }
    if ( mSettings->readOnly() || !payloadChanged ) {
      changeProcessed();
      return;
    }

    Maildir dir = maildirForCollection( item.parentCollection() );
    QString errMsg;
    if ( !dir.isValid( errMsg ) ) {
        cancelTask( errMsg );
        return;
    }
    // we can only deal with mail
    if ( !item.hasPayload<KMime::Message::Ptr>() ) {
        cancelTask( i18n("Error: Unsupported type.") );
        return;
    }
    const KMime::Message::Ptr mail = item.payload<KMime::Message::Ptr>();
    dir.writeEntry( item.remoteId(), mail->encodedContent() );
    changeCommitted( item );
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

  const QString newRid = sourceDir.moveEntryTo( item.remoteId(), destDir );
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
    if ( dir.isValid() && !dir.removeEntry( item.remoteId() ) ) {
      emit error( i18n("Failed to delete message: %1", item.remoteId()) );
    }
  }
  changeProcessed();
}

Collection::List MaildirResource::listRecursive( const Collection &root, const Maildir &dir )
{
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
  const QStringList entryList = md.entryList();

  Item::List items;
  foreach ( const QString &entry, entryList ) {
    Item item;
    item.setRemoteId( entry );
    item.setMimeType( itemMimeType() );
    item.setSize( md.size( entry ) );
    KMime::Message *msg = new KMime::Message;
    msg->setHead( KMime::CRLFtoLF( md.readEntryHeaders( entry ) ) );
    msg->parse();
    item.setPayload( KMime::Message::Ptr( msg ) );
    items << item;
  }
  itemsRetrieved( items );
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
  if ( !root.isValid() ) {
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

#include "maildirresource.moc"
