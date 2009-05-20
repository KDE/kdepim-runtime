/*
    This file is part of libkabc and/or kaddressbook.
    Copyright (c) 2004 Klarälvdalens Datakonsult AB
        <info@klaralvdalens-datakonsult.se>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "contact.h"
// #include "resourcekolab.h"

#include <kabc/addressee.h>
#include <kabc/addressbook.h>
#include <libkdepim/distributionlist.h>
#include <kio/netaccess.h>
#include <kdebug.h>
#include <QFile>
#include <float.h>

using namespace Kolab;

static const char* s_pictureAttachmentName = "kolab-picture.png";
static const char* s_logoAttachmentName = "kolab-logo.png";
static const char* s_soundAttachmentName = "sound";
static const char* s_unhandledTagAppName = "KOLABUNHANDLED"; // no hyphens in appnames!

// saving (addressee->xml)
Contact::Contact( const KABC::Addressee* addr, KABC::AddressBook* addressBook )
  : mHasGeo( false )
{
  setFields( addr, addressBook );
}

// loading (xml->addressee)
Contact::Contact( const QString& xml )
  : mHasGeo( false )
{
  load( xml );
}

Contact::~Contact()
{
}

void Contact::setGivenName( const QString& name )
{
  mGivenName = name;
}

QString Contact::givenName() const
{
  return mGivenName;
}

void Contact::setMiddleNames( const QString& names )
{
  mMiddleNames = names;
}

QString Contact::middleNames() const
{
  return mMiddleNames;
}

void Contact::setLastName( const QString& name )
{
  mLastName = name;
}

QString Contact::lastName() const
{
  return mLastName;
}

void Contact::setFullName( const QString& name )
{
  mFullName = name;
}

QString Contact::fullName() const
{
  return mFullName;
}

void Contact::setInitials( const QString& initials )
{
  mInitials = initials;
}

QString Contact::initials() const
{
  return mInitials;
}

void Contact::setPrefix( const QString& prefix )
{
  mPrefix = prefix;
}

QString Contact::prefix() const
{
  return mPrefix;
}

void Contact::setSuffix( const QString& suffix )
{
  mSuffix = suffix;
}

QString Contact::suffix() const
{
  return mSuffix;
}

void Contact::setRole( const QString& role )
{
  mRole = role;
}

QString Contact::role() const
{
  return mRole;
}

void Contact::setFreeBusyUrl( const QString& fbUrl )
{
  mFreeBusyUrl = fbUrl;
}

QString Contact::freeBusyUrl() const
{
  return mFreeBusyUrl;
}

void Contact::setOrganization( const QString& organization )
{
  mOrganization = organization;
}

QString Contact::organization() const
{
  return mOrganization;
}

void Contact::setWebPage( const QString& url )
{
  mWebPage = url;
}

QString Contact::webPage() const
{
  return mWebPage;
}

void Contact::setIMAddress( const QString& imAddress )
{
  mIMAddress = imAddress;
}

QString Contact::imAddress() const
{
  return mIMAddress;
}

void Contact::setDepartment( const QString& department )
{
  mDepartment = department;
}

QString Contact::department() const
{
  return mDepartment;
}

void Contact::setOfficeLocation( const QString& location )
{
  mOfficeLocation = location;
}

QString Contact::officeLocation() const
{
  return mOfficeLocation;
}

void Contact::setProfession( const QString& profession )
{
  mProfession = profession;
}

QString Contact::profession() const
{
  return mProfession;
}

void Contact::setTitle( const QString& title )
{
  mTitle = title;
}

QString Contact::title() const
{
  return mTitle;
}

void Contact::setManagerName( const QString& name )
{
  mManagerName = name;
}

QString Contact::managerName() const
{
  return mManagerName;
}

void Contact::setAssistant( const QString& name )
{
  mAssistant = name;
}

QString Contact::assistant() const
{
  return mAssistant;
}

void Contact::setNickName( const QString& name )
{
  mNickName = name;
}

QString Contact::nickName() const
{
  return mNickName;
}

void Contact::setSpouseName( const QString& name )
{
  mSpouseName = name;
}

QString Contact::spouseName() const
{
  return mSpouseName;
}

void Contact::setBirthday( const QDate& date )
{
  mBirthday = date;
}

QDate Contact::birthday() const
{
  return mBirthday;
}

void Contact::setAnniversary( const QDate& date )
{
  mAnniversary = date;
}

QDate Contact::anniversary() const
{
  return mAnniversary;
}

void Contact::setChildren( const QString& children )
{
  mChildren = children;
}

QString Contact::children() const
{
  return mChildren;
}

void Contact::setGender( const QString& gender )
{
  mGender = gender;
}

QString Contact::gender() const
{
  return mGender;
}

void Contact::setLanguage( const QString& language )
{
  mLanguage = language;
}

QString Contact::language() const
{
  return mLanguage;
}

void Contact::addPhoneNumber( const PhoneNumber& number )
{
  mPhoneNumbers.append( number );
}

QList<Contact::PhoneNumber>& Contact::phoneNumbers()
{
  return mPhoneNumbers;
}

const QList<Contact::PhoneNumber>& Contact::phoneNumbers() const
{
  return mPhoneNumbers;
}

void Contact::addEmail( const Email& email )
{
  mEmails.append( email );
}

QList<Contact::Email>& Contact::emails()
{
  return mEmails;
}

const QList<Contact::Email>& Contact::emails() const
{
  return mEmails;
}

void Contact::addAddress( const Contact::Address& address )
{
  mAddresses.append( address );
}

QList<Contact::Address>& Contact::addresses()
{
  return mAddresses;
}

const QList<Contact::Address>& Contact::addresses() const
{
  return mAddresses;
}

void Contact::setPreferredAddress( const QString& address )
{
  mPreferredAddress = address;
}

QString Contact::preferredAddress() const
{
  return mPreferredAddress;
}

bool Contact::loadNameAttribute( QDomElement& element )
{
  for ( QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    if ( n.isComment() )
      continue;
    if ( n.isElement() ) {
      QDomElement e = n.toElement();
      QString tagName = e.tagName();

      if ( tagName == "given-name" )
        setGivenName( e.text() );
      else if ( tagName == "middle-names" )
        setMiddleNames( e.text() );
      else if ( tagName == "last-name" )
        setLastName( e.text() );
      else if ( tagName == "full-name" )
        setFullName( e.text() );
      else if ( tagName == "initials" )
        setInitials( e.text() );
      else if ( tagName == "prefix" )
        setPrefix( e.text() );
      else if ( tagName == "suffix" )
        setSuffix( e.text() );
      else
        // TODO: Unhandled tag - save for later storage
        kDebug() <<"Warning: Unhandled tag" << e.tagName();
    } else
      kDebug() <<"Node is not a comment or an element???";
  }

  return true;
}

void Contact::saveNameAttribute( QDomElement& element ) const
{
  QDomElement e = element.ownerDocument().createElement( "name" );
  element.appendChild( e );

  writeString( e, "given-name", givenName() );
  writeString( e, "middle-names", middleNames() );
  writeString( e, "last-name", lastName() );
  writeString( e, "full-name", fullName() );
  writeString( e, "initials", initials() );
  writeString( e, "prefix", prefix() );
  writeString( e, "suffix", suffix() );
}

bool Contact::loadPhoneAttribute( QDomElement& element )
{
  PhoneNumber number;
  for ( QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    if ( n.isComment() )
      continue;
    if ( n.isElement() ) {
      QDomElement e = n.toElement();
      QString tagName = e.tagName();

      if ( tagName == "type" )
        number.type = e.text();
      else if ( tagName == "number" )
        number.number = e.text();
      else
        // TODO: Unhandled tag - save for later storage
        kDebug() <<"Warning: Unhandled tag" << e.tagName();
    } else
      kDebug() <<"Node is not a comment or an element???";
  }

  addPhoneNumber( number );
  return true;
}

void Contact::savePhoneAttributes( QDomElement& element ) const
{
  QList<PhoneNumber>::ConstIterator it = mPhoneNumbers.constBegin();
  for ( ; it != mPhoneNumbers.constEnd(); ++it ) {
    QDomElement e = element.ownerDocument().createElement( "phone" );
    element.appendChild( e );
    const PhoneNumber& p = *it;
    writeString( e, "type", p.type );
    writeString( e, "number", p.number );
  }
}

void Contact::saveEmailAttributes( QDomElement& element ) const
{
  QList<Email>::ConstIterator it = mEmails.constBegin();
  for ( ; it != mEmails.constEnd(); ++it )
    saveEmailAttribute( element, *it );
}

void Contact::loadCustomAttributes( QDomElement& element )
{
  Custom custom;
  custom.app = element.attribute( "app" );
  custom.name = element.attribute( "name" );
  custom.value = element.attribute( "value" );
  mCustomList.append( custom );
}

void Contact::saveCustomAttributes( QDomElement& element ) const
{
  QList<Custom>::ConstIterator it = mCustomList.constBegin();
  for ( ; it != mCustomList.constEnd(); ++it ) {
    Q_ASSERT( !(*it).name.isEmpty() );
    if ( (*it).app == s_unhandledTagAppName ) {
      writeString( element, (*it).name, (*it).value );
    } else {
      // Let's use attributes so that other tag-preserving-code doesn't need sub-elements
      QDomElement e = element.ownerDocument().createElement( "x-custom" );
      element.appendChild( e );
      e.setAttribute( "app", (*it).app );
      e.setAttribute( "name", (*it).name );
      e.setAttribute( "value", (*it).value );
    }
  }
}

bool Contact::loadAddressAttribute( QDomElement& element )
{
  Address address;

  for ( QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    if ( n.isComment() )
      continue;
    if ( n.isElement() ) {
      QDomElement e = n.toElement();
      QString tagName = e.tagName();

      if ( tagName == "type" )
        address.type = e.text();
      else if ( tagName == "x-kde-type" )
        address.kdeAddressType = e.text().toInt();
      else if ( tagName == "street" )
        address.street = e.text();
      else if ( tagName == "pobox" )
        address.pobox = e.text();
      else if ( tagName == "locality" )
        address.locality = e.text();
      else if ( tagName == "region" )
        address.region = e.text();
      else if ( tagName == "postal-code" )
        address.postalCode = e.text();
      else if ( tagName == "country" )
        address.country = e.text();
      else
        // TODO: Unhandled tag - save for later storage
        kDebug() <<"Warning: Unhandled tag" << e.tagName();
    } else
      kDebug() <<"Node is not a comment or an element???";
  }

  addAddress( address );
  return true;
}

void Contact::saveAddressAttributes( QDomElement& element ) const
{
  QList<Address>::ConstIterator it = mAddresses.constBegin();
  for ( ; it != mAddresses.constEnd(); ++it ) {
    QDomElement e = element.ownerDocument().createElement( "address" );
    element.appendChild( e );
    const Address& a = *it;
    writeString( e, "type", a.type );
    writeString( e, "x-kde-type", QString::number( a.kdeAddressType ) );
    if ( !a.street.isEmpty() )
      writeString( e, "street", a.street );
    if ( !a.pobox.isEmpty() )
      writeString( e, "pobox", a.pobox );
    if ( !a.locality.isEmpty() )
    writeString( e, "locality", a.locality );
    if ( !a.region.isEmpty() )
      writeString( e, "region", a.region );
    if ( !a.postalCode.isEmpty() )
      writeString( e, "postal-code", a.postalCode );
    if ( !a.country.isEmpty() )
      writeString( e, "country", a.country );
  }
}


void Kolab::Contact::loadDistrListMember( const QDomElement& element )
{
  Member member;
  for ( QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    if ( n.isComment() )
      continue;
    if ( n.isElement() ) {
      QDomElement e = n.toElement();
      QString tagName = e.tagName();
      if ( tagName == "display-name" )
        member.displayName = e.text();
      else if ( tagName == "smtp-address" )
        member.email = e.text();
    }
  }
  mDistrListMembers.append( member );
}

void Contact::saveDistrListMembers( QDomElement& element ) const
{
  QList<Member>::ConstIterator it = mDistrListMembers.constBegin();
  for( ; it != mDistrListMembers.constEnd(); ++it ) {
    QDomElement e = element.ownerDocument().createElement( "member" );
    element.appendChild( e );
    const Member& m = *it;
    writeString( e, "display-name", m.displayName );
    writeString( e, "smtp-address", m.email );
  }
}

bool Contact::loadAttribute( QDomElement& element )
{
  const QString tagName = element.tagName();
  switch ( tagName[0].toLatin1() ) {
  case 'a':
    if ( tagName == "address" )
      return loadAddressAttribute( element );
    if ( tagName == "assistant" ) {
      setAssistant( element.text() );
      return true;
    }
    if ( tagName == "anniversary" ) {
      if ( !element.text().isEmpty() )
        setAnniversary( stringToDate( element.text() ) );
      return true;
    }
    break;
  case 'b':
    if ( tagName == "birthday" ) {
      if ( !element.text().isEmpty() )
        setBirthday( stringToDate( element.text() ) );
      return true;
    }
    break;
  case 'c':
    if ( tagName == "children" ) {
      setChildren( element.text() );
      return true;
    }
    break;
  case 'd':
    if ( tagName == "department" ) {
      setDepartment( element.text() );
      return true;
    }
    if ( mIsDistributionList && tagName == "display-name" ) {
      setFullName( element.text() );
      return true;
    }
    break;
  case 'e':
    if ( tagName == "email" ) {
      Email email;
      if ( loadEmailAttribute( element, email ) ) {
        addEmail( email );
        return true;
      } else
        return false;
    }
    break;
  case 'f':
    if ( tagName == "free-busy-url" ) {
      setFreeBusyUrl( element.text() );
      return true;
    }
    break;
  case 'g':
    if ( tagName == "gender" ) {
      setGender( element.text() );
      return true;
    }
    break;
  case 'i':
    if ( tagName == "im-address" ) {
      setIMAddress( element.text() );
      return true;
    }
    break;
  case 'j':
   if ( tagName == "job-title" ) {
     // see saveAttributes: <job-title> is mapped to the Role field
      setTitle( element.text() );
      return true;
    }
    break;
  case 'l':
    if ( tagName == "language" ) {
      setLanguage( element.text() );
      return true;
    }
    if ( tagName == "latitude" ) {
      setLatitude( element.text().toFloat() );
      mHasGeo = true;
      return true;
    }
    if ( tagName == "longitude" ) {
      setLongitude( element.text().toFloat() );
      mHasGeo = true;
    }
    break;
  case 'm':
    if ( tagName == "manager-name" ) {
      setManagerName( element.text() );
      return true;
    }
    if ( mIsDistributionList && tagName == "member" ) {
      loadDistrListMember( element );
      return true;
    }
    break;
  case 'n':
    if ( tagName == "name" )
      return loadNameAttribute( element );
    if ( tagName == "nick-name" ) {
      setNickName( element.text() );
      return true;
    }
    break;
  case 'o':
    if ( tagName == "organization" ) {
      setOrganization( element.text() );
      return true;
    }
    if ( tagName == "office-location" ) {
      setOfficeLocation( element.text() );
      return true;
    }
    break;
  case 'p':
    if ( tagName == "profession" ) {
      setProfession( element.text() );
      return true;
    }
    if ( tagName == "picture" ) {
      mPictureAttachmentName = element.text();
      return true;
    }
    if ( tagName == "phone" ) {
      return loadPhoneAttribute( element );
      return true;
    }
    if ( tagName == "preferred-address" ) {
      setPreferredAddress( element.text() );
      return true;
    }
    break;
  case 'r':
    if ( tagName == "role" ) {
      setRole( element.text() );
      return true;
    }
    break;
  case 's':
    if ( tagName == "spouse-name" ) {
      setSpouseName( element.text() );
      return true;
    }
    break;
  case 'x':
    if ( tagName == "x-logo" ) {
      mLogoAttachmentName = element.text();
      return true;
    }
    if ( tagName == "x-sound" ) {
      mSoundAttachmentName = element.text();
      return true;
    }
    if ( tagName == "x-custom" ) {
      loadCustomAttributes( element );
      return true;
    }
    if ( tagName == "x-title" ) {
      setTitle( element.text() );
      return true;
    }
    break;
  case 'w':
    if ( tagName == "web-page" ) {
      setWebPage( element.text() );
      return true;
    }
    break;
  default:
    break;
  }
  return KolabBase::loadAttribute( element );
}

bool Contact::saveAttributes( QDomElement& element ) const
{
  // Save the base class elements
  KolabBase::saveAttributes( element );
  if ( mIsDistributionList ) {
    writeString( element, "display-name", fullName() );
    saveDistrListMembers( element );
  } else {
    saveNameAttribute( element );
    writeString( element, "free-busy-url", freeBusyUrl() );
    writeString( element, "organization", organization() );
    writeString( element, "web-page", webPage() );
    writeString( element, "im-address", imAddress() );
    writeString( element, "department", department() );
    writeString( element, "office-location", officeLocation() );
    writeString( element, "profession", profession() );
    writeString( element, "role", role() );
    writeString( element, "job-title", title() );
    writeString( element, "manager-name", managerName() );
    writeString( element, "assistant", assistant() );
    writeString( element, "nick-name", nickName() );
    writeString( element, "spouse-name", spouseName() );
    writeString( element, "birthday", dateToString( birthday() ) );
    writeString( element, "anniversary", dateToString( anniversary() ) );
    if ( !picture().isNull() )
      writeString( element, "picture", mPictureAttachmentName );
    if ( !logo().isNull() )
      writeString( element, "x-logo", mLogoAttachmentName );
    if ( !sound().isNull() )
      writeString( element, "x-sound", mSoundAttachmentName );
    writeString( element, "children", children() );
    writeString( element, "gender", gender() );
    writeString( element, "language", language() );
    savePhoneAttributes( element );
    saveEmailAttributes( element );
    saveAddressAttributes( element );
    writeString( element, "preferred-address", preferredAddress() );
    if ( mHasGeo ) {
      writeString( element, "latitude", QString::number( latitude(), 'g', DBL_DIG ) );
      writeString( element, "longitude", QString::number( longitude(), 'g', DBL_DIG ) );
    }
  }
  saveCustomAttributes( element );

  return true;
}

bool Contact::loadXML( const QDomDocument& document )
{
  QDomElement top = document.documentElement();

  mIsDistributionList = top.tagName() == "distribution-list";
  if ( top.tagName() != "contact" && !mIsDistributionList ) {
    qWarning( "XML error: Top tag was %s instead of the expected contact or distribution-list",
              top.tagName().toAscii().data() );
    return false;
  }


  for ( QDomNode n = top.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    if ( n.isComment() )
      continue;
    if ( n.isElement() ) {
      QDomElement e = n.toElement();
      if ( !loadAttribute( e ) ) {
        // Unhandled tag - save for later storage
        //kDebug() <<"Saving unhandled tag" << e.tagName();
        Custom c;
        c.app = s_unhandledTagAppName;
        c.name = e.tagName();
        c.value = e.text();
        mCustomList.append( c );
      }
    } else
      kDebug() <<"Node is not a comment or an element???";
  }

  return true;
}

QString Contact::saveXML() const
{
  QDomDocument document = domTree();
  QDomElement element = document.createElement(
    mIsDistributionList ? "distribution-list" : "contact" );
  element.setAttribute( "version", "1.0" );
  saveAttributes( element );
  document.appendChild( element );
  return document.toString();
}

static QString addressTypeToString( int /*KABC::Address::Type*/ type )
{
  if ( type & KABC::Address::Home )
    return "home";
  if ( type & KABC::Address::Work )
    return "business";
  return "other";
}

static int addressTypeFromString( const QString& type )
{
  if ( type == "home" )
    return KABC::Address::Home;
  if ( type == "business" )
    return KABC::Address::Work;
  // well, this shows "other" in the editor, which is what we want...
  return KABC::Address::Dom | KABC::Address::Intl | KABC::Address::Postal | KABC::Address::Parcel;
}

static QStringList phoneTypeToString( KABC::PhoneNumber::Type type )
{
  // KABC has a bitfield, i.e. the same phone number can be used for work and home
  // and fax and cellphone etc. etc.
  // So when saving we need to create as many tags as bits that were set.
  QStringList types;
  if ( type & KABC::PhoneNumber::Fax ) {
    if ( type & KABC::PhoneNumber::Home )
      types << "homefax";
    else // assume work -- if ( type & KABC::PhoneNumber::Work )
      types << "businessfax";
    type = type & ~KABC::PhoneNumber::Home;
    type = type & ~KABC::PhoneNumber::Work;
  }

  // To support both "home1" and "home2", map Home+Pref to home1
  if ( ( type & KABC::PhoneNumber::Home ) && ( type & KABC::PhoneNumber::Pref ) )
  {
      types << "home1";
      type = type & ~KABC::PhoneNumber::Home;
      type = type & ~KABC::PhoneNumber::Pref;
  }
  // To support both "business1" and "business2", map Work+Pref to business1
  if ( ( type & KABC::PhoneNumber::Work ) && ( type & KABC::PhoneNumber::Pref ) )
  {
      types << "business1";
      type = type & ~KABC::PhoneNumber::Work;
      type = type & ~KABC::PhoneNumber::Pref;
  }


  if ( type & KABC::PhoneNumber::Home )
    types << "home2";
  if ( type & KABC::PhoneNumber::Msg ) // Msg==messaging
    types << "company";
  if ( type & KABC::PhoneNumber::Work )
    types << "business2";
  if ( type & KABC::PhoneNumber::Pref )
    types << "primary";
  if ( type & KABC::PhoneNumber::Voice )
    types << "callback"; // ##
  if ( type & KABC::PhoneNumber::Cell )
    types << "mobile";
  if ( type & KABC::PhoneNumber::Video )
    types << "radio"; // ##
  if ( type & KABC::PhoneNumber::Bbs )
    types << "ttytdd";
  if ( type & KABC::PhoneNumber::Modem )
    types << "telex"; // #
  if ( type & KABC::PhoneNumber::Car )
    types << "car";
  if ( type & KABC::PhoneNumber::Isdn )
    types << "isdn";
  if ( type & KABC::PhoneNumber::Pcs )
    types << "assistant"; // ## Assistant is e.g. secretary
  if ( type & KABC::PhoneNumber::Pager )
    types << "pager";
  return types;
}

static KABC::PhoneNumber::Type phoneTypeFromString( const QString& type )
{
  if ( type == "homefax" )
    return KABC::PhoneNumber::Home | KABC::PhoneNumber::Fax;
  if ( type == "businessfax" )
    return KABC::PhoneNumber::Work | KABC::PhoneNumber::Fax;
  if ( type == "business1" )
    return KABC::PhoneNumber::Work | KABC::PhoneNumber::Pref;
  if ( type == "business2" )
    return KABC::PhoneNumber::Work;
  if ( type == "home1" )
    return KABC::PhoneNumber::Home | KABC::PhoneNumber::Pref;
  if ( type == "home2" )
    return KABC::PhoneNumber::Home;
  if ( type == "company" )
    return KABC::PhoneNumber::Msg;
  if ( type == "primary" )
    return KABC::PhoneNumber::Pref;
  if ( type == "callback" )
    return KABC::PhoneNumber::Voice;
  if ( type == "mobile" )
    return KABC::PhoneNumber::Cell;
  if ( type == "radio" )
    return KABC::PhoneNumber::Video;
  if ( type == "ttytdd" )
    return KABC::PhoneNumber::Bbs;
  if ( type == "telex" )
    return KABC::PhoneNumber::Modem;
  if ( type == "car" )
    return KABC::PhoneNumber::Car;
  if ( type == "isdn" )
    return KABC::PhoneNumber::Isdn;
  if ( type == "assistant" )
    return KABC::PhoneNumber::Pcs;
  if ( type == "pager" )
    return KABC::PhoneNumber::Pager;
  return KABC::PhoneNumber::Home; // whatever
}

static const char* s_knownCustomFields[] = {
  "X-IMAddress",
  "X-Office",
  "X-Profession",
  "X-ManagersName",
  "X-AssistantsName",
  "X-SpousesName",
  "X-Anniversary",
  "DistributionList",
  0
};


// The saving is addressee -> Contact -> xml, this is the first part
void Contact::setFields( const KABC::Addressee* addressee, KABC::AddressBook* addressBook )
{
  KolabBase::setFields( addressee );

  mIsDistributionList = KPIM::DistributionList::isDistributionList( *addressee );
  if ( mIsDistributionList ) {
    // Hopefully all resources are available during saving, so we can look up
    // in the addressbook to get name+email from the UID.
    KPIM::DistributionList distrList( *addressee );
    const KPIM::DistributionList::Entry::List entries = distrList.entries( addressBook );
    KPIM::DistributionList::Entry::List::ConstIterator it = entries.constBegin();
    for ( ; it != entries.constEnd() ; ++it ) {
      Member m;
      m.displayName = (*it).addressee.formattedName();
      m.email = (*it).email;
      if ( m.email.isEmpty() )
        m.email = (*it).addressee.preferredEmail();
      mDistrListMembers.append( m );
    }
  }

  setGivenName( addressee->givenName() );
  setMiddleNames( addressee->additionalName() );
  setLastName( addressee->familyName() );
  setFullName( addressee->formattedName() );
  setPrefix( addressee->prefix() );
  setSuffix( addressee->suffix() );
  setOrganization( addressee->organization() );
  setWebPage( addressee->url().url() );
  setIMAddress( addressee->custom( "KADDRESSBOOK", "X-IMAddress" ) );
  setDepartment( addressee->department());
  setOfficeLocation( addressee->custom( "KADDRESSBOOK", "X-Office" ) );
  setProfession( addressee->custom( "KADDRESSBOOK", "X-Profession" ) );
  setRole( addressee->role() );
  setTitle( addressee->title() );
  setManagerName( addressee->custom( "KADDRESSBOOK", "X-ManagersName" ) );
  setAssistant( addressee->custom( "KADDRESSBOOK", "X-AssistantsName" ) );
  setNickName( addressee->nickName() );
  setSpouseName( addressee->custom( "KADDRESSBOOK", "X-SpousesName" ) );
  if ( !addressee->birthday().isNull() )
    setBirthday( addressee->birthday().date() );
  const QString& anniversary = addressee->custom( "KADDRESSBOOK", "X-Anniversary" );
  if ( !anniversary.isEmpty() )
    setAnniversary( stringToDate( anniversary  ) );

  const QStringList emails = addressee->emails();
  // Conversion problem here:
  // KABC::Addressee has only one full name and N addresses, but the XML format
  // has N times (fullname+address). So we just copy the fullname over and ignore it on loading.
  for ( QStringList::ConstIterator it = emails.constBegin(); it != emails.constEnd(); ++it ) {
    Email email;
    email.displayName = fullName();
    email.smtpAddress = *it;
    addEmail( email );
  }

  // Now the real-world addresses
  QString preferredAddress = "home";
  const KABC::Address::List addresses = addressee->addresses();
  for ( KABC::Address::List::ConstIterator it = addresses.constBegin() ; it != addresses.constEnd(); ++it ) {
    Address address;
    address.kdeAddressType = (*it).type();
    address.type = addressTypeToString( address.kdeAddressType );
    address.street = (*it).street();
    address.pobox = (*it).postOfficeBox();
    address.locality = (*it).locality();
    address.region = (*it).region();
    address.postalCode = (*it).postalCode();
    address.country = (*it).country();
    // ## TODO not in the XML format: extended address info.
    // ## KDE-specific tags? Or hiding those fields? Or adding a warning?
    addAddress( address );
    if ( address.kdeAddressType & KABC::Address::Pref ) {
      preferredAddress = address.type; // home, business or other
    }
  }
  setPreferredAddress( preferredAddress );

  const KABC::PhoneNumber::List phones = addressee->phoneNumbers();
  for ( KABC::PhoneNumber::List::ConstIterator it = phones.constBegin(); it != phones.constEnd(); ++it ) {
    // Create a tag per phone type set in the bitfield
    QStringList types = phoneTypeToString( (*it).type() );
    for( QStringList::ConstIterator typit = types.constBegin(); typit != types.constEnd(); ++typit ) {
      PhoneNumber phoneNumber;
      phoneNumber.type = *typit;
      phoneNumber.number = (*it).number();
      addPhoneNumber( phoneNumber );
    }
  }

  setPicture( loadPictureFromAddressee( addressee->photo() ) );
  mPictureAttachmentName = addressee->custom( "KOLAB", "PictureAttachmentName" );
  if ( mPictureAttachmentName.isEmpty() )
    mPictureAttachmentName = s_pictureAttachmentName;

  setLogo( loadPictureFromAddressee( addressee->logo() ) );
  mLogoAttachmentName = addressee->custom( "KOLAB", "LogoAttachmentName" );
  if ( mLogoAttachmentName.isEmpty() )
    mLogoAttachmentName = s_logoAttachmentName;

  setSound( loadSoundFromAddressee( addressee->sound() ) );
  mSoundAttachmentName = addressee->custom( "KOLAB", "SoundAttachmentName" );
  if ( mSoundAttachmentName.isEmpty() )
    mSoundAttachmentName = s_soundAttachmentName;

  if ( addressee->geo().isValid() ) {
    setLatitude( addressee->geo().latitude() );
    setLongitude( addressee->geo().longitude() );
    mHasGeo = true;
  }

  // Other KADDRESSBOOK custom fields than those already handled
  //    (includes e.g. crypto settings, and extra im addresses)
  QStringList knownCustoms;
  for ( const char** p = s_knownCustomFields; *p; ++p )
    knownCustoms << QString::fromLatin1( *p );
  QStringList customs = addressee->customs();
  for( QStringList::ConstIterator it = customs.constBegin(); it != customs.constEnd(); ++it ) {
    // KABC::Addressee doesn't offer a real way to iterate over customs, other than splitting strings ourselves
    // The format is "app-name:value".
    int pos = (*it).indexOf( '-' );
    if ( pos == -1 ) continue;
    QString app = (*it).left( pos );
    if ( app == "KOLAB" ) continue;
    QString name = (*it).mid( pos + 1 );
    pos = name.indexOf( ':' );
    if ( pos == -1 ) continue;
    QString value = name.mid( pos + 1 );
    name = name.left( pos );
    if ( !knownCustoms.contains( name ) ) {
      //kDebug() <<"app=" << app <<" name=" << name <<" value=" << value;
      Custom c;
      if ( app != "KADDRESSBOOK" ) // that's the default
        c.app = app;
      c.name = name;
      c.value = value;
      mCustomList.append( c );
    }
  }

  // Those fields, although defined in Addressee, are not used in KDE
  // (e.g. not visible in kaddressbook/addresseeeditorwidget.cpp)
  // So it doesn't matter much if we don't have them in the XML.
  // mailer, timezone, productId, sortString, agent, rfc2426 name()

  // Things KAddressBook can't handle, so they are saved as unhandled tags:
  // initials, children, gender, language

  // TODO: Free/Busy URL. This is done rather awkward in KAddressBook -
  // it stores it in a local file through a korganizer file :-(
}


// The loading is: xml -> Contact -> addressee, this is the second part
void Contact::saveTo( KABC::Addressee* addressee )
{
  // TODO: This needs the same set of TODOs as the setFields method
  KolabBase::saveTo( addressee );

  if ( mIsDistributionList ) {
    KPIM::DistributionList distrList( *addressee );
    distrList.setName( fullName() );
    QList<Member>::ConstIterator mit = mDistrListMembers.constBegin();
    for ( ; mit != mDistrListMembers.constEnd(); ++mit ) {
      QString displayName = (*mit).displayName;
      // fixup the display name DistributionList::assumes neither ',' nor ';' is present
      displayName.replace( ',', ' ' );
      displayName.replace( ';', ' ' );
      distrList.insertEntry( displayName, (*mit).email );
    }
    addressee->insertCustom( "KADDRESSBOOK", "DistributionList", distrList.custom( "KADDRESSBOOK", "DistributionList" ) );
    Q_ASSERT( KPIM::DistributionList::isDistributionList( *addressee ) );
  }

  addressee->setGivenName( givenName() );
  addressee->setAdditionalName( middleNames() );
  addressee->setFamilyName( lastName() );
  addressee->setFormattedName( fullName() );
  if ( mIsDistributionList )
    addressee->setName( fullName() );
  addressee->setPrefix( prefix() );
  addressee->setSuffix( suffix() );
  addressee->setOrganization( organization() );
  addressee->setUrl( webPage() );
  addressee->insertCustom( "KADDRESSBOOK", "X-IMAddress", imAddress() );
#if KDE_IS_VERSION(3,5,8)
  addressee->setDepartment( department() );
#else
  addressee->insertCustom( "KADDRESSBOOK", "X-Department", department() );
#endif
  addressee->insertCustom( "KADDRESSBOOK", "X-Office", officeLocation() );
  addressee->insertCustom( "KADDRESSBOOK", "X-Profession", profession() );
  addressee->setRole( role() );
  addressee->setTitle( title() );
  addressee->insertCustom( "KADDRESSBOOK", "X-ManagersName", managerName() );
  addressee->insertCustom( "KADDRESSBOOK", "X-AssistantsName", assistant() );
  addressee->setNickName( nickName() );
  addressee->insertCustom( "KADDRESSBOOK", "X-SpousesName", spouseName() );
  if ( birthday().isValid() )
    addressee->setBirthday( QDateTime( birthday() ) );

  if ( anniversary().isValid() )
    addressee->insertCustom( "KADDRESSBOOK", "X-Anniversary",
                             dateToString( anniversary() ) );
  else
    addressee->removeCustom( "KADDRESSBOOK", "X-Anniversary" );

  // We need to store both the original attachment name and the picture data into the addressee.
  // This is important, otherwise we would save the image under another attachment name w/o deleting the original one!
  if ( !mPicture.isNull() )
    addressee->setPhoto( KABC::Picture( mPicture ) );
  // Note that we must save the filename in all cases, so that removing the picture
  // actually deletes the attachment.
  addressee->insertCustom( "KOLAB", "PictureAttachmentName", mPictureAttachmentName );
  if ( !mLogo.isNull() )
    addressee->setLogo( KABC::Picture( mLogo ) );
  addressee->insertCustom( "KOLAB", "LogoAttachmentName", mLogoAttachmentName );
  if ( !mSound.isNull() )
    addressee->setSound( KABC::Sound( mSound ) );
  addressee->insertCustom( "KOLAB", "SoundAttachmentName", mSoundAttachmentName );

  if ( mHasGeo )
    addressee->setGeo( KABC::Geo( mLatitude, mLongitude ) );

  QStringList emailAddresses;
  for ( QList<Email>::ConstIterator it = mEmails.constBegin(); it != mEmails.constEnd(); ++it ) {
    // we can't do anything with (*it).displayName
    emailAddresses.append( (*it).smtpAddress );
  }
  addressee->setEmails( emailAddresses );

  for ( QList<Address>::ConstIterator it = mAddresses.constBegin(); it != mAddresses.constEnd(); ++it ) {
    KABC::Address address;
    int type = (*it).kdeAddressType;
    if ( type == -1 ) { // no kde-specific type available
      type = addressTypeFromString( (*it).type );
      if ( (*it).type == mPreferredAddress )
        type |= KABC::Address::Pref;
    }
    address.setType( static_cast<KABC::Address::Type>(type) );
    address.setStreet( (*it).street );
    address.setPostOfficeBox( (*it).pobox );
    address.setLocality( (*it).locality );
    address.setRegion( (*it).region );
    address.setPostalCode( (*it).postalCode );
    address.setCountry( (*it).country );
    addressee->insertAddress( address );
  }

  for ( QList<PhoneNumber>::ConstIterator it = mPhoneNumbers.constBegin(); it != mPhoneNumbers.constEnd(); ++it ) {
    KABC::PhoneNumber number;
    number.setType( phoneTypeFromString( (*it).type ) );
    number.setNumber( (*it).number );
    addressee->insertPhoneNumber( number );
  }

  for( QList<Custom>::ConstIterator it = mCustomList.constBegin(); it != mCustomList.constEnd(); ++it ) {
    QString app = (*it).app.isEmpty() ? QString::fromLatin1( "KADDRESSBOOK" ) : (*it).app;
    addressee->insertCustom( app, (*it).name, (*it).value );
  }
  //kDebug(5006) << addressee->customs();
}

QImage Contact::loadPictureFromAddressee( const KABC::Picture& picture )
{
  QImage img;
  if ( !picture.isIntern() && !picture.url().isEmpty() ) {
    QString tmpFile;
    if ( KIO::NetAccess::download( picture.url(), tmpFile, 0 /*no widget known*/ ) ) {
      img.load( tmpFile );
      KIO::NetAccess::removeTempFile( tmpFile );
    }
  } else
    img = picture.data();
  return img;
}

QByteArray Kolab::Contact::loadSoundFromAddressee( const KABC::Sound& sound )
{
  QByteArray data;
  if ( !sound.isIntern() && !sound.url().isEmpty() ) {
    QString tmpFile;
    if ( KIO::NetAccess::download( sound.url(), tmpFile, 0 /*no widget known*/ ) ) {
      QFile f( tmpFile );
      if ( f.open( QIODevice::ReadOnly ) ) {
        data = f.readAll();
        f.close();
      }
      KIO::NetAccess::removeTempFile( tmpFile );
    }
  } else
    data = sound.data();
  return data;
}

QString Kolab::Contact::productID() const
{
  // TODO: When KAB has the version number in a header file, don't hardcode (Bo)
  // Or we could use Addressee::productID? (David)
  return "KAddressBook 3.3, Kolab resource";
}
