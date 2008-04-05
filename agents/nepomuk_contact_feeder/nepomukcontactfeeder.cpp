/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#include <nepomuk/resource.h>
#include <nepomuk/variant.h>
#include <kurl.h>

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

using namespace Akonadi;

NepomukContactFeeder::NepomukContactFeeder( const QString &id )
  : AgentBase( id )
{
  changeRecorder()->itemFetchScope().setFetchAllParts( true );
  changeRecorder()->setMimeTypeMonitored( "text/directory" );
  changeRecorder()->setChangeRecordingEnabled( false );
}

void NepomukContactFeeder::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
{
  itemChanged( item, QSet<QByteArray>() );
}

void NepomukContactFeeder::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  if ( !item.hasPayload<KABC::Addressee>() )
    return;

  const KABC::Addressee addressee = item.payload<KABC::Addressee>();

  Nepomuk::PersonContact contact( item.url() );

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
  contact.setEmailAddresss( QList<Nepomuk::EmailAddress>() );

  const QStringList emails = addressee.emails();
  for ( int i = 0; i < emails.count(); ++i ) {
    Nepomuk::EmailAddress email;
    email.setEmailAddress( emails[ i ] );
    contact.addEmailAddress( email );
  }

  // addresses
  contact.setPostalAddresss( QList<Nepomuk::PostalAddress>() );

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

void NepomukContactFeeder::itemRemoved( const Akonadi::Item &item )
{
  Nepomuk::PersonContact contact( item.url() );

  QList<Nepomuk::PhoneNumber> numbers = contact.phoneNumbers();
  Q_FOREACH( Nepomuk::PhoneNumber number, numbers ) {
    number.remove();
  }

  QList<Nepomuk::EmailAddress> emails = contact.emailAddresss();
  Q_FOREACH( Nepomuk::EmailAddress email, emails ) {
    email.remove();
  }

  QList<Nepomuk::PostalAddress> addresses = contact.postalAddresss();
  Q_FOREACH( Nepomuk::PostalAddress address, addresses ) {
    address.remove();
  }

  contact.remove();
}

AKONADI_AGENT_MAIN( NepomukContactFeeder )

#include "nepomukcontactfeeder.moc"
