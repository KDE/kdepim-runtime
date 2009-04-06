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

#include "resourceakonadi_p.h"

#include "concurrentjobs.h"
#include "itemsavecontext.h"
#include "resourceakonadiconfig.h"
#include "storecollectiondialog.h"

#include <akonadi/agentfilterproxymodel.h>
#include <akonadi/agentinstancemodel.h>

#include <kabc/locknull.h>

using namespace KCal;

ResourceAkonadi::Private::Private( ResourceAkonadi *parent )
  : SharedResourcePrivate<SubResource>( new IdArbiter(), parent ),
    mParent( parent ), mCalendar( QLatin1String( "UTC" ) ),
    mLock( new KABC::LockNull( true ) ), mInternalCalendarModification( false ),
    mAgentModel( 0 ), mAgentFilterModel( 0 )
{
}

ResourceAkonadi::Private::Private( const KConfigGroup &config, ResourceAkonadi *parent )
  : SharedResourcePrivate<SubResource>( config, new IdArbiter(), parent ),
    mParent( parent ), mCalendar( QLatin1String( "UTC" ) ),
    mLock( new KABC::LockNull( true ) ), mInternalCalendarModification( false ),
    mAgentModel( 0 ), mAgentFilterModel( 0 )
{
}

ResourceAkonadi::Private::~Private()
{
  delete mLock;
}

bool ResourceAkonadi::Private::doSaveIncidence( Incidence *incidence )
{
  const ChangeByKResId::const_iterator findIt = mChanges.constFind( incidence->uid() );
  if ( findIt == mChanges.constEnd() ) {
    kWarning( 5800 ) << "No change for incidence (uid=" << incidence->uid()
                     << ", summary=" << incidence->summary() << ")";
    return true;
  }

  ItemSaveContext saveContext;
  if ( !prepareItemSaveContext( findIt, saveContext ) ) {
    const QString message = i18nc( "@info:status", "Processing change set failed" );
    savingResult( false, message );
    return false;
  }

  ConcurrentItemSaveJob itemSaveJob( saveContext );
  if ( !itemSaveJob.exec() ) {
    savingResult( false, itemSaveJob->errorString() );
    return false;
  }

  mChanges.remove( incidence->uid() );

  return true;
}

QString ResourceAkonadi::Private::subResourceIdentifier( const QString &incidenceUid ) const
{
  return mUidToResourceMap.value( incidenceUid );
}

bool ResourceAkonadi::Private::openResource()
{
  kDebug( 5800 ) << (void*) mAgentModel << "state=" << state();
  if ( mAgentModel == 0 && state() != Failed ) {
    mAgentModel = new Akonadi::AgentInstanceModel( this );
    mAgentFilterModel = new Akonadi::AgentFilterProxyModel( this );
    mAgentFilterModel->addCapabilityFilter( QLatin1String( "Resource" ) );
    mAgentFilterModel->addMimeTypeFilter( QLatin1String( "text/calendar" ) );

    mAgentFilterModel->setSourceModel( mAgentModel );
  }

  mCalendar.registerObserver( this );

  return true;
}

bool ResourceAkonadi::Private::closeResource()
{
  mCalendar.unregisterObserver( this );

  return true;
}

void ResourceAkonadi::Private::clearResource()
{
  SharedResourcePrivate<SubResource>::clearResource();
  // block scope for BoolGuard
  {
    BoolGuard internalModification( mInternalCalendarModification, true );
    mCalendar.close();
  }

  emit mParent->resourceChanged( mParent );
}

const SubResourceBase *ResourceAkonadi::Private::storeSubResourceFromUser( const QString &uid, const QString &mimeType )
{
  // TODO Strings should reflect whether this is a question for just one
  // item (ask always) or for all of a certain category (ask once)
  Q_UNUSED( uid );

  Q_ASSERT( mStoreCollectionDialog != 0 );

  if ( mimeType == Akonadi::KCalMimeTypeVisitor::eventMimeType() ) {
    mStoreCollectionDialog->setLabelText( i18nc( "@label where to store a calendar entry of type Event", "Please select a storage folder for this event" ) );
  } else if ( mimeType == Akonadi::KCalMimeTypeVisitor::todoMimeType() ) {
    mStoreCollectionDialog->setLabelText( i18nc( "@label where to store a calendar entry of type Todo", "Please select a storage folder for this todo" ) );
  } else if ( mimeType == Akonadi::KCalMimeTypeVisitor::journalMimeType() ) {
    mStoreCollectionDialog->setLabelText( i18nc( "@label where to store a calendar entry of type Journal", "Please select a storage folder for this journal" ) );
  } else if ( mimeType == QLatin1String( "text/calendar" ) ) {
    kWarning( 5800 ) << "Unexpected generic MIME type text/calendar";
    mStoreCollectionDialog->setLabelText( i18nc( "@label where to store a calendar entry of unspecified type", "Please select a storage folder for this calendar entry" ) );
  } else {
    kError( 5800 ) << "Unexpected MIME type:" << mimeType;
    mStoreCollectionDialog->setLabelText( i18nc( "@label", "Please select a storage folder" ) );
  }

  mStoreCollectionDialog->setMimeType( mimeType );

  const SubResourceBase *resource = 0;
  while ( resource == 0 ) {
    if ( mStoreCollectionDialog->exec() != QDialog::Accepted ) {
      return 0;
    }

    Akonadi::Collection collection = mStoreCollectionDialog->selectedCollection();
    if ( collection.isValid() ) {
      resource = mModel.subResource( collection.id() );
      if ( resource != 0 ) {
        setStoreCollectionForMimeType( mimeType, collection );
      }
    }
  }

  return resource;
}

Akonadi::Item ResourceAkonadi::Private::createItem( const QString &kresId )
{
  Akonadi::Item item;

  Incidence *cachedIncidence = mCalendar.incidence( kresId );
  kDebug( 5800 ) << "kresId=" << kresId
                 << "cachedIncidence=" << (void*) cachedIncidence;
  if ( cachedIncidence != 0 ) {
    item.setMimeType( mMimeVisitor.mimeType( cachedIncidence ) );
    item.setPayload<IncidencePtr>( IncidencePtr( cachedIncidence->clone() ) );
  }

  return item;
}

Akonadi::Item ResourceAkonadi::Private::updateItem( const Akonadi::Item &item, const QString &kresId, const QString &originalId )
{
  Akonadi::Item update( item );

  Incidence *cachedIncidence = mCalendar.incidence( kresId );
  if ( cachedIncidence != 0 ) {
    IncidencePtr incidencePtr( cachedIncidence->clone() );
    incidencePtr->setUid( originalId );

    update.setPayload<IncidencePtr>( incidencePtr );
  }

  return update;
}

void ResourceAkonadi::Private::calendarIncidenceAdded( Incidence *incidence )
{
  if ( mInternalCalendarModification ) {
    return;
  }

  // added to the calendar needs to be caught at resource level, e.g
  // addEvent() method
  Q_ASSERT( mUidToResourceMap.constFind( incidence->uid() ) != mUidToResourceMap.constEnd() );
}

void ResourceAkonadi::Private::calendarIncidenceChanged( Incidence *incidence )
{
  if ( mInternalCalendarModification ) {
    return;
  }

  kDebug( 5800 ) << "Incidence (uid=" << incidence->uid()
                 << ", summary=" << incidence->summary()
                 << ")";

  changeLocalItem( incidence->uid() );
}

void ResourceAkonadi::Private::calendarIncidenceDeleted( Incidence *incidence )
{
  if ( mInternalCalendarModification ) {
    return;
  }

  kDebug( 5800 ) << "Incidence (uid=" << incidence->uid()
                 << ", summary=" << incidence->summary()
                 << ")";

  removeLocalItem( incidence->uid() );
}

void ResourceAkonadi::Private::subResourceAdded( SubResourceBase *subResource )
{
  kDebug( 5800 ) << "id=" << subResource->subResourceIdentifier();

  SharedResourcePrivate<SubResource>::subResourceAdded( subResource );

  connect( subResource, SIGNAL( incidenceAdded( IncidencePtr, QString ) ),
           this, SLOT( incidenceAdded( IncidencePtr, QString ) ) );
  connect( subResource, SIGNAL( incidenceChanged( IncidencePtr, QString ) ),
           this, SLOT( incidenceChanged( IncidencePtr, QString ) ) );
  connect( subResource, SIGNAL( incidenceRemoved( QString, QString ) ),
           this, SLOT( incidenceRemoved( QString, QString ) ) );

  emit mParent->signalSubresourceAdded( mParent, QLatin1String( "calendar" ), subResource->subResourceIdentifier(), subResource->label() );
}

void ResourceAkonadi::Private::subResourceRemoved( SubResourceBase *subResource )
{
  kDebug( 5800 ) << "id=" << subResource->subResourceIdentifier();

  SharedResourcePrivate<SubResource>::subResourceRemoved( subResource );

  disconnect( subResource, SIGNAL( incidenceAdded( IncidencePtr, QString ) ),
              this, SLOT( incidenceAdded( IncidencePtr, QString ) ) );
  disconnect( subResource, SIGNAL( incidenceChanged( IncidencePtr, QString ) ),
              this, SLOT( incidenceChanged( IncidencePtr, QString ) ) );
  disconnect( subResource, SIGNAL( incidenceRemoved( QString, QString ) ),
              this, SLOT( incidenceRemoved( QString, QString ) ) );

  // block scope for BoolGuard
  {
    BoolGuard internalModification( mInternalCalendarModification, true );
    QMap<QString, QString>::iterator it = mUidToResourceMap.begin();
    while ( it != mUidToResourceMap.end() ) {
      if ( it.value() == subResource->subResourceIdentifier() ) {
        const QString uid = it.key();

        mChanges.remove( uid );
        mIdArbiter->removeArbitratedId( uid );

        Incidence *cachedIncidence = mCalendar.incidence( uid );
        if ( cachedIncidence != 0 ) {
          mCalendar.deleteIncidence( cachedIncidence );
        }

        delete cachedIncidence;

        it = mUidToResourceMap.erase( it );
      } else {
        ++it;
      }
    }
  }

  emit mParent->signalSubresourceRemoved( mParent, QLatin1String( "calendar" ), subResource->subResourceIdentifier() );

  emit mParent->resourceChanged( mParent );
}

void ResourceAkonadi::Private::loadingResult( bool ok, const QString &errorString )
{
  SharedResourcePrivate<SubResource>::loadingResult( ok, errorString );

  if ( ok ) {
    emit mParent->resourceLoaded( mParent );
  } else {
    mParent->loadError( errorString );
  }
}

void ResourceAkonadi::Private::savingResult( bool ok, const QString &errorString )
{
  SharedResourcePrivate<SubResource>::savingResult( ok, errorString );

  if ( ok ) {
    emit mParent->resourceSaved( mParent );
  } else {
    mParent->saveError( errorString );
  }
}

void ResourceAkonadi::Private::incidenceAdded( const IncidencePtr &incidencePtr, const QString &subResourceIdentifier )
{
  kDebug( 5800 ) << "Incidence (uid=" << incidencePtr->uid()
                 << ", summary=" << incidencePtr->summary()
                 << "), subResource=" << subResourceIdentifier;

  mChanges.remove( incidencePtr->uid() );

  // check if we already have it, i.e. if it is the result of us saving it
  if ( mCalendar.incidence( incidencePtr->uid() ) == 0 ) {
    Incidence *incidence = incidencePtr->clone();

    // block scope for BoolGuard
    {
      BoolGuard internalModification( mInternalCalendarModification, true );
      mCalendar.addIncidence( incidence );
    }

    mUidToResourceMap.insert( incidence->uid(), subResourceIdentifier );

    if ( !isLoading() ) {
      emit mParent->resourceChanged( mParent );
    }
  }
}

void ResourceAkonadi::Private::incidenceChanged( const IncidencePtr &incidencePtr, const QString &subResourceIdentifier )
{
  kDebug( 5800 ) << "Incidence (uid=" << incidencePtr->uid()
                 << ", summary=" << incidencePtr->summary()
                 << "), subResource=" << subResourceIdentifier;

  mChanges.remove( incidencePtr->uid() );

  Incidence *cachedIncidence = mCalendar.incidence( incidencePtr->uid() );
  if ( cachedIncidence == 0 ) {
    kWarning( 5800 ) << "Incidence" << incidencePtr->uid()
                     << "changed but no longer in local list";
    return;
  }

  // block scope for BoolGuard
  {
    BoolGuard internalModification( mInternalCalendarModification, true );
    // make sure any observer the resource might have installed gets properly notified
    cachedIncidence->startUpdates();
    bool assignResult = mIncidenceAssigner.assign( cachedIncidence, incidencePtr.get() );
    if ( assignResult ) {
      cachedIncidence->updated();
    }
    cachedIncidence->endUpdates();

    if ( !assignResult ) {
      kWarning( 5800 ) << "Incidence (uid=" << cachedIncidence->uid()
                       << ", summary=" << cachedIncidence->summary()
                       << ") changed type. Replacing it.";

      mCalendar.deleteIncidence( cachedIncidence );
      delete cachedIncidence;
      mCalendar.addIncidence( incidencePtr->clone() );
    }
  }

  if ( !isLoading() ) {
    emit mParent->resourceChanged( mParent );
  }
}

void ResourceAkonadi::Private::incidenceRemoved( const QString &uid, const QString &subResourceIdentifier )
{
  kDebug( 5800 ) << "Incidence (uid=" << uid
                 << "), subResource=" << subResourceIdentifier;

  mUidToResourceMap.remove( uid );

  Incidence *cachedIncidence = mCalendar.incidence( uid );
  if ( cachedIncidence == 0 ) {
    kWarning() << "Incidence (uid=" << uid << ") no longer in local list";
    return;
  }

  // block scope for BoolGuard
  {
    BoolGuard internalModification( mInternalCalendarModification, true );
    mCalendar.deleteIncidence( cachedIncidence );
    delete cachedIncidence;
  }

  if ( !isLoading() ) {
    emit mParent->resourceChanged( mParent );
  }
}

#include "resourceakonadi_p.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
