/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "addresseediffalgo.h"

using namespace KPIM;

static QString filterString( const QString &str )
{
  if ( str.isEmpty() )
    return QString::null;
  else
    return str;
}

AddresseeDiffAlgo::AddresseeDiffAlgo( const KABC::Addressee &leftAddressee,
                              const KABC::Addressee &rightAddressee )
  : mLeftAddressee( leftAddressee ), mRightAddressee( rightAddressee )
{
}

void AddresseeDiffAlgo::run()
{
  begin();

  if ( filterString( mLeftAddressee.uid() ) != filterString( mRightAddressee.uid() ) )
    conflictField( KABC::Addressee::uidLabel(), mLeftAddressee.uid(), mRightAddressee.uid() );

  if ( filterString( mLeftAddressee.name() ) != filterString( mRightAddressee.name() ) )
    conflictField( KABC::Addressee::nameLabel(), mLeftAddressee.name(), mRightAddressee.name() );

  if ( filterString( mLeftAddressee.formattedName() ) != filterString( mRightAddressee.formattedName() ) )
    conflictField( KABC::Addressee::formattedNameLabel(), mLeftAddressee.formattedName(), mRightAddressee.formattedName() );

  if ( filterString( mLeftAddressee.familyName() ) != filterString( mRightAddressee.familyName() ) )
    conflictField( KABC::Addressee::familyNameLabel(), mLeftAddressee.familyName(), mRightAddressee.familyName() );

  if ( filterString( mLeftAddressee.givenName() ) != filterString( mRightAddressee.givenName() ) )
    conflictField( KABC::Addressee::givenNameLabel(), mLeftAddressee.givenName(), mRightAddressee.givenName() );

  if ( filterString( mLeftAddressee.additionalName() ) != filterString( mRightAddressee.additionalName() ) )
    conflictField( KABC::Addressee::additionalNameLabel(), mLeftAddressee.additionalName(), mRightAddressee.additionalName() );

  if ( filterString( mLeftAddressee.prefix() ) != filterString( mRightAddressee.prefix() ) )
    conflictField( KABC::Addressee::prefixLabel(), mLeftAddressee.prefix(), mRightAddressee.prefix() );

  if ( filterString( mLeftAddressee.suffix() ) != filterString( mRightAddressee.suffix() ) )
    conflictField( KABC::Addressee::suffixLabel(), mLeftAddressee.suffix(), mRightAddressee.suffix() );

  if ( filterString( mLeftAddressee.nickName() ) != filterString( mRightAddressee.nickName() ) )
    conflictField( KABC::Addressee::nickNameLabel(), mLeftAddressee.nickName(), mRightAddressee.nickName() );

  if ( mLeftAddressee.birthday() != mRightAddressee.birthday() )
    conflictField( KABC::Addressee::birthdayLabel(), mLeftAddressee.birthday().toString(),
                   mRightAddressee.birthday().toString() );

  if ( filterString( mLeftAddressee.mailer() ) != filterString( mRightAddressee.mailer() ) )
    conflictField( KABC::Addressee::mailerLabel(), mLeftAddressee.mailer(), mRightAddressee.mailer() );

  if ( mLeftAddressee.timeZone() != mRightAddressee.timeZone() )
    conflictField( KABC::Addressee::timeZoneLabel(), mLeftAddressee.timeZone().asString(), mRightAddressee.timeZone().asString() );

  if ( mLeftAddressee.geo() != mRightAddressee.geo() )
    conflictField( KABC::Addressee::geoLabel(), mLeftAddressee.geo().asString(), mRightAddressee.geo().asString() );

  if ( filterString( mLeftAddressee.title() ) != filterString( mRightAddressee.title() ) )
    conflictField( KABC::Addressee::titleLabel(), mLeftAddressee.title(), mRightAddressee.title() );

  if ( filterString( mLeftAddressee.role() ) != filterString( mRightAddressee.role() ) )
    conflictField( KABC::Addressee::roleLabel(), mLeftAddressee.role(), mRightAddressee.role() );

  if ( filterString( mLeftAddressee.organization() ) != filterString( mRightAddressee.organization() ) )
    conflictField( KABC::Addressee::organizationLabel(), mLeftAddressee.organization(), mRightAddressee.organization() );

  if ( filterString( mLeftAddressee.note() ) != filterString( mRightAddressee.note() ) )
    conflictField( KABC::Addressee::noteLabel(), mLeftAddressee.note(), mRightAddressee.note() );

  if ( filterString( mLeftAddressee.productId() ) != filterString( mRightAddressee.productId() ) )
    conflictField( KABC::Addressee::productIdLabel(), mLeftAddressee.productId(), mRightAddressee.productId() );

  if ( filterString( mLeftAddressee.sortString() ) != filterString( mRightAddressee.sortString() ) )
    conflictField( KABC::Addressee::sortStringLabel(), mLeftAddressee.sortString(), mRightAddressee.sortString() );

  if ( mLeftAddressee.secrecy() != mRightAddressee.secrecy() ) {
    conflictField( KABC::Addressee::secrecyLabel(), mLeftAddressee.secrecy().asString(), mRightAddressee.secrecy().asString() );
  }

  if ( mLeftAddressee.logo() != mRightAddressee.logo() ) {
  }

  if ( mLeftAddressee.photo() != mRightAddressee.photo() ) {
  }

  diffList( "Phone Numbers", mLeftAddressee.phoneNumbers(), mRightAddressee.phoneNumbers() );
  diffList( "Addresses", mLeftAddressee.addresses(), mRightAddressee.addresses() );

  end();
}

QString AddresseeDiffAlgo::toString( const KABC::PhoneNumber &number )
{
  return number.number();
}

QString AddresseeDiffAlgo::toString( const KABC::Address &addr )
{
  return addr.formattedAddress();
}

template <class L>
void AddresseeDiffAlgo::diffList( const QString &id,
                                  const QValueList<L> &left, const QValueList<L> &right )
{
  for ( uint i = 0; i < left.count(); ++i ) {
    if ( right.find( left[ i ] ) == right.end() )
      additionalLeftField( id, toString( left[ i ] ) );
  }

  for ( uint i = 0; i < right.count(); ++i ) {
    if ( left.find( right[ i ] ) == left.end() )
      additionalRightField( id, toString( right[ i ] ) );
  }
}
