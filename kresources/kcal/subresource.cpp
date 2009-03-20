/*
    This file is part of libkcal.
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

#include "concurrentjobs.h"

#include <kcal/calformat.h>

#include <KConfigGroup>
#include <KDebug>
#include <KMimeType>

using namespace KCal;

QString IdArbiter::createArbitratedId() const
{
  // FIXME needs to be a calendar UID
  QString id;
  do {
    id = CalFormat::createUniqueId();
  } while ( mArbitratedToOriginal.constFind( id ) != mArbitratedToOriginal.constEnd() );

  return id;
}

SubResource::SubResource( const Akonadi::Collection &collection )
  : SubResourceBase( collection )
{
}

SubResource::~SubResource()
{
}

QStringList SubResource::supportedMimeTypes()
{
  return QStringList() << QLatin1String( "text/calendar" );
}

bool SubResource::createChildSubResource( const QString &resourceName )
{
  if ( ( mCollection.rights() & Akonadi::Collection::CanCreateCollection ) == 0 ) {
    kError(5800) << "Parent collection does not allow creation of child collections";
    return false;
  }

  Akonadi::Collection collection;
  collection.setName( resourceName );
  collection.setParent( mCollection );

  // TODO should we set content mime types at all?
  collection.setContentMimeTypes( mCollection.contentMimeTypes() );

  ConcurrentCollectionCreateJob colCreateJob( collection );
  if ( !colCreateJob.exec() ) {
    kError( 5800 ) << "CollectionCreateJob failed:" << colCreateJob->errorString();
    return false;
  }

  return true;
}

bool SubResource::remove()
{
  ConcurrentCollectionDeleteJob colDeleteJob( mCollection );
  if ( !colDeleteJob.exec() ) {
    kError( 5800 ) << "CollectionDeleteJob failed:" << colDeleteJob->errorString();
    return false;
  }

  return true;
}

QString SubResource::subResourceType() const
{
  QStringList mimeTypes = mCollection.contentMimeTypes();
  mimeTypes.removeAll( Akonadi::Collection::mimeType() );
  if ( mimeTypes.count() > 1 )
    return QString(); // mixed

  KMimeType::Ptr mimeType = KMimeType::mimeType( mimeTypes[ 0 ], KMimeType::ResolveAliases );
  if ( !mimeType.isNull() ) {
    if ( mimeType->is( QLatin1String( "application/x-vnd.akonadi.calendar.event" ) ) )
      return QLatin1String( "event" );

    if ( mimeType->is( QLatin1String( "application/x-vnd.akonadi.calendar.todo" ) ) )
      return QLatin1String( "todo" );

    if ( mimeType->is( QLatin1String( "application/x-vnd.akonadi.calendar.journal" ) ) )
      return QLatin1String( "journal" );
  }

  return QString();
}

void SubResource::itemAdded( const Akonadi::Item &item )
{
  Q_ASSERT( mIdArbiter != 0 );

  QString arbitratedId;
  if ( item.hasPayload<IncidencePtr>() ) {
    IncidencePtr incidencePtr = item.payload<IncidencePtr>();

    arbitratedId = mIdArbiter->arbitrateOriginalId( incidencePtr->uid() );
    incidencePtr->setUid( arbitratedId );

    emit incidenceAdded( incidencePtr, subResourceIdentifier() );
  } else {
    kError( 5800 ) << "No IncidencePtr payload";
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

  if ( item.hasPayload<IncidencePtr>() ) {
    IncidencePtr incidencePtr = item.payload<IncidencePtr>();

    incidencePtr->setUid( kresId );

    emit incidenceChanged( incidencePtr, subResourceIdentifier() );
  } else {
    kError( 5800 ) << "No IncidencePtr payload";
    return;
  }

  mMappedItems[ kresId ] = item;
}

void SubResource::itemRemoved( const Akonadi::Item &item )
{
  Q_ASSERT( mIdArbiter != 0 );

  const QString kresId = mMappedIds.value( item.id() );
  Q_ASSERT( !kresId.isEmpty() );

  emit incidenceRemoved( kresId, subResourceIdentifier() );

  mMappedItems.remove( kresId );
  mMappedIds.remove( item.id() );

  mIdArbiter->removeArbitratedId( kresId );
}

#include "subresource.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
