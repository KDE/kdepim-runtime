
/*
    This file is part of libkabc.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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
*/

#include "subresource.h"

#include <akonadi/mimetypechecker.h>

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>

#include <KConfigGroup>
#include <KDebug>
#include <KRandom>

using namespace KABC;

QString IdArbiter::createArbitratedId() const
{
  QString id;
  do {
    id = KRandom::randomString(10);
  } while ( mArbitratedToOriginal.constFind( id ) != mArbitratedToOriginal.constEnd() );

  return id;
}

SubResource::SubResource( const Akonadi::Collection &collection )
  : SubResourceBase( collection ),
    mCompletionWeight( 80 )
{
}

SubResource::~SubResource()
{
}

QStringList SubResource::supportedMimeTypes()
{
  return QStringList() << Addressee::mimeType() << ContactGroup::mimeType();
}

void SubResource::setCompletionWeight( int weight )
{
  mCompletionWeight = weight;
}

int SubResource::completionWeight() const
{
  return mCompletionWeight;
}

void SubResource::readTypeSpecificConfig( const KConfigGroup &config )
{
  mCompletionWeight = config.readEntry<int>( QLatin1String( "CompletionWeight" ), 80 );
}

void SubResource::writeTypeSpecificConfig( KConfigGroup &config ) const
{
  config.writeEntry( QLatin1String( "CompletionWeight" ), mCompletionWeight );
}

void SubResource::collectionChanged( const Akonadi::Collection &collection )
{
  bool changed = false;

  const QString oldLabel = label( mCollection );
  const QString newLabel = label( collection );
  if ( oldLabel != newLabel ) {
    kDebug( 5700 ) << "SubResource label changed from" << oldLabel
                   << "to" << newLabel;
    changed = true;
  }

  const bool oldWritable = isWritable( mCollection );
  const bool newWritable = isWritable( collection );
  if ( oldWritable != newWritable ) {
    kDebug( 5700 ) << "SubResource isWritable changed from" << oldWritable
                   << "to" << newWritable;
    changed = true;
  }

  if ( changed ) {
    mCollection = collection;
    emit subResourceChanged( subResourceIdentifier() );
  }
}

void SubResource::itemAdded( const Akonadi::Item &item )
{
  Q_ASSERT( mIdArbiter != 0 );

  QString arbitratedId;
  QString kresId;
  if ( item.hasPayload<Addressee>() ) {
    Addressee addressee = item.payload<Addressee>();
    kresId = addressee.uid();

    arbitratedId = mIdArbiter->arbitrateOriginalId( addressee.uid() );
    addressee.setUid( arbitratedId );

    emit addresseeAdded( addressee, subResourceIdentifier() );
  } else if ( item.hasPayload<ContactGroup>() ) {
    ContactGroup contactGroup = item.payload<ContactGroup>();
    kresId = contactGroup.id();

    arbitratedId = mIdArbiter->arbitrateOriginalId( contactGroup.id() );
    contactGroup.setId( arbitratedId );

    emit contactGroupAdded( contactGroup, subResourceIdentifier() );
  } else {
    kError( 5700 ) << "Neither Addressee nor ContactGroup payload";
    return;
  }

  mMappedItems.insert( arbitratedId, item );
  mMappedIds.insert( item.id(), arbitratedId );
}

void SubResource::itemChanged( const Akonadi::Item &item )
{
  Q_ASSERT( mIdArbiter != 0 );

  const QString kresId = mMappedIds.value( item.id() );
  Q_ASSERT( !kresId.isEmpty() );

  if ( item.hasPayload<Addressee>() ) {
    Addressee addressee = item.payload<Addressee>();

    addressee.setUid( kresId );

    emit addresseeChanged( addressee, subResourceIdentifier() );
  } else if ( item.hasPayload<ContactGroup>() ) {
    ContactGroup contactGroup = item.payload<ContactGroup>();

    contactGroup.setId( kresId );

    emit contactGroupChanged( contactGroup, subResourceIdentifier() );
  } else {
    kError( 5700 ) << "Neither Addressee nor ContactGroup payload";
    return;
  }

  mMappedItems[ kresId ] = item;
}

void SubResource::itemRemoved( const Akonadi::Item &item )
{
  Q_ASSERT( mIdArbiter != 0 );

  const QString kresId = mMappedIds.value( item.id() );
  Q_ASSERT( !kresId.isEmpty() );

  if ( Akonadi::MimeTypeChecker::isWantedItem( item, Addressee::mimeType() ) ) {
    emit addresseeRemoved( kresId, subResourceIdentifier() );
  } else if ( Akonadi::MimeTypeChecker::isWantedItem( item, ContactGroup::mimeType() ) ) {
    emit contactGroupRemoved( kresId, subResourceIdentifier() );
  }

  mMappedItems.remove( kresId );
  mMappedIds.remove( item.id() );

  mIdArbiter->removeArbitratedId( kresId );
}

#include "subresource.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
