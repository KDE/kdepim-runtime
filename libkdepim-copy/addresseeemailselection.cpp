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

#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>

#include "recentaddresses.h"

#include "addresseeemailselection.h"

using namespace KPIM;
using KRecentAddress::RecentAddresses;

AddresseeEmailSelection::AddresseeEmailSelection()
  : Selection()
{
}

uint AddresseeEmailSelection::fieldCount() const
{
  return 3;
}

QString AddresseeEmailSelection::fieldTitle( uint index ) const
{
  switch ( index ) {
    case 0:
      return i18n( "To" );
      break;
    case 1:
      return i18n( "Cc" );
      break;
    case 2:
      return i18n( "Bcc" );
      break;
    default:
      return QString::null;
  }
}

QStringList AddresseeEmailSelection::to() const
{
  return mToEmailList;
}

QStringList AddresseeEmailSelection::cc() const
{
  return mCcEmailList;
}

QStringList AddresseeEmailSelection::bcc() const
{
  return mBccEmailList;
}

KABC::Addressee::List AddresseeEmailSelection::toAddresses() const
{
  return mToAddresseeList;
}

KABC::Addressee::List AddresseeEmailSelection::ccAddresses() const
{
  return mCcAddresseeList;
}

KABC::Addressee::List AddresseeEmailSelection::bccAddresses() const
{
  return mBccAddresseeList;
}

QStringList AddresseeEmailSelection::toDistributionLists() const
{
  return mToDistributionList;
}

QStringList AddresseeEmailSelection::ccDistributionLists() const
{
  return mCcDistributionList;
}

QStringList AddresseeEmailSelection::bccDistributionLists() const
{
  return mBccDistributionList;
}

void AddresseeEmailSelection::setSelectedTo( const QStringList &emails )
{
  setSelectedItem( 0, emails );
}

void AddresseeEmailSelection::setSelectedCC( const QStringList &emails )
{
  setSelectedItem( 1, emails );
}

void AddresseeEmailSelection::setSelectedBCC( const QStringList &emails )
{
  setSelectedItem( 2, emails );
}


uint AddresseeEmailSelection::itemCount( const KABC::Addressee &addressee ) const
{
  return addressee.emails().count();
}

QString AddresseeEmailSelection::itemText( const KABC::Addressee &addressee, uint index ) const
{
  return addressee.formattedName() + " " + email( addressee, index );
}

QPixmap AddresseeEmailSelection::itemIcon( const KABC::Addressee &addressee, uint ) const
{
  if ( !addressee.photo().data().isNull() )
    return addressee.photo().data().smoothScale( 16, 16 );
  else
    return KGlobal::iconLoader()->loadIcon( "personal", KIcon::Small );
}

bool AddresseeEmailSelection::itemEnabled( const KABC::Addressee &addressee, uint ) const
{
  return addressee.emails().count() != 0;
}

bool AddresseeEmailSelection::itemMatches( const KABC::Addressee &addressee, uint index, const QString &pattern ) const
{
  return addressee.formattedName().startsWith( pattern, false ) ||
         email( addressee, index ).startsWith( pattern, false );
}

bool AddresseeEmailSelection::itemEquals( const KABC::Addressee &addressee, uint index, const QString &pattern ) const
{
  return (pattern == addressee.formattedName() + " " + email( addressee, index )) ||
         (addressee.emails().contains( pattern ));
}

QString AddresseeEmailSelection::distributionListText( const KABC::DistributionList *distributionList ) const
{
  return distributionList->name();
}

QPixmap AddresseeEmailSelection::distributionListIcon( const KABC::DistributionList* ) const
{
  return KGlobal::iconLoader()->loadIcon( "kdmconfig", KIcon::Small );
}

bool AddresseeEmailSelection::distributionListEnabled( const KABC::DistributionList* ) const
{
  return true;
}

bool AddresseeEmailSelection::distributionListMatches( const KABC::DistributionList *distributionList,
                                                       const QString &pattern ) const
{
  // check whether the name of the distribution list matches the pattern or one of its entries.
  bool ok = distributionList->name().startsWith( pattern, false );

  KABC::DistributionList::Entry::List entries = distributionList->entries();
  KABC::DistributionList::Entry::List::ConstIterator it;
  for ( it = entries.begin(); it != entries.end(); ++it ) {
    ok = ok || (*it).addressee.formattedName().startsWith( pattern, false ) ||
               (*it).email.startsWith( pattern, false );
  }

  return ok;
}

uint AddresseeEmailSelection::addressBookCount() const
{
  // we provide the recent email addresses via the custom addressbooks
  return 1;
}

QString AddresseeEmailSelection::addressBookTitle( uint index ) const
{
  if ( index == 0 )
    return i18n( "Recent Addresses" );
  else
    return QString::null;
}

KABC::Addressee::List AddresseeEmailSelection::addressBookContent( uint index ) const
{
  if ( index == 0 ) {
    KConfig config( "kmailrc" );
    return RecentAddresses::self( &config )->kabcAddresses();
  } else {
    return KABC::Addressee::List();
  }
}

QString AddresseeEmailSelection::email( const KABC::Addressee &addressee, uint index ) const
{
  return addressee.emails()[ index ];
}

void AddresseeEmailSelection::setSelectedItem( uint fieldIndex, const QStringList &emails )
{
  QStringList::ConstIterator it;
  for ( it = emails.begin(); it != emails.end(); ++it ) {
    KABC::Addressee addr;
    addr.insertEmail( *it, true );

    selector()->setItemSelected( fieldIndex, addr, 0, *it );
  }
}

void AddresseeEmailSelection::addSelectedAddressees( uint fieldIndex, const KABC::Addressee &addressee, uint itemIndex )
{
  switch ( fieldIndex ) {
    case 0:
      mToAddresseeList.append( addressee );
      mToEmailList.append( email( addressee, itemIndex ) );
      break;
    case 1:
      mCcAddresseeList.append( addressee );
      mCcEmailList.append( email( addressee, itemIndex ) );
      break;
    case 2:
      mBccAddresseeList.append( addressee );
      mBccEmailList.append( email( addressee, itemIndex ) );
      break;
    default:
      // oops
      break;
  }
}

void AddresseeEmailSelection::addSelectedDistributionList( uint fieldIndex, const KABC::DistributionList *list )
{
  switch ( fieldIndex ) {
    case 0:
      mToDistributionList.append( list->name() );
      break;
    case 1:
      mCcDistributionList.append( list->name() );
      break;
    case 2:
      mBccDistributionList.append( list->name() );
      break;
    default:
      // oops
      break;
  }
}
