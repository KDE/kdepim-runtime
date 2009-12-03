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

#include "objectutils.h"

#include "oxutils.h"

#include <kcal/event.h>
#include <kcal/todo.h>

using namespace OXA;

/*
  {"categories", "categories"},
  // incidence fields
  {"title", "title"},
  {"description", "note"},
  {"members", "participants"},
  {"member", "user"},
  {"reminder", "alarm"},
  // recurrence fields
  {"date_sequence", "recurrence_type"},
  {"ds_ends", "until"},
  {"daily_value", "interval"},
  {"weekly_value", "interval"},
  {"monthly_value_month", "interval"},
  {"monthly_value_day", "day_in_month"},
  {"yearly_value_day", "day_in_month"},
  {"yearly_month", "month"},
  {"monthly2_value_month", "interval"},
  {"monthly2_day", "days"},
  {"monthly2_recurrency", "day_in_month"},
  {"yearly2_day", "days"},
  {"yearly2_reccurency", "day_in_month"}, // this is not a typo, this is what SLOX erally sends!
  {"yearly2_month", "month"},
  {"deleteexceptions", "deleteexceptions"},
  // event fields
  {"begins", "start_date"},
  {"ends", "end_date"},
  {"location", "location"},
  {"full_time", "full_time"},
  // task fields
  {"startdate", "start_date"},
  {"deadline", "end_date"},
  {"priority", "priority"},
  {"status", "percent_complete"},
};
*/
static void parseContact( const QDomElement &propElement, Object &object )
{
  KABC::Addressee contact;

  QDomElement element = propElement.firstChildElement();
  while ( !element.isNull() ) {
    const QString tagName = element.tagName();
    const QString text = OXUtils::readString( element.text() );

    if ( tagName == QLatin1String( "birthday" ) ) {
      contact.setBirthday( OXUtils::readDateTime( element.text() ).dateTime() );
    } else if ( tagName == QLatin1String( "position" ) ) {
      contact.setRole( text );
    } else if ( tagName == QLatin1String( "title" ) ) {
      contact.setTitle( text );
    } else if ( tagName == QLatin1String( "company" ) ) {
      contact.setOrganization( text );
    } else if ( tagName == QLatin1String( "department" ) ) {
      contact.setDepartment( text );
    } else if ( tagName == QLatin1String( "last_name" ) ) {
      contact.setFamilyName( text );
    } else if ( tagName == QLatin1String( "first_name" ) ) {
      contact.setGivenName( text );
    } else if ( tagName == QLatin1String( "second_name" ) ) {
      contact.setAdditionalName( text );
    } else if ( tagName == QLatin1String( "display_name" ) ) {
      contact.setFormattedName( text );
    } else if ( tagName == QLatin1String( "suffix" ) ) {
      contact.setSuffix( text );
    } else if ( tagName == QLatin1String( "note" ) ) {
      contact.setNote( text );
    } else if ( tagName == QLatin1String( "email1" ) ) {
      contact.insertEmail( text, true );
    } else if ( tagName == QLatin1String( "email2" ) ||
                tagName == QLatin1String( "email3" ) ) {
      contact.insertEmail( text );
    } else if ( tagName == QLatin1String( "url" ) ) {
      contact.setUrl( text );
    } else if ( tagName.startsWith( QLatin1String( "telephone_" ) ) ) {
      KABC::PhoneNumber number;
      number.setNumber( text );
      if ( tagName.endsWith( QLatin1String( "_business1" ) ) ) {
        number.setType( KABC::PhoneNumber::Work );
      } else if ( tagName.endsWith( QLatin1String( "_home1" ) ) ) {
        number.setType( KABC::PhoneNumber::Home );
      } else if ( tagName.endsWith( QLatin1String( "_pager" ) ) ) {
        number.setType( KABC::PhoneNumber::Pager );
      } else if ( tagName.endsWith( QLatin1String( "_car" ) ) ) {
        number.setType( KABC::PhoneNumber::Car );
      } else if ( tagName.endsWith( QLatin1String( "_isdn" ) ) ) {
        number.setType( KABC::PhoneNumber::Isdn );
      } else if ( tagName.endsWith( QLatin1String( "_primary" ) ) ) {
        number.setType( KABC::PhoneNumber::Pref );
      } else if ( tagName.endsWith( QLatin1String( "_telex" ) ) ) {
        number.setType( KABC::PhoneNumber::Msg );
      }
      contact.insertPhoneNumber( number );
    } else if ( tagName.startsWith( QLatin1String( "fax_" ) ) ) {
      KABC::PhoneNumber number;
      number.setNumber( text );
      if ( tagName.endsWith( QLatin1String( "_business" ) ) ) {
        number.setType( KABC::PhoneNumber::Fax | KABC::PhoneNumber::Work );
      } else if ( tagName.endsWith( QLatin1String( "_home" ) ) ) {
        number.setType( KABC::PhoneNumber::Fax | KABC::PhoneNumber::Home );
      }
      contact.insertPhoneNumber( number );
    } else if ( tagName == QLatin1String( "image1" ) ) {
      const QByteArray data = text.toUtf8();
      contact.setPhoto( KABC::Picture( QImage::fromData( QByteArray::fromBase64( data ) ) ) );
    } else if ( tagName == QLatin1String( "nickname" ) ) {
      contact.setNickName( text );
    } else if ( tagName == QLatin1String( "instant_messenger1" ) ) {
      contact.insertCustom( "KADDRESSBOOK", "X-IMAddress", text );
    } else if ( tagName == QLatin1String( "room_number" ) ) {
      contact.insertCustom( "KADDRESSBOOK", "X-Office", text );
    } else if ( tagName == QLatin1String( "profession" ) ) {
      contact.insertCustom( "KADDRESSBOOK", "X-Profession", text );
    } else if ( tagName == QLatin1String( "manager_name" ) ) {
      contact.insertCustom( "KADDRESSBOOK", "X-ManagersName", text );
    } else if ( tagName == QLatin1String( "assistant_name" ) ) {
      contact.insertCustom( "KADDRESSBOOK", "X-AssistantsName", text );
    } else if ( tagName == QLatin1String( "spouse_name" ) ) {
      contact.insertCustom( "KADDRESSBOOK", "X-SpousesName", text );
    } else if ( tagName == QLatin1String( "anniversary" ) ) {
      const QDateTime dateTime = OXUtils::readDateTime( element.text() ).dateTime();
      contact.insertCustom( "KADDRESSBOOK", "X-Anniversary", dateTime.toString( Qt::ISODate ) );
    } else if ( tagName == QLatin1String( "categories" ) ) {
      contact.setCategories( text.split( QRegExp(",\\s*") ) );
    } else {
    /*
      // read addresses
      Address addr;
      if ( tagName.startsWith( fieldName( BusinessPrefix ) ) ) {
        addr = a.address( KABC::Address::Work );
      } else if ( tagName.startsWith( fieldName( OtherPrefix ) ) ) {
        addr = a.address( 0 );
      } else {
        addr = a.address( KABC::Address::Home );
      }
      if ( tagName.endsWith( fieldName( Street ) ) ) {
        addr.setStreet( text );
      } else if ( tagName.endsWith( fieldName( PostalCode ) ) ) {
        addr.setPostalCode( text );
      } else if ( tagName.endsWith( fieldName( City ) ) ) {
        addr.setLocality( text );
      } else if ( tagName.endsWith( fieldName( State ) ) ) {
        addr.setRegion( text );
      } else if ( tagName.endsWith( fieldName( Country ) ) ) {
        addr.setCountry( text );
      }
      contact.insertAddress( addr );
     */
    }

    element = element.nextSiblingElement();
  }

  object.setContact( contact );
}

static void parseEvent( const QDomElement &propElement, Object &object )
{
  KCal::Event *event = new KCal::Event;

  QDomElement element = propElement.firstChildElement();
  while ( !element.isNull() ) {

    element = element.nextSiblingElement();
  }

  object.setEvent( KCal::Incidence::Ptr( event ) );
}

static void parseTask( const QDomElement &propElement, Object &object )
{
  KCal::Todo *todo = new KCal::Todo;

  QDomElement element = propElement.firstChildElement();
  while ( !element.isNull() ) {

    element = element.nextSiblingElement();
  }

  object.setTask( KCal::Incidence::Ptr( todo ) );
}

Object OXA::ObjectUtils::parseObject( const QDomElement &propElement, Object::Type type )
{
  Object object;

  QDomElement element = propElement.firstChildElement();
  while ( !element.isNull() ) {
    if ( element.tagName() == QLatin1String( "last_modified" ) ) {
      object.setLastModified( OXUtils::readString( element.text() ) );
    } else if ( element.tagName() == QLatin1String( "object_id" ) ) {
      object.setObjectId( OXUtils::readNumber( element.text() ) );
    } else if ( element.tagName() == QLatin1String( "folder_id" ) ) {
      object.setFolderId( OXUtils::readNumber( element.text() ) );
    }

    element = element.nextSiblingElement();
  }

  switch ( type ) {
    case Object::Contact: parseContact( propElement, object ); break;
    case Object::Event: parseEvent( propElement, object ); break;
    case Object::Task: parseTask( propElement, object ); break;
  }

  return object;
}

