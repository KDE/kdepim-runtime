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

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtDBus/QDBusConnection>

#include <kdebug.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <klocale.h>

#include <libakonadi/collectionlistjob.h>
#include <libakonadi/collectionmodifyjob.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>

#include <maildir/maildir.h>

#include <kmime/kmime_message.h>

#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;
using KPIM::Maildir;

MaildirResource::MaildirResource( const QString &id )
    :ResourceBase( id )
{
}

MaildirResource::~ MaildirResource()
{
}

bool MaildirResource::requestItemDelivery( const Akonadi::DataReference &ref, const QStringList &parts, const QDBusMessage &msg )
{
  const QString rid = ref.remoteId();
  const QString dir = rid.left( rid.lastIndexOf( QDir::separator() ) );
  const QString entry = rid.mid( rid.lastIndexOf( QDir::separator() ) + 1 );

  Maildir md( dir );
  if ( !md.isValid() )
    return false;

  const QByteArray data = md.readEntry( entry );
  KMime::Message *mail = new KMime::Message();
  mail->setContent( data );
  mail->parse();

  Item item( ref );
  item.setMimeType( "message/rfc822" );
  item.setPayload( MessagePtr( mail ) );
  ItemStoreJob *job = new ItemStoreJob( item, session() );
  job->storePayload();

  return deliverItem( job, msg );
}

void MaildirResource::aboutToQuit()
{
  kDebug() << "Implement me: " << k_funcinfo << endl;
}

void MaildirResource::configure()
{
  QString oldDir = settings()->value( "General/Path" ).toString();
  KUrl url;
  if ( !oldDir.isEmpty() )
    url = KUrl::fromPath( oldDir );
  else
    url = KUrl::fromPath( QDir::homePath() );
  QString newDir = KFileDialog::getExistingDirectory( url, 0, i18n("Select Mail Directory") );
  if ( newDir.isEmpty() )
    return;
  if ( oldDir == newDir )
    return;
  settings()->setValue( "General/Path", newDir );
  synchronize();
}

void MaildirResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& collection )
{
    Maildir dir( collection.remoteId() );
    QString errMsg;
    if ( !dir.isValid( errMsg ) ) {
      error( errMsg );
      return;
    }
    // we can only deal with mail
    if ( item.mimeType() != "message/rfc822" ) {
      error( i18n("Only email messages can be added to the Maildir resource!") );
      return;
    }
    const MessagePtr mail = item.payload<MessagePtr>();
    dir.addEntry( mail->encodedContent() );
}

void MaildirResource::itemChanged( const Akonadi::Item& item, const QStringList& parts )
{
    if ( !parts.contains( Item::PartBody ) ) return;

    const QString rid = item.reference().remoteId();
    const QString path = rid.left( rid.lastIndexOf( QDir::separator() ) );
    const QString entry = rid.mid( rid.lastIndexOf( QDir::separator() ) + 1 );

    Maildir dir( path );
    QString errMsg;
    if ( !dir.isValid( errMsg ) ) {
        error( errMsg );
        return;
    }
    // we can only deal with mail
    if ( item.mimeType() != "message/rfc822" ) {
        error( i18n("Only email messages can be added to the Maildir resource!") );
        return;
    }
    const MessagePtr mail = item.payload<MessagePtr>();
    dir.writeEntry( entry, mail->encodedContent() );
}

void MaildirResource::itemRemoved(const Akonadi::DataReference & ref)
{
  kDebug() << "Implement me: " << k_funcinfo << endl;
 }

void MaildirResource::retrieveCollections()
{
  Maildir dir( settings()->value( "General/Path" ).toString() );
  QString errMsg;
  if ( !dir.isValid( errMsg ) ) {
    error( errMsg );
    collectionsRetrieved( Collection::List() );
  }

  Collection root;
  root.setParent( Collection::root() );
  root.setRemoteId( settings()->value( "General/Path" ).toString() );
  root.setName( name() );
  QStringList mimeTypes;
  mimeTypes << "message/rfc822" << Collection::collectionMimeType();
  root.setContentTypes( mimeTypes );
  root.setCachePolicyId( 1 ); // ### just for testing

  Collection::List list;
  list << root;
  foreach ( const QString sub, dir.subFolderList() ) {
    Collection c;
    c.setRemoteId( sub );
    c.setParent( root );
    c.setContentTypes( mimeTypes );
    list << c;
  }
  collectionsRetrieved( list );
}

void MaildirResource::synchronizeCollection(const Akonadi::Collection & col)
{
  ItemFetchJob *fetch = new ItemFetchJob( col, session() );
  if ( !fetch->exec() ) {
    changeStatus( Error, i18n("Unable to fetch listing of collection '%1': %2", col.name(), fetch->errorString()) );
    return;
  }
  Item::List items = fetch->items();

  changeProgress( 0 );

  Maildir md( col.remoteId() );
  if ( !md.isValid() ) {
    error( i18n("Invalid maildir" ) );
    return;
  }
  QStringList entryList = md.entryList();

  int counter = 0;
  foreach ( const QString entry, entryList ) {
    const QString rid = col.remoteId() + QDir::separator() + entry;
    bool found = false;
    foreach ( Item item, items ) {
      if ( item.reference().remoteId() == rid ) {
        found = true;
        break;
      }
    }
    if ( found )
      continue;
    Item item( DataReference( -1, rid ) );
    item.setMimeType( "message/rfc822" );
    ItemAppendJob *append = new ItemAppendJob( item, col, session() );
    if ( !append->exec() ) {
      changeProgress( 0 );
      changeStatus( Error, i18n("Appending new message failed: %1", append->errorString()) );
      return;
    }

    counter++;
    int percentage = (counter * 100) / entryList.count();
    changeProgress( percentage );
  }
  collectionSynchronized();
}

#include "maildirresource.moc"
