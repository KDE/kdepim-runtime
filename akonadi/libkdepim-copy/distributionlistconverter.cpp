/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "distributionlistconverter.h"

#include "distributionlist.h"

#include "kabc/distributionlist.h"
#include "kabc/resource.h"

namespace KPIM {
  class DistributionListConverter::Private
  {
    public:
      explicit Private( KABC::Resource *resource ) : mResource( resource ) {}

    public:
      KABC::Resource *mResource;
  };
}

KPIM::DistributionListConverter::DistributionListConverter( KABC::Resource *resource )
  : d( new Private( resource ) )
{
}

KPIM::DistributionListConverter::~DistributionListConverter()
{
  delete d;
}

KABC::DistributionList
*KPIM::DistributionListConverter::convertToKABC( const KPIM::DistributionList &pimList )
{
  KABC::DistributionList *kabcList =
    new KABC::DistributionList( d->mResource, pimList.uid(), pimList.name() );

  const KPIM::DistributionList::Entry::List entries =
    pimList.entries( d->mResource->addressBook() );
  foreach ( const KPIM::DistributionList::Entry &entry, entries ) {
    kabcList->insertEntry( entry.addressee, entry.email );
  }

  return kabcList;
}

KPIM::DistributionList
KPIM::DistributionListConverter::convertFromKABC( const KABC::DistributionList *kabcList )
{
  KPIM::DistributionList pimList;
  pimList.setUid( kabcList->identifier() );
  pimList.setName( kabcList->name() );

  const KABC::DistributionList::Entry::List entries = kabcList->entries();
  foreach ( const KABC::DistributionList::Entry &entry, entries ) {
    // KPIM::DistributionList doesn't like empty but not null email addresses
    QString email = entry.email();
    if ( email.isEmpty() && !email.isNull() )
      email = QString();

    pimList.insertEntry( entry.addressee(), entry.email() );
  }

  return pimList;
}

bool KPIM::DistributionListConverter::updateKABC( const KPIM::DistributionList &pimList, KABC::DistributionList *kabcList )
{
  Q_ASSERT( kabcList != 0 );

  if ( pimList.uid() != kabcList->identifier() ) {
    kError() << "Trying to update KABC distributionlist (id="
             << kabcList->identifier() << ", name=" << kabcList->name()
             << ") with different (id mismatch) KPIM distributionlist"
             << pimList.uid() << ", name=" << pimList.name() << ")";
    return false;
  }

  kabcList->setName( pimList.name() );

  const KABC::DistributionList::Entry::List kabcEntries = kabcList->entries();
  foreach ( const KABC::DistributionList::Entry &entry, kabcEntries ) {
    kabcList->removeEntry( entry.addressee(), entry.email() );
  }

  const KPIM::DistributionList::Entry::List pimEntries =
    pimList.entries( d->mResource->addressBook() );
  foreach ( const KPIM::DistributionList::Entry &entry, pimEntries ) {
    kabcList->insertEntry( entry.addressee, entry.email );
  }

  return true;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
