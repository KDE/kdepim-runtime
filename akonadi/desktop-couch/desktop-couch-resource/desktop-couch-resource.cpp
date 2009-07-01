/*
    Copyright (c) 2009 Canonical

    Author: Till Adam <till@kdab.net>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2.1 or version 3 of the license,
    or later versions approved by the membership of KDE e.V.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "desktop-couch-resource.h"
//#include "settingsadaptor.h"

#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <KWindowSystem>

#include <QtDBus/QDBusConnection>
#include <QDebug>
using namespace Akonadi;

DesktopCouchResource::DesktopCouchResource( const QString &id )
  : ResourceBase( id )
{
  setName( i18n("Desktop Couch Addressbook") );
//  setSupportedMimetypes( QStringList() << KABC::Addressee::mimeType(), "office-address-book" );
/*
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
                            */
  changeRecorder()->itemFetchScope().fetchFullPayload();
}

DesktopCouchResource::~DesktopCouchResource()
{
}

bool DesktopCouchResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  const QString rid = item.remoteId();
  db.disconnect( this, SLOT( slotDocumentRetrieved(QVariant) ) );
  db.connect( &db, SIGNAL( documentRetrieved(QVariant) ),
              this, SLOT( slotDocumentRetrieved(QVariant) ) );
  CouchDBDocumentInfo info;
  info.setId( rid );
  info.setDatabase( "contacts" );
  db.requestDocument( info );

  setProperty( "akonadiItem", QVariant::fromValue(item) );

  return true;
}

static KABC::Addressee variantToAddressee( const QVariant& v )
{
  KABC::Addressee a;
  a.setUid( v.toMap()["_id"].toString() );
  a.setGivenName( v.toMap()["first_name"].toString() );
  a.setFamilyName( v.toMap()["last_name"].toString() );

  // extract email addresses
  const QVariantMap emails = v.toMap()["email_addresses"].toMap();
  Q_FOREACH( QVariant email, emails ) {
    QVariantMap emailmap = email.toMap();
    const QString emailstr = emailmap["address"].toString();
    if ( !emailstr.isEmpty() )
      a.insertEmail( emailstr );
  }
  return a;
}

void DesktopCouchResource::slotDocumentRetrieved( const QVariant& v )
{


  KABC::Addressee a( variantToAddressee( v ) );

  Item i( property("akonadiItem").value<Item>() );
  Q_ASSERT( i.remoteId() == a.uid() );
  i.setMimeType( KABC::Addressee::mimeType() );
  i.setPayload<KABC::Addressee>( a );
  itemRetrieved( i );
}

void DesktopCouchResource::aboutToQuit()
{
  //Settings::self()->writeConfig();
}

void DesktopCouchResource::configure( WId windowId )
{
    /*
  SingleFileResourceConfigDialog<Settings> dlg( windowId );
  dlg.setFilter( "*.vcf|" + i18nc("Filedialog filter for *.vcf", "vCard Address Book File" ) );
  dlg.setCaption( i18n("Select Address Book") );
  if ( dlg.exec() == QDialog::Accepted ) {
    reloadFile();
  }
  */
}

void DesktopCouchResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
{
  KABC::Addressee addressee;
  if ( item.hasPayload<KABC::Addressee>() )
    addressee  = item.payload<KABC::Addressee>();

  if ( !addressee.isEmpty() ) {
//    mAddressees.insert( addressee.uid(), addressee );

    Item i( item );
    i.setRemoteId( addressee.uid() );
    changeCommitted( i );

  } else {
    changeProcessed();
  }
}

void DesktopCouchResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  KABC::Addressee addressee;
  if ( item.hasPayload<KABC::Addressee>() )
    addressee  = item.payload<KABC::Addressee>();

  if ( !addressee.isEmpty() ) {
  //  mAddressees.insert( addressee.uid(), addressee );

    Item i( item );
    i.setRemoteId( addressee.uid() );
    changeCommitted( i );

  } else {
    changeProcessed();
  }
}

void DesktopCouchResource::itemRemoved(const Akonadi::Item & item)
{
//  if ( mAddressees.contains( item.remoteId() ) )
//    mAddressees.remove( item.remoteId() );


  changeProcessed();
}

void DesktopCouchResource::retrieveItems( const Akonadi::Collection & col )
{
  // CouchDB does not support folders so we can safely ignore the collection
  Q_UNUSED( col );

  db.disconnect( this, SLOT( slotDocumentsListed(CouchDBDocumentInfoList) ) );
  db.connect( &db, SIGNAL( documentsListed( CouchDBDocumentInfoList ) ),
              this, SLOT( slotDocumentsListed(CouchDBDocumentInfoList) ) );
  db.requestDocumentListing( "contacts" );
}

void DesktopCouchResource::slotDocumentsListed( const CouchDBDocumentInfoList& list )
{
  Item::List items;
  Q_FOREACH( const CouchDBDocumentInfo& info, list ) {
    Item item;
    item.setRemoteId( info.id() );
    item.setMimeType( KABC::Addressee::mimeType() );
    items.append( item );
  }

  itemsRetrieved( items );
}


void DesktopCouchResource::retrieveCollections()
{
  Collection c;
  c.setParent( Collection::root() );
  c.setRemoteId( QLatin1String("contacts_root") );
  c.setName( name() );

  QStringList mimeTypes;
  mimeTypes << "text/directory";
  c.setContentMimeTypes( mimeTypes );

  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

AKONADI_RESOURCE_MAIN( DesktopCouchResource )

#include "desktop-couch-resource.moc"
