/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "contactutils.h"

#include "davutils.h"
#include "oxutils.h"

#include <QtCore/QBuffer>
#include <QtXml/QDomElement>

using namespace OXA;

void OXA::ContactUtils::parseContact( const QDomElement &propElement, Object &object )
{
  KABC::Addressee contact;
  KABC::Address homeAddress( KABC::Address::Home );
  KABC::Address workAddress( KABC::Address::Work );
  KABC::Address otherAddress( KABC::Address::Dom );

  QDomElement element = propElement.firstChildElement();
  while ( !element.isNull() ) {
    const QString tagName = element.tagName();
    const QString text = OXUtils::readString( element.text() );

    // name
    if ( tagName == QLatin1String( "title" ) ) {
      contact.setTitle( text );
    } else if ( tagName == QLatin1String( "first_name" ) ) {
      contact.setGivenName( text );
    } else if ( tagName == QLatin1String( "second_name" ) ) {
      contact.setAdditionalName( text );
    } else if ( tagName == QLatin1String( "last_name" ) ) {
      contact.setFamilyName( text );
    } else if ( tagName == QLatin1String( "suffix" ) ) {
      contact.setSuffix( text );
    } else if ( tagName == QLatin1String( "displayname" ) ) {
      contact.setFormattedName( text );
    } else if ( tagName == QLatin1String( "nickname" ) ) {
      contact.setNickName( text );
    // dates
    } else if ( tagName == QLatin1String( "birthday" ) ) {
      contact.setBirthday( OXUtils::readDateTime( element.text() ).dateTime() );
    } else if ( tagName == QLatin1String( "anniversary" ) ) {
      const QDateTime dateTime = OXUtils::readDateTime( element.text() ).dateTime();
      contact.insertCustom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-Anniversary" ), dateTime.toString( Qt::ISODate ) );
    } else if ( tagName == QLatin1String( "spouse_name" ) ) {
      contact.insertCustom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-SpousesName" ), text );
    // addresses
    } else if ( tagName == QLatin1String( "street" ) ) {
      homeAddress.setStreet( text );
    } else if ( tagName == QLatin1String( "postal_code" ) ) {
      homeAddress.setPostalCode( text );
    } else if ( tagName == QLatin1String( "city" ) ) {
      homeAddress.setLocality( text );
    } else if ( tagName == QLatin1String( "country" ) ) {
      homeAddress.setCountry( text );
    } else if ( tagName == QLatin1String( "state" ) ) {
      homeAddress.setRegion( text );
    } else if ( tagName == QLatin1String( "business_street" ) ) {
      workAddress.setStreet( text );
    } else if ( tagName == QLatin1String( "business_postal_code" ) ) {
      workAddress.setPostalCode( text );
    } else if ( tagName == QLatin1String( "business_city" ) ) {
      workAddress.setLocality( text );
    } else if ( tagName == QLatin1String( "business_country" ) ) {
      workAddress.setCountry( text );
    } else if ( tagName == QLatin1String( "business_state" ) ) {
      workAddress.setRegion( text );
    } else if ( tagName == QLatin1String( "second_street" ) ) {
      otherAddress.setStreet( text );
    } else if ( tagName == QLatin1String( "second_postal_code" ) ) {
      otherAddress.setPostalCode( text );
    } else if ( tagName == QLatin1String( "second_city" ) ) {
      otherAddress.setLocality( text );
    } else if ( tagName == QLatin1String( "second_country" ) ) {
      otherAddress.setCountry( text );
    } else if ( tagName == QLatin1String( "second_state" ) ) {
      otherAddress.setRegion( text );
    } else if ( tagName == QLatin1String( "defaultaddress" ) ) {
      const int number = text.toInt();
      if ( number == 1 )
        workAddress.setType( workAddress.type() | KABC::Address::Pref );
      else if ( number == 2 )
        homeAddress.setType( homeAddress.type() | KABC::Address::Pref );
      else if ( number == 3 )
        otherAddress.setType( otherAddress.type() | KABC::Address::Pref );
    // further information
    } else if ( tagName == QLatin1String( "note" ) ) {
      contact.setNote( text );
    } else if ( tagName == QLatin1String( "url" ) ) {
      contact.setUrl( text );
    } else if ( tagName == QLatin1String( "image1" ) ) {
      const QByteArray data = text.toUtf8();
      contact.setPhoto( KABC::Picture( QImage::fromData( QByteArray::fromBase64( data ) ) ) );
    // company information
    } else if ( tagName == QLatin1String( "company" ) ) {
      contact.setOrganization( text );
    } else if ( tagName == QLatin1String( "department" ) ) {
      contact.setDepartment( text );
    } else if ( tagName == QLatin1String( "assistants_name" ) ) {
      contact.insertCustom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-AssistantsName" ), text );
    } else if ( tagName == QLatin1String( "managers_name" ) ) {
      contact.insertCustom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-ManagersName" ), text );
    } else if ( tagName == QLatin1String( "position" ) ) {
      contact.setRole( text );
    } else if ( tagName == QLatin1String( "profession" ) ) {
      contact.insertCustom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-Profession" ), text );
    } else if ( tagName == QLatin1String( "room_number" ) ) {
      contact.insertCustom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-Office" ), text );
    // communication
    } else if ( tagName == QLatin1String( "email1" ) ) {
      contact.insertEmail( text, true );
    } else if ( tagName == QLatin1String( "email2" ) ||
                tagName == QLatin1String( "email3" ) ) {
      contact.insertEmail( text );
    } else if ( tagName == QLatin1String( "mobile1" ) ) {
      contact.insertPhoneNumber( KABC::PhoneNumber( text, KABC::PhoneNumber::Cell ) );
    } else if ( tagName == QLatin1String( "instant_messenger" ) ) {
      contact.insertCustom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-IMAddress" ), text );
    } else if ( tagName.startsWith( QLatin1String( "phone_" ) ) ) {
      KABC::PhoneNumber number;
      number.setNumber( text );
      bool supportedType = false;

      if ( tagName.endsWith( QLatin1String( "_business" ) ) ) {
        number.setType( KABC::PhoneNumber::Work );
        supportedType = true;
      } else if ( tagName.endsWith( QLatin1String( "_home" ) ) ) {
        number.setType( KABC::PhoneNumber::Home );
        supportedType = true;
      } else if ( tagName.endsWith( QLatin1String( "_other" ) ) ) {
        number.setType( KABC::PhoneNumber::Voice );
        supportedType = true;
      } else if ( tagName.endsWith( QLatin1String( "_car" ) ) ) {
        number.setType( KABC::PhoneNumber::Car );
        supportedType = true;
      }

      if ( supportedType )
        contact.insertPhoneNumber( number );
    } else if ( tagName.startsWith( QLatin1String( "fax_" ) ) ) {
      KABC::PhoneNumber number;
      number.setNumber( text );
      bool supportedType = false;

      if ( tagName.endsWith( QLatin1String( "_business" ) ) ) {
        number.setType( KABC::PhoneNumber::Fax | KABC::PhoneNumber::Work );
        supportedType = true;
      } else if ( tagName.endsWith( QLatin1String( "_home" ) ) ) {
        number.setType( KABC::PhoneNumber::Fax | KABC::PhoneNumber::Home );
        supportedType = true;
      } else if ( tagName.endsWith( QLatin1String( "_other" ) ) ) {
        number.setType( KABC::PhoneNumber::Fax | KABC::PhoneNumber::Voice );
        supportedType = true;
      }

      if ( supportedType )
        contact.insertPhoneNumber( number );
    } else if ( tagName == QLatin1String( "pager" ) ) {
      contact.insertPhoneNumber( KABC::PhoneNumber( text, KABC::PhoneNumber::Pager ) );
    } else if ( tagName == QLatin1String( "categories" ) ) {
      contact.setCategories( text.split( QRegExp( QLatin1String( ",\\s*" ) ) ) );
    }

    element = element.nextSiblingElement();
  }

  if ( !homeAddress.isEmpty() )
    contact.insertAddress( homeAddress );
  if ( !workAddress.isEmpty() )
    contact.insertAddress( workAddress );
  if ( !otherAddress.isEmpty() )
    contact.insertAddress( otherAddress );

  object.setContact( contact );
}

void OXA::ContactUtils::addContactElements( QDomDocument &document, QDomElement &propElement, const Object &object )
{
  const KABC::Addressee contact = object.contact();

  // name
  DAVUtils::addOxElement( document, propElement, QLatin1String( "title" ), OXUtils::writeString( contact.title() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "first_name" ), OXUtils::writeString( contact.givenName() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "second_name" ), OXUtils::writeString( contact.additionalName() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "last_name" ), OXUtils::writeString( contact.familyName() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "suffix" ), OXUtils::writeString( contact.suffix() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "displayname" ), OXUtils::writeString( contact.formattedName() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "nickname" ), OXUtils::writeString( contact.nickName() ) );

  // dates
  const QDateTime birthday( contact.birthday().date(), QTime(), Qt::UTC );
  if ( birthday.isValid() )
    DAVUtils::addOxElement( document, propElement, QLatin1String( "birthday" ), OXUtils::writeDateTime( KDateTime( birthday ) ) );
  else
    DAVUtils::addOxElement( document, propElement, QLatin1String( "birthday" ) );

  // since QDateTime::to/fromString() doesn't carry timezone information, we have to fake it here
  const QDate date = QDate::fromString( contact.custom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-Anniversary" ) ), Qt::ISODate );
  const QDateTime anniversary( date, QTime(), Qt::UTC );
  if ( anniversary.isValid() )
    DAVUtils::addOxElement( document, propElement, QLatin1String( "anniversary" ), OXUtils::writeDateTime( KDateTime( anniversary ) ) );
  else
    DAVUtils::addOxElement( document, propElement, QLatin1String( "anniversary" ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "spouse_name" ), OXUtils::writeString( contact.custom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-SpousesName" ) ) ) );

  // addresses
  const KABC::Address homeAddress = contact.address( KABC::Address::Home );
  if ( !homeAddress.isEmpty() ) {
    DAVUtils::addOxElement( document, propElement, QLatin1String( "street" ), OXUtils::writeString( homeAddress.street() ) );
    DAVUtils::addOxElement( document, propElement, QLatin1String( "postal_code" ), OXUtils::writeString( homeAddress.postalCode() ) );
    DAVUtils::addOxElement( document, propElement, QLatin1String( "city" ), OXUtils::writeString( homeAddress.locality() ) );
    DAVUtils::addOxElement( document, propElement, QLatin1String( "state" ), OXUtils::writeString( homeAddress.region() ) );
    DAVUtils::addOxElement( document, propElement, QLatin1String( "country" ), OXUtils::writeString( homeAddress.country() ) );
  }
  const KABC::Address workAddress = contact.address( KABC::Address::Work );
  if ( !workAddress.isEmpty() ) {
    DAVUtils::addOxElement( document, propElement, QLatin1String( "business_street" ), OXUtils::writeString( workAddress.street() ) );
    DAVUtils::addOxElement( document, propElement, QLatin1String( "business_postal_code" ), OXUtils::writeString( workAddress.postalCode() ) );
    DAVUtils::addOxElement( document, propElement, QLatin1String( "business_city" ), OXUtils::writeString( workAddress.locality() ) );
    DAVUtils::addOxElement( document, propElement, QLatin1String( "business_state" ), OXUtils::writeString( workAddress.region() ) );
    DAVUtils::addOxElement( document, propElement, QLatin1String( "business_country" ), OXUtils::writeString( workAddress.country() ) );
  }
  const KABC::Address otherAddress = contact.address( KABC::Address::Dom );
  if ( !otherAddress.isEmpty() ) {
    DAVUtils::addOxElement( document, propElement, QLatin1String( "second_street" ), OXUtils::writeString( otherAddress.street() ) );
    DAVUtils::addOxElement( document, propElement, QLatin1String( "second_postal_code" ), OXUtils::writeString( otherAddress.postalCode() ) );
    DAVUtils::addOxElement( document, propElement, QLatin1String( "second_city" ), OXUtils::writeString( otherAddress.locality() ) );
    DAVUtils::addOxElement( document, propElement, QLatin1String( "second_state" ), OXUtils::writeString( otherAddress.region() ) );
    DAVUtils::addOxElement( document, propElement, QLatin1String( "second_country" ), OXUtils::writeString( otherAddress.country() ) );
  }

  // further information
  DAVUtils::addOxElement( document, propElement, QLatin1String( "note" ), OXUtils::writeString( contact.note() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "url" ), OXUtils::writeString( contact.url().url() ) );

  // image
  const KABC::Picture photo = contact.photo();
  if ( !photo.data().isNull() ) {
    QByteArray imageData;
    QBuffer buffer( &imageData );
    buffer.open( QIODevice::WriteOnly );

    QString contentType;
    if ( !photo.data().hasAlphaChannel() ) {
      photo.data().save( &buffer, "JPEG" );
      contentType = QLatin1String( "image/jpg" );
    } else {
      photo.data().save( &buffer, "PNG" );
      contentType = QLatin1String( "image/png" );
    }

    buffer.close();

    DAVUtils::addOxElement( document, propElement, QLatin1String( "image1" ), QString::fromLatin1( imageData.toBase64() ) );
    DAVUtils::addOxElement( document, propElement, QLatin1String( "image_content_type" ), contentType );
  } else {
    DAVUtils::addOxElement( document, propElement, QLatin1String( "image1" ) );
  }

  // company information
  DAVUtils::addOxElement( document, propElement, QLatin1String( "company" ), OXUtils::writeString( contact.organization() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "department" ), OXUtils::writeString( contact.department() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "assistants_name" ), OXUtils::writeString( contact.custom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-AssistantsName" ) ) ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "managers_name" ), OXUtils::writeString( contact.custom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-ManagersName" ) ) ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "position" ), OXUtils::writeString( contact.role() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "profession" ), OXUtils::writeString( contact.custom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-Profession" ) ) ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "room_number" ), OXUtils::writeString( contact.custom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-Office" ) ) ) );

  // communication
  const QStringList emails = contact.emails();
  for ( int i = 0; i < 3 && i < emails.count(); ++i ) {
    DAVUtils::addOxElement( document, propElement, QString::fromLatin1( "email%1" ).arg( i + 1 ), OXUtils::writeString( emails.at( i ) ) );
  }

  DAVUtils::addOxElement( document, propElement, QLatin1String( "mobile1" ), OXUtils::writeString( contact.phoneNumber( KABC::PhoneNumber::Cell ).number() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "instant_messenger" ), OXUtils::writeString( contact.custom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-IMAddress" ) ) ) );

  DAVUtils::addOxElement( document, propElement, QLatin1String( "phone_business" ), OXUtils::writeString( contact.phoneNumber( KABC::PhoneNumber::Work ).number() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "phone_home" ), OXUtils::writeString( contact.phoneNumber( KABC::PhoneNumber::Home ).number() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "phone_other" ), OXUtils::writeString( contact.phoneNumber( KABC::PhoneNumber::Voice ).number() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "phone_car" ), OXUtils::writeString( contact.phoneNumber( KABC::PhoneNumber::Car ).number() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "fax_business" ), OXUtils::writeString( contact.phoneNumber( KABC::PhoneNumber::Fax | KABC::PhoneNumber::Work ).number() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "fax_home" ), OXUtils::writeString( contact.phoneNumber( KABC::PhoneNumber::Fax | KABC::PhoneNumber::Home ).number() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "fax_other" ), OXUtils::writeString( contact.phoneNumber( KABC::PhoneNumber::Fax | KABC::PhoneNumber::Voice ).number() ) );

  DAVUtils::addOxElement( document, propElement, QLatin1String( "pager" ), OXUtils::writeString( contact.phoneNumber( KABC::PhoneNumber::Pager ).number() ) );

  DAVUtils::addOxElement( document, propElement, QLatin1String( "categories" ), OXUtils::writeString( contact.categories().join( QLatin1String( "," ) ) ) );
}
