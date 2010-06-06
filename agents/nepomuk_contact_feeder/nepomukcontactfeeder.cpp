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
#include <akonadi/itemfetchscope.h>

#include <nepomuk/resource.h>
#include <nepomuk/resourcemanager.h>
#include <nepomuk/variant.h>
#include <kurl.h>
#include <KLocale>

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

#include <KDebug>

namespace Akonadi {

NepomukContactFeeder::NepomukContactFeeder( const QString &id )
  : NepomukFeederAgent<NepomukFast::ContactList>( id )
{
  KGlobal::locale()->insertCatalog( "akonadi_nepomukfeeder" );
  addSupportedMimeType( KABC::Addressee::mimeType() );
  addSupportedMimeType( KABC::ContactGroup::mimeType() );

  changeRecorder()->itemFetchScope().fetchFullPayload();
}

NepomukContactFeeder::~NepomukContactFeeder()
{
}

namespace {
  inline QStringList listFromString( const QString& s ) {
    if ( s.isEmpty() )
      return QStringList();
    else
      return QStringList( s );
  }
}

void NepomukContactFeeder::updateItem( const Akonadi::Item &item, const QUrl &graphUri )
{
  if ( item.hasPayload<KABC::Addressee>() )
    updateContactItem( item, graphUri );
  else if ( item.hasPayload<KABC::ContactGroup>() )
    updateGroupItem( item, graphUri );
  else
    kDebug() << "Got item without known payload. Mimetype:" << item.mimeType()
             << "Id:" << item.id();
}

void NepomukContactFeeder::updateContactItem( const Akonadi::Item &item, const QUrl &graphUri )
{
  // create the contact with the graph reference
  NepomukFast::PersonContact contact( item.url(), graphUri );
  contact.addProperty( Soprano::Vocabulary::NAO::hasSymbol(), Soprano::LiteralValue( "view-pim-contacts" ) );
  setParent( contact, item );

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
//    contact.setLocation( geoString ); // make it better
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
    contact.setFullname( addressee.name() );

  if ( addressee.birthday().date().isValid() )
    contact.setBirthDate( addressee.birthday().date() );

  if ( addressee.url().isValid() ) {
    KUrl url = addressee.url();

    // Nepomuk doesn't like URLs without a protocol
    if ( url.protocol().isEmpty() )
      url.setProtocol( "http" );

    NepomukFast::Website website( url, graphUri );
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

  tagsFromCategories( contact, addressee.categories() );
}

void NepomukContactFeeder::updateGroupItem( const Akonadi::Item &item, const QUrl &graphUri )
{
  // create the contact group with the graph reference
  NepomukFast::ContactGroup group( item.url(), graphUri );
  setParent( group, item );

  const KABC::ContactGroup contactGroup = item.payload<KABC::ContactGroup>();

  group.setContactGroupName( contactGroup.name() );
  group.setLabel( contactGroup.name() );

  for ( uint i = 0; i < contactGroup.contactReferenceCount(); ++i ) {
    const Akonadi::Item contactItem( contactGroup.contactReference( i ).uid().toLongLong() );

    NepomukFast::PersonContact person( contactItem.url(), graphUri );
    person.addBelongsToGroup( group );
  }
}

} // namespace Akonadi

AKONADI_AGENT_MAIN( Akonadi::NepomukContactFeeder )

#include "nepomukcontactfeeder.moc"
