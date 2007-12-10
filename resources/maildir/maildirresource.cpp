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

static QString maildirSubdirPath( const QString &parentPath, const QString &subName )
{
  const int pos = parentPath.lastIndexOf( QDir::separator() );
  QString basePath = maildirPath( parentPath );
  QString name = maildirId( parentPath );
  return basePath + QDir::separator() + '.' + name + ".directory" + QDir::separator() + subName;
}

MaildirResource::MaildirResource( const QString &id )
    :ResourceBase( id )
{
}

MaildirResource::~ MaildirResource()
{
}

bool MaildirResource::retrieveItem( const Akonadi::Item &item, const QStringList &parts )
{
  const QString dir = maildirPath( item.reference().remoteId() );
  const QString entry = maildirId( item.reference().remoteId() );

  Maildir md( dir );
  if ( !md.isValid() )
    return false;

  const QByteArray data = md.readEntry( entry );
  KMime::Message *mail = new KMime::Message();
  mail->setContent( data );
  mail->parse();

  Item i( item );
  i.setMimeType( "message/rfc822" );
  i.setPayload( MessagePtr( mail ) );
  itemRetrieved( i );
  return true;
}

void MaildirResource::aboutToQuit()
{
  kDebug( 5254 ) << "Implement me!" ;
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
//   synchronize();
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
    const QString rid = collection.remoteId() + QDir::separator() + dir.addEntry( mail->encodedContent() );
    DataReference ref = item.reference();
    ref.setRemoteId( rid );
    changesCommitted( ref );
}

void MaildirResource::itemChanged( const Akonadi::Item& item, const QStringList& parts )
{
    if ( !parts.contains( Item::PartBody ) ) return;

    const QString path = maildirPath( item.reference().remoteId() );
    const QString entry = maildirId( item.reference().remoteId() );

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
    changesCommitted( item.reference() );
}

void MaildirResource::itemRemoved(const Akonadi::DataReference & ref)
{
    const QString path = maildirPath( ref.remoteId() );
    const QString entry = maildirId( ref.remoteId() );

    Maildir dir( path );
    QString errMsg;
    if ( !dir.isValid( errMsg ) ) {
        error( errMsg );
        return;
    }
    if ( !dir.removeEntry( entry ) ) {
        error( i18n("Failed to delete item: %1", ref.remoteId()) );
    }
}

Collection::List listRecursive( const Collection &root, const Maildir &dir )
{
  Collection::List list;
  const QStringList mimeTypes = QStringList() << "message/rfc822" << Collection::collectionMimeType();
  foreach ( const QString sub, dir.subFolderList() ) {
    const QString path = maildirSubdirPath( root.remoteId(), sub );
    Maildir md( path );
    if ( !md.isValid() )
      continue;
    Collection c;
    c.setName( sub );
    c.setRemoteId( path );
    c.setParent( root );
    c.setContentTypes( mimeTypes );
    list << c;
    list += listRecursive( c, md );
  }
  return list;
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
  list += listRecursive( root, dir );
  collectionsRetrieved( list );
}

void MaildirResource::retrieveItems(const Akonadi::Collection & col, const QStringList &parts)
{
  Maildir md( col.remoteId() );
  if ( !md.isValid() ) {
    error( i18n("Invalid maildir: %1", col.remoteId() ) );
    return;
  }
  QStringList entryList = md.entryList();

  int counter = 0;
  Item::List items;
  foreach ( const QString entry, entryList ) {
    const QString rid = col.remoteId() + QDir::separator() + entry;
    Item item( DataReference( -1, rid ) );
    item.setMimeType( "message/rfc822" );
    KMime::Message *msg = new KMime::Message;
    msg->setHead( readHeader( rid ) );
    msg->parse();
    item.setPayload( MessagePtr( msg ) );
    items << item;
  }
  itemsRetrieved( items );
}

void MaildirResource::collectionAdded(const Collection & collection, const Collection &parent)
{
  Maildir md( parent.remoteId() );
  kDebug( 5254 ) << md.subFolderList() << md.entryList();
  if ( !md.isValid() )
    return;
  if ( !md.addSubFolder( collection.name() ) )
    return;
  kDebug( 5254 ) << md.subFolderList() << md.entryList();

  Collection col = collection;
  col.setRemoteId( parent.remoteId() + QDir::separator() + collection.name() );
  CollectionModifyJob *mjob = new CollectionModifyJob( col );
}

void MaildirResource::collectionChanged(const Collection & collection)
{
  kDebug( 5254 ) << "Implement me!";
}

void MaildirResource::collectionRemoved(int id, const QString & remoteId)
{
  kDebug( 5254 ) << "Implement me!";
}

QByteArray MaildirResource::readHeader(const QString & fileName)
{
  const QString dir = maildirPath( fileName );
  const QString entry = maildirId( fileName );
  // ### FIXME Maildir can only read complete entries, we should avoid that
  // but "cur" is not the only place to look for mails
  QFile f( dir + QDir::separator() + "cur" + QDir::separator() + entry );
  if ( !f.open( QFile::ReadOnly ) )
    return QByteArray();
  QByteArray rv;
  forever {
    QByteArray line = f.readLine();
    if ( line.trimmed().isEmpty() )
      break;
    rv.append( line );
  }
  return rv;
}

#include "maildirresource.moc"
