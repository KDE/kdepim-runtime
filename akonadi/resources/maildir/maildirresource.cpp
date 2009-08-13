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
#include "settingsadaptor.h"
#include "configdialog.h"

#include <QtCore/QDir>
#include <QtDBus/QDBusConnection>

#include <akonadi/kmime/messageparts.h>
#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/collectionfetchscope.h>

#include <kdebug.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <klocale.h>

#include <KWindowSystem>

#include "libmaildir/maildir.h"

#include <kmime/kmime_message.h>

#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;
using KPIM::Maildir;

static QString maildirPath( const QString &remoteId )
{
  const int pos = remoteId.lastIndexOf( QDir::separator() );
  if ( pos >= 0 )
    return remoteId.left( pos );
  return QString();
}

static QString maildirId( const QString &remoteId )
{
  const int pos = remoteId.lastIndexOf( QDir::separator() );
  if ( pos >= 0 )
    return remoteId.mid( pos + 1 );
  return QString();
}

/** Creates a maildir object for the collection @p col, given it has the full ancestor chain set. */
static Maildir maildirForCollection( const Collection &col )
{
  if ( col.remoteId().isEmpty() ) {
    kWarning() << "Got incomplete ancestor chain:" << col;
    return Maildir();
  }

  if ( col.parentCollection() == Collection::root() ) {
    kWarning( col.remoteId() != Settings::self()->path() ) << "RID mismatch, is " << col.remoteId() << " expected " << Settings::self()->path();
    return Maildir( col.remoteId(), Settings::self()->topLevelIsContainer() );
  }
  Maildir parentMd = maildirForCollection( col.parentCollection() );
  return parentMd.subFolder( col.remoteId() );
}

MaildirResource::MaildirResource( const QString &id )
    :ResourceBase( id )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                              Settings::self(), QDBusConnection::ExportAdaptors );
  connect( this, SIGNAL(reloadConfiguration()), SLOT(ensureDirExists()) );

  // We need to enable this here, otherwise we neither get the remote ID of the
  // parent collection when a collection changes, nor the full item when an item
  // is added.
  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().fetchFullPayload( true );
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::All );
  changeRecorder()->collectionFetchScope().setAncestorRetrieval( CollectionFetchScope::All );
}

MaildirResource::~ MaildirResource()
{
}

bool MaildirResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );

  const QString dir = maildirPath( item.remoteId() );
  const QString entry = maildirId( item.remoteId() );

  Maildir md( dir );
  if ( !md.isValid() ) {
    cancelTask( i18n( "Unable to fetch item: The maildir folder \"%1\" is not valid.",
                      md.path() ) );
    return false;
  }

  const QByteArray data = md.readEntry( entry );
  KMime::Message *mail = new KMime::Message();
  mail->setContent( KMime::CRLFtoLF( data ) );
  mail->parse();

  Item i( item );
  i.setPayload( MessagePtr( mail ) );
  itemRetrieved( i );
  return true;
}

void MaildirResource::aboutToQuit()
{
  // The settings may not have been saved if e.g. they have been modified via
  // DBus instead of the config dialog.
  Settings::self()->writeConfig();
}

void MaildirResource::configure( WId windowId )
{
  ConfigDialog dlg;
  if ( windowId )
    KWindowSystem::setMainWindow( &dlg, windowId );
  dlg.exec();

  ensureDirExists();
  synchronizeCollectionTree();
}

void MaildirResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& collection )
{
    Maildir dir = maildirForCollection( collection );
    QString errMsg;
    if ( Settings::readOnly() || !dir.isValid( errMsg ) ) {
      cancelTask( errMsg );
      return;
    }
    // we can only deal with mail
    if ( item.mimeType() != "message/rfc822" ) {
      cancelTask( i18n("Only email messages can be added to the Maildir resource.") );
      return;
    }
    const MessagePtr mail = item.payload<MessagePtr>();
    const QString rid = collection.remoteId() + QDir::separator() + dir.addEntry( mail->encodedContent() );
    Item i( item );
    i.setRemoteId( rid );
    changeCommitted( i );
}

void MaildirResource::itemChanged( const Akonadi::Item& item, const QSet<QByteArray>& parts )
{
    if ( Settings::self()->readOnly() || !parts.contains( MessagePart::Body ) ) {
      changeProcessed();
      return;
    }

    const QString path = maildirPath( item.remoteId() );
    const QString entry = maildirId( item.remoteId() );

    Maildir dir( path );
    QString errMsg;
    if ( !dir.isValid( errMsg ) ) {
        cancelTask( errMsg );
        return;
    }
    // we can only deal with mail
    if ( item.mimeType() != "message/rfc822" ) {
        cancelTask( i18n("Only email messages can be added to the Maildir resource.") );
        return;
    }
    const MessagePtr mail = item.payload<MessagePtr>();
    dir.writeEntry( entry, mail->encodedContent() );
    changeCommitted( item );
}

void MaildirResource::itemRemoved(const Akonadi::Item & item)
{
  if ( !Settings::self()->readOnly() ) {
    const QString path = maildirPath( item.remoteId() );
    const QString entry = maildirId( item.remoteId() );

    Maildir dir( path );
    QString errMsg;
    if ( !dir.isValid( errMsg ) ) {
      cancelTask( errMsg );
      return;
    }
    if ( !dir.removeEntry( entry ) ) {
      emit error( i18n("Failed to delete item: %1", item.remoteId()) );
    }
  }
  changeProcessed();
}

Collection::List listRecursive( const Collection &root, const Maildir &dir )
{
  Collection::List list;
  const QStringList mimeTypes = QStringList() << "message/rfc822" << Collection::mimeType();
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
  Maildir dir( Settings::self()->path(), Settings::self()->topLevelIsContainer() );
  QString errMsg;
  if ( !dir.isValid( errMsg ) ) {
    emit error( errMsg );
    collectionsRetrieved( Collection::List() );
    return;
  }

  Collection root;
  root.setParentCollection( Collection::root() );
  root.setRemoteId( Settings::self()->path() );
  root.setName( name() );
// FIXME: enable once r1010699 is merged, doesn't build otherwise
//  root.setRights( Collection::CanChangeItem | Collection::CanCreateItem | Collection::CanDeleteItem 
//                | Collection::CanChangeCollection | Collection::CanCreateCollection );
  QStringList mimeTypes;
  mimeTypes << Collection::mimeType();
  if ( !Settings::self()->topLevelIsContainer() )
    mimeTypes << "message/rfc822";
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
    const QString rid = md.path() + QDir::separator() + entry;
    Item item;
    item.setRemoteId( rid );
    item.setMimeType( "message/rfc822" );
    item.setSize( md.size( entry ) );
    KMime::Message *msg = new KMime::Message;
    msg->setHead( KMime::CRLFtoLF( md.readEntryHeaders( entry ) ) );
    msg->parse();
    item.setPayload( MessagePtr( msg ) );
    items << item;
  }
  itemsRetrieved( items );
}

void MaildirResource::collectionAdded(const Collection & collection, const Collection &parent)
{
  Maildir md = maildirForCollection( parent );
  kDebug( 5254 ) << md.subFolderList() << md.entryList();
  if ( Settings::self()->readOnly() || !md.isValid() ) {
    changeProcessed();
    return;
  }
  else {

    const QString newFolderPath = md.addSubFolder( collection.name() );
    if ( newFolderPath.isEmpty() ) {
      changeProcessed();
      return;
    }

    kDebug( 5254 ) << md.subFolderList() << md.entryList();

    Collection col = collection;
    col.setRemoteId( collection.name() );
    changeCommitted( col );
  }

}

void MaildirResource::collectionChanged(const Collection & collection)
{
  kDebug( 5254 ) << "Implement me!";
  AgentBase::Observer::collectionChanged( collection );
}

void MaildirResource::collectionRemoved( const Akonadi::Collection &collection )
{
  if ( collection.parentCollection() == Collection::root() ) {
    emit error( i18n("Cannot delete top-level maildir folder '%1'.", Settings::self()->path() ) );
    changeProcessed();
    return;
  }

  Maildir md = maildirForCollection( collection.parentCollection() );
  if ( !md.removeSubFolder( collection.remoteId() ) )
    emit error( i18n("Failed to delete sub-folder '%1'.", collection.remoteId() ) );
  changeProcessed();
}

void MaildirResource::ensureDirExists()
{
  Maildir root( Settings::self()->path() );
  if ( !root.isValid() ) {
    if ( !root.create() )
      emit status( Broken, i18n( "Unable to create maildir '%1'.", Settings::self()->path() ) );
  }
}


AKONADI_RESOURCE_MAIN( MaildirResource )

#include "maildirresource.moc"
