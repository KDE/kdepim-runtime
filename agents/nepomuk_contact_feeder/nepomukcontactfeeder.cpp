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

#include <akonadi/changerecorder.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/collectionfetchjob.h>

#include <nepomuk/resource.h>
#include <nepomuk/resourcemanager.h>
#include <nepomuk/variant.h>
#include <kurl.h>

#include <Soprano/Model>

// ontology includes
#include "bbsnumber.h"
#include "carphonenumber.h"
#include "cellphonenumber.h"
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

#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>

#include <KDebug>


namespace Akonadi {

NepomukContactFeeder::NepomukContactFeeder( const QString &id )
  : AgentBase( id ),
    mForceUpdate( false )
{
  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->setMimeTypeMonitored( "text/directory" );
  changeRecorder()->setChangeRecordingEnabled( false );

  // do the initial scan to make sure all items have been fed to nepomuk
  QTimer::singleShot( 0, this, SLOT( slotInitialItemScan() ) );

  // The line below is not necessary anymore once AgentBase also exports scriptable slots
  QDBusConnection::sessionBus().registerObject( "/nepomukcontactfeeder", this, QDBusConnection::ExportScriptableSlots );
}

void NepomukContactFeeder::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
{
  itemChanged( item, QSet<QByteArray>() );
}

void NepomukContactFeeder::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  if ( item.hasPayload<KABC::Addressee>() )
    updateItem( item );
}

void NepomukContactFeeder::itemRemoved( const Akonadi::Item &item )
{
  Nepomuk::PersonContact contact( item.url() );

  QList<Nepomuk::PhoneNumber> numbers = contact.phoneNumbers();
  Q_FOREACH( Nepomuk::PhoneNumber number, numbers ) {
    number.remove();
  }

  QList<Nepomuk::EmailAddress> emails = contact.emailAddresses();
  Q_FOREACH( Nepomuk::EmailAddress email, emails ) {
    email.remove();
  }

  QList<Nepomuk::PostalAddress> addresses = contact.postalAddresses();
  Q_FOREACH( Nepomuk::PostalAddress address, addresses ) {
    address.remove();
  }

  contact.remove();
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

    foreach( const Collection &collection, collections) {
      kDebug() << "checking collection" << collection.name();
      if ( collection.contentMimeTypes().contains( QLatin1String( "text/directory" ) ) ) {
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

void NepomukContactFeeder::updateItem( const Akonadi::Item &item )
{
  const KABC::Addressee addressee = item.payload<KABC::Addressee>();

  Nepomuk::PersonContact contact( item.url() );

  // Nepomuk has no inference yet, thus having a label makes things easier
  // when displaying resources generically, since it will be returned by
  // Nepomuk::Resource::genericLabel()
  contact.setLabel( addressee.name() );

  contact.setNameGivens( QStringList( addressee.givenName() ) );
  contact.setNameAdditionals( QStringList( addressee.additionalName() ) );
  contact.setNameFamilys( QStringList( addressee.familyName() ) );
  contact.setNameHonorificPrefixs( QStringList( addressee.prefix() ) );
  contact.setNameHonorificSuffixs( QStringList( addressee.suffix() ) );
  contact.setLocations( QStringList( addressee.geo().toString() ) ); // make it better
  // keys
  // sounds
  // logos
  // photos
  contact.setNotes( QStringList( addressee.note() ) );
  contact.setNicknames( QStringList( addressee.nickName() ) );
  contact.setContactUIDs( QStringList( addressee.uid() ) );
  contact.setFullnames( QStringList( addressee.name() ) );
  contact.setBirthDate( addressee.birthday().date() );

  // phone numbers

  // first clean all
  contact.setPhoneNumbers( QList<Nepomuk::PhoneNumber>() );

  // add all existing
  const KABC::PhoneNumber::List phoneNumbers = addressee.phoneNumbers();
  for ( int i = 0; i < phoneNumbers.count(); ++i ) {
    if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Bbs ) {
      Nepomuk::BbsNumber number;
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Car ) {
      Nepomuk::CarPhoneNumber number;
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Cell ) {
      Nepomuk::CellPhoneNumber number;
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Fax ) {
      Nepomuk::FaxNumber number;
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Isdn ) {
      Nepomuk::IsdnNumber number;
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Msg ) {
      Nepomuk::MessagingNumber number;
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Modem ) {
      Nepomuk::ModemNumber number;
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Pager ) {
      Nepomuk::PagerNumber number;
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Pcs ) {
      Nepomuk::PcsNumber number;
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Video ) {
      Nepomuk::VideoTelephoneNumber number;
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Voice ) {
      Nepomuk::VoicePhoneNumber number;
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    } else { // matches Home and Work
      Nepomuk::PhoneNumber number;
      number.setPhoneNumbers( QStringList( phoneNumbers[ i ].number() ) );
      contact.addPhoneNumber( number );
    }
  }

  // im accounts

  // email addresses
  contact.setEmailAddresses( QList<Nepomuk::EmailAddress>() );

  const QStringList emails = addressee.emails();
  for ( int i = 0; i < emails.count(); ++i ) {
    Nepomuk::EmailAddress email;
    email.setEmailAddress( emails[ i ] );
    contact.addEmailAddress( email );
  }

  // addresses
  contact.setPostalAddresses( QList<Nepomuk::PostalAddress>() );

  const KABC::Address::List addresses = addressee.addresses();
  for ( int i = 0; i < addresses.count(); ++i ) {
    Nepomuk::PostalAddress address;
    address.addStreetAddress( addresses[ i ].street() );
    address.setPostalcode( addresses[ i ].postalCode() );
    address.setLocality( addresses[ i ].locality() );
    address.setRegion( addresses[ i ].region() );
    address.addPobox( addresses[ i ].postOfficeBox() );
    address.setCountry( addresses[ i ].country() );
    address.addExtendedAddress(  addresses[ i ].extended() );

    contact.addPostalAddress( address );
  }
}

} // namespace Akonadi

AKONADI_AGENT_MAIN( Akonadi::NepomukContactFeeder )

#include "nepomukcontactfeeder.moc"
