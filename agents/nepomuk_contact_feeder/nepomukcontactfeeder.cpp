/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
                  2008 Sebastian Trueg <trueg@kde.org>

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

#include "nepomukcontactfeeder.h"

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>

#include <akonadi/changerecorder.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/mimetypechecker.h>

#include <nepomuk/resource.h>
#include <nepomuk/resourcemanager.h>
#include <nepomuk/variant.h>
#include <kurl.h>

#include <Soprano/Model>
#include <Soprano/NodeIterator>
#include <Soprano/QueryResultIterator>
#include <soprano/nrl.h>

#define USING_SOPRANO_NRLMODEL_UNSTABLE_API 1
#include <soprano/nrlmodel.h>

// ontology includes
#include "bbsnumber.h"
#include "carphonenumber.h"
#include "cellphonenumber.h"
#include "contactgroup.h"
#include "emailaddress.h"
#include "faxnumber.h"
#include "isdnnumber.h"
#include "messagingnumber.h"
#include "modemnumber.h"
#include "pagernumber.h"
#include "personcontact.h"
#include "pcsnumber.h"
#include "postaladdress.h"
#include "videotelephonenumber.h"
#include "voicephonenumber.h"
#include "website.h"

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>

#include <KDebug>

namespace Akonadi {

static void removeItemFromNepomuk( const Akonadi::Item &item )
{
  // find the graph that contains our item and delete the complete graph
  QList<Soprano::Node> list = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(
                QString( "select ?g where { ?g <http://www.semanticdesktop.org/ontologies/2007/01/19/nie#dataGraphFor> %1. }")
                       .arg( Soprano::Node::resourceToN3( item.url() ) ),
                Soprano::Query::QueryLanguageSparql ).iterateBindings( 0 ).allNodes();

  foreach ( const Soprano::Node &node, list )
    Nepomuk::ResourceManager::instance()->mainModel()->removeContext( node );
}

NepomukContactFeeder::NepomukContactFeeder( const QString &id )
  : AgentBase( id ),
    mForceUpdate( false )
{
  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->setMimeTypeMonitored( KABC::Addressee::mimeType() );
  changeRecorder()->setMimeTypeMonitored( KABC::ContactGroup::mimeType() );
  changeRecorder()->setChangeRecordingEnabled( false );

  // do the initial scan to make sure all items have been fed to nepomuk
  QTimer::singleShot( 0, this, SLOT( slotInitialItemScan() ) );

  // The line below is not necessary anymore once AgentBase also exports scriptable slots
  QDBusConnection::sessionBus().registerObject( "/nepomukcontactfeeder", this, QDBusConnection::ExportScriptableSlots );

  // initialize Nepomuk
  Nepomuk::ResourceManager::instance()->init();

  mNrlModel = new Soprano::NRLModel( Nepomuk::ResourceManager::instance()->mainModel() );
}

NepomukContactFeeder::~NepomukContactFeeder()
{
  delete mNrlModel;
}

void NepomukContactFeeder::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
{
  itemChanged( item, QSet<QByteArray>() );
}

void NepomukContactFeeder::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  if ( item.hasPayload<KABC::Addressee>() || item.hasPayload<KABC::ContactGroup>() )
    updateItem( item );
}

void NepomukContactFeeder::itemRemoved( const Akonadi::Item &item )
{
  removeItemFromNepomuk( item );
}

void NepomukContactFeeder::slotInitialItemScan()
{
  kDebug();
  updateAll( false );
}

void NepomukContactFeeder::updateAll( bool force )
{
  mForceUpdate = force;

  CollectionFetchJob *collectionFetch = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  if ( collectionFetch->exec() ) {
    Collection::List collections = collectionFetch->collections();

    MimeTypeChecker contactFilter;
    contactFilter.addWantedMimeType( KABC::Addressee::mimeType() );
    contactFilter.addWantedMimeType( KABC::ContactGroup::mimeType() );
    foreach( const Collection &collection, collections) {
      kDebug() << "checking collection" << collection.name();
      if ( contactFilter.isWantedCollection( collection ) ) {
        kDebug() << "fetching items from collection" << collection.name();
        ItemFetchJob *itemFetch = new ItemFetchJob( collection );
        itemFetch->fetchScope().fetchFullPayload();
        connect( itemFetch, SIGNAL( itemsReceived( Akonadi::Item::List ) ),
                 this, SLOT( slotItemsReceivedForInitialScan( Akonadi::Item::List ) ) );
      }
    }
  }
}

void NepomukContactFeeder::slotItemsReceivedForInitialScan( const Akonadi::Item::List& items )
{
  kDebug() << items.count();
  foreach( const Item &item, items ) {
    // only update the item if it does not exist
    if ( mForceUpdate ||
         !Nepomuk::ResourceManager::instance()->mainModel()->containsAnyStatement( item.url(), Soprano::Node(), Soprano::Node() ) ) {
      // FIXME: the idea is that we only fetch the full payload for items that need updating
      //        However, the code below always returns an empty list!
//       ItemFetchJob *itemFetch = new ItemFetchJob( item );
//       itemFetch->fetchScope().fetchFullPayload();
//       if ( itemFetch->exec() ) {
//         Akonadi::Item::List fullItems = itemFetch->items();
//         if ( fullItems.isEmpty() ) {
//           kDebug() << "Failed to get full payload for item" << item.url();
//         }
//         else {
//           updateItem( itemFetch->items().first() );
//         }
//       }
      updateItem( item );
    }
  }
  kDebug() << "done";
}

namespace {
  inline QStringList listFromString( const QString& s ) {
    if ( s.isEmpty() )
      return QStringList();
    else
      return QStringList( s );
  }
}

void NepomukContactFeeder::updateItem( const Akonadi::Item &item )
{
  if ( !item.hasPayload<KABC::Addressee>() && !item.hasPayload<KABC::ContactGroup>() ) {
    kDebug() << "Got item without payload. Mimetype:" << item.mimeType()
             << "Id:" << item.id();
    return;
  }

  // first remove the item: since we use a graph that has a reference to all parts
  // of the item's semantic representation this is a really fast operation
  removeItemFromNepomuk( item );

  // create a new graph for the item
  QUrl metaDataGraphUri;
  QUrl graphUri = mNrlModel->createGraph( Soprano::Vocabulary::NRL::InstanceBase(), &metaDataGraphUri );

  // remember to which graph the item belongs to (used in search query in removeItemFromNepomuk())
  mNrlModel->addStatement( graphUri,
                           QUrl::fromEncoded( "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#dataGraphFor", QUrl::StrictMode ),
                           item.url(), metaDataGraphUri );

  if ( item.hasPayload<KABC::Addressee>() )
    updateContactItem( item, graphUri );
  else
    updateGroupItem( item, graphUri );
}

void NepomukContactFeeder::updateContactItem( const Akonadi::Item &item, const QUrl &graphUri )
{
  // create the contact with the graph reference
  NepomukFast::PersonContact contact( item.url(), graphUri );

  const KABC::Addressee addressee = item.payload<KABC::Addressee>();

  if ( !addressee.formattedName().isEmpty() )
    contact.setLabel( addressee.formattedName() );
  else
    contact.setLabel( addressee.assembledName() );

  if ( !addressee.givenName().isEmpty() )
    contact.setNameGivens( listFromString( addressee.givenName() ) );

  if ( !addressee.additionalName().isEmpty() )
    contact.setNameAdditionals( listFromString( addressee.additionalName() ) );

  if ( !addressee.familyName().isEmpty() )
    contact.setNameFamilys( listFromString( addressee.familyName() ) );

  if ( !addressee.prefix().isEmpty() )
    contact.setNameHonorificPrefixs( listFromString( addressee.prefix() ) );

  if ( !addressee.suffix().isEmpty() )
    contact.setNameHonorificSuffixs( listFromString( addressee.suffix() ) );

  const KABC::Geo geo = addressee.geo();
  if ( geo.isValid() ) {
    QString geoString;
    geoString.sprintf( "%.6f;%.6f", geo.latitude(), geo.longitude() );
    contact.setLocations( listFromString( geoString ) ); // make it better
  }

  // keys
  // sounds
  // logos
  // photos
  if ( !addressee.note().isEmpty() )
    contact.setNotes( listFromString( addressee.note() ) );

  if ( !addressee.nickName().isEmpty() )
    contact.setNicknames( listFromString( addressee.nickName() ) );

  contact.setContactUIDs( listFromString( addressee.uid() ) ); // never empty

  if ( !addressee.name().isEmpty() )
    contact.setFullnames( listFromString( addressee.name() ) );

  if ( addressee.birthday().date().isValid() )
    contact.setBirthDate( addressee.birthday().date() );

  if ( addressee.url().isValid() ) {
    NepomukFast::Website website( addressee.url(), graphUri );
    contact.addWebsiteUrl( website );
  }

  // phone numbers
  const KABC::PhoneNumber::List phoneNumbers = addressee.phoneNumbers();
  for ( int i = 0; i < phoneNumbers.count(); ++i ) {
    if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Bbs ) {
      NepomukFast::BbsNumber number( QUrl(), graphUri );
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Car ) {
      NepomukFast::CarPhoneNumber number( QUrl(), graphUri );
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Cell ) {
      NepomukFast::CellPhoneNumber number( QUrl(), graphUri );
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Fax ) {
      NepomukFast::FaxNumber number( QUrl(), graphUri );
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Isdn ) {
      NepomukFast::IsdnNumber number( QUrl(), graphUri );
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Msg ) {
      NepomukFast::MessagingNumber number( QUrl(), graphUri );
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Modem ) {
      NepomukFast::ModemNumber number( QUrl(), graphUri );
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Pager ) {
      NepomukFast::PagerNumber number( QUrl(), graphUri );
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Pcs ) {
      NepomukFast::PcsNumber number( QUrl(), graphUri );
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Video ) {
      NepomukFast::VideoTelephoneNumber number( QUrl(), graphUri );
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Voice ) {
      NepomukFast::VoicePhoneNumber number( QUrl(), graphUri );
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else { // matches Home and Work
      NepomukFast::PhoneNumber number( QUrl(), graphUri );
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    }
  }

  // im accounts

  // email addresses
  const QStringList emails = addressee.emails();
  for ( int i = 0; i < emails.count(); ++i ) {
    NepomukFast::EmailAddress email( QUrl( "mailto:" + emails[i] ), graphUri );
    email.setEmailAddress( emails[ i ] );
    contact.addEmailAddress( email );
  }

  // addresses
  const KABC::Address::List addresses = addressee.addresses();
  for ( int i = 0; i < addresses.count(); ++i ) {
    NepomukFast::PostalAddress address( QUrl(), graphUri );
    address.setStreetAddresses( listFromString( addresses[ i ].street() ) );
    if ( !addresses[ i ].postalCode().isEmpty() )
      address.setPostalcode( addresses[ i ].postalCode() );
    if ( !addresses[ i ].locality().isEmpty() )
      address.setLocality( addresses[ i ].locality() );
    if ( !addresses[ i ].region().isEmpty() )
      address.setRegion( addresses[ i ].region() );
    if ( !addresses[ i ].postOfficeBox().isEmpty() )
      address.addPobox( addresses[ i ].postOfficeBox() );
    if ( !addresses[ i ].country().isEmpty() )
      address.setCountry( addresses[ i ].country() );
    address.setExtendedAddresses( listFromString( addresses[ i ].extended() ) );

    contact.addPostalAddress( address );
  }
}

void NepomukContactFeeder::updateGroupItem( const Akonadi::Item &item, const QUrl &graphUri )
{
  // create the contact group with the graph reference
  NepomukFast::ContactGroup group( item.url(), graphUri );

  const KABC::ContactGroup contactGroup = item.payload<KABC::ContactGroup>();

  group.setContactGroupName( contactGroup.name() );

  for ( uint i = 0; i < contactGroup.contactReferenceCount(); ++i ) {
    const Akonadi::Item contactItem( contactGroup.contactReference( i ).uid().toLongLong() );

    NepomukFast::PersonContact person( contactItem.url(), graphUri );
    person.addBelongsToGroup( group );
  }
}

} // namespace Akonadi

AKONADI_AGENT_MAIN( Akonadi::NepomukContactFeeder )

#include "nepomukcontactfeeder.moc"
