/*
    This file is part of libkcal.
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "resourceakonadi.h"

#include "kcal/calendarlocal.h"
#include "kabc/locknull.h"

#include <libakonadi/collection.h>
#include <libakonadi/monitor.h>
#include <libakonadi/item.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemdeletejob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/itemsync.h>

#include <kdebug.h>
#include <kconfiggroup.h>

#include <QHash>
#include <QTimer>

#include <boost/shared_ptr.hpp>

using namespace Akonadi;
using namespace KCal;

using namespace Akonadi;
using namespace KCal;

typedef boost::shared_ptr<Incidence> IncidencePtr;

typedef QHash<QString, Item> ItemHash;

// Will probably have to implement it ourselves, ItemSync is
// currently not exported by libakonadi and we use a local copy
// in ../libakonadi
class SaveSequence : public ItemSync
{
  public:
    SaveSequence( const Collection& collection, QObject *parent )
      : ItemSync( collection, parent )
    {
    }
};

class ResourceAkonadi::Private : public KCal::Calendar::CalendarObserver
{
  public:
    Private( ResourceAkonadi *parent )
      : mParent( parent ), mMonitor( new Monitor( parent ) ), mCalendar( "UTC" ),
        mLock( true ), mInternalDelete( false )
    {
        mCalendar.registerObserver( this );
    }

  public:
    ResourceAkonadi *mParent;

    Monitor *mMonitor;

    CalendarLocal mCalendar;

    KABC::LockNull mLock;

    Collection mCollection;
    ItemHash mItems;

    bool mInternalDelete;

    QTimer mAutoSaveOnDeleteTimer;

  public:
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QStringList &partIdentifiers );
    void itemRemoved( const Akonadi::DataReference &reference );

    void delayedAutoSaveOnDelete();

    KJob *createSaveSequence();

    // from the CalendarObserver interface
    virtual void calendarIncidenceDeleted( Incidence *incidence );
};

ResourceAkonadi::ResourceAkonadi()
  : ResourceCalendar(), d( new Private( this ) )
{
  init();
}

ResourceAkonadi::ResourceAkonadi( const KConfigGroup &group )
  : ResourceCalendar( group ), d( new Private( this ) )
{
  KUrl url = group.readEntry( "CollectionUrl", KUrl() );

  if ( !url.isValid() ) {
    // TODO error handling
  } else {
    d->mCollection = Collection::fromUrl( url );
  }

  init();
}

ResourceAkonadi::~ResourceAkonadi()
{
  delete d;
}

void ResourceAkonadi::writeConfig( KConfigGroup &group )
{
  ResourceCalendar::writeConfig( group );

  group.writeEntry( "CollectionUrl", d->mCollection.url() );
}

void ResourceAkonadi::setCollection( const Collection& collection )
{
  if ( collection == d->mCollection )
    return;

  if ( isOpen() ) {
    kError(5800) << "Trying to change collection while resource is open";
    return;
  }

  d->mCollection = collection;
}

Collection ResourceAkonadi::collection() const
{
  return d->mCollection;
}

KABC::Lock *ResourceAkonadi::lock()
{
  return &(d->mLock);
}

bool ResourceAkonadi::addEvent( Event *event )
{
  return d->mCalendar.addEvent( event );
}

bool ResourceAkonadi::deleteEvent( Event *event )
{
  return d->mCalendar.deleteEvent( event );
}

void ResourceAkonadi::deleteAllEvents()
{
  d->mCalendar.deleteAllEvents();
}

Event *ResourceAkonadi::event( const QString &uid )
{
  return d->mCalendar.event( uid );
}

Event::List ResourceAkonadi::rawEvents( EventSortField sortField,
                                        SortDirection sortDirection )
{
  return d->mCalendar.rawEvents( sortField, sortDirection );
}

Event::List ResourceAkonadi::rawEventsForDate( const QDate &date,
                                               const KDateTime::Spec &timespec,
                                               EventSortField sortField,
                                               SortDirection sortDirection )
{
  return d->mCalendar.rawEventsForDate( date, timespec, sortField, sortDirection );
}

Event::List ResourceAkonadi::rawEventsForDate( const KDateTime &date )
{
  return d->mCalendar.rawEventsForDate( date );
}

Event::List ResourceAkonadi::rawEvents( const QDate &start, const QDate &end,
                                        const KDateTime::Spec &timespec,
                                        bool inclusive )
{
  return d->mCalendar.rawEvents( start, end, timespec, inclusive );
}

bool ResourceAkonadi::addTodo( Todo *todo )
{
  return d->mCalendar.addTodo( todo );
}

bool ResourceAkonadi::deleteTodo( Todo *todo )
{
  return d->mCalendar.deleteTodo( todo );
}

void ResourceAkonadi::deleteAllTodos()
{
  d->mCalendar.deleteAllTodos();
}

Todo *ResourceAkonadi::todo( const QString &uid )
{
  return d->mCalendar.todo( uid );
}

Todo::List ResourceAkonadi::rawTodos( TodoSortField sortField,
                                      SortDirection sortDirection )
{
  return d->mCalendar.rawTodos( sortField, sortDirection );
}

Todo::List ResourceAkonadi::rawTodosForDate( const QDate &date )
{
  return d->mCalendar.rawTodosForDate( date );
}

bool ResourceAkonadi::addJournal( Journal *journal )
{
  return d->mCalendar.addJournal( journal );
}

bool ResourceAkonadi::deleteJournal( Journal *journal )
{
  return d->mCalendar.deleteJournal( journal );
}

void ResourceAkonadi::deleteAllJournals()
{
  d->mCalendar.deleteAllJournals();
}

Journal *ResourceAkonadi::journal( const QString &uid )
{
  return d->mCalendar.journal( uid );
}

Journal::List ResourceAkonadi::rawJournals( JournalSortField sortField,
                                            SortDirection sortDirection )
{
  return d->mCalendar.rawJournals( sortField, sortDirection );
}

Journal::List ResourceAkonadi::rawJournalsForDate( const QDate &date )
{
  return d->mCalendar.rawJournalsForDate( date );
}

Alarm::List ResourceAkonadi::alarms( const KDateTime &from, const KDateTime &to )
{
  return d->mCalendar.alarms( from, to );
}

Alarm::List ResourceAkonadi::alarmsTo( const KDateTime &to )
{
  return d->mCalendar.alarmsTo( to );
}

void ResourceAkonadi::setTimeSpec( const KDateTime::Spec &timeSpec )
{
  d->mCalendar.setTimeSpec( timeSpec );
}

KDateTime::Spec ResourceAkonadi::timeSpec() const
{
  return d->mCalendar.timeSpec();
}

void ResourceAkonadi::setTimeZoneId( const QString &timeZoneId )
{
  d->mCalendar.setTimeZoneId( timeZoneId );
}

QString ResourceAkonadi::timeZoneId() const
{
  return d->mCalendar.timeZoneId();
}

void ResourceAkonadi::shiftTimes( const KDateTime::Spec &oldSpec,
                                  const KDateTime::Spec &newSpec )
{
  d->mCalendar.shiftTimes( oldSpec, newSpec );
}

bool ResourceAkonadi::doLoad( bool syncCache )
{
  kDebug(5800) << "syncCache=" << syncCache;

  // clear local caches
  d->mInternalDelete = true;
  d->mCalendar.close();
  d->mItems.clear();
  d->mInternalDelete = false;

  ItemFetchJob *job = new ItemFetchJob( d->mCollection, this );
  job->fetchAllParts();

  if ( !job->exec() ) {
    emit resourceLoadError( this, job->errorString() );
    return false;
  }

  Item::List items = job->items();

  kDebug(5800) << "Item fetch produced" << items.count() << "items";

  foreach ( const Item& item, items ) {
    if ( item.hasPayload<IncidencePtr>() ) {
      IncidencePtr incidence = item.payload<IncidencePtr>();

      d->mCalendar.addIncidence( incidence->clone() );
      d->mItems.insert( incidence->uid(), item );
    }
  }

  emit resourceLoaded( this );
#if 0
  // TODO probably can do this async, but the API docs of ResourceCalendar::load()
  // says the contents need to be available when it returns
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( loadResult( KJob* ) ) );

  job->start();
#endif

  return true;
}

bool ResourceAkonadi::doSave( bool syncCache )
{
  kDebug(5800) << "syncCache=" << syncCache;

  KJob *job = d->createSaveSequence();
  if ( job == 0 )
    return false;

  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( saveResult( KJob* ) ) );

  job->start();

  return true;
}

bool ResourceAkonadi::doSave( bool syncCache, Incidence *incidence )
{
  kDebug(5800) << "syncCache=" << syncCache
               << ", incidence" << incidence->uid();

  KJob *job = 0;

  ItemHash::const_iterator itemIt = d->mItems.find( incidence->uid() );
  if ( itemIt == d->mItems.end() ) {
    kDebug(5800) << "No item yet, creating append job";

    Item item( "text/calendar" );
    item.setPayload<IncidencePtr>( IncidencePtr( incidence->clone() ) );

    DataReference reference = item.reference();
    reference.setRemoteId( incidence->uid() );
    item.setReference( reference );

    job = new ItemAppendJob( item, d->mCollection, this );
  } else {
    kDebug(5800) << "Item already exists, creating store job";
    Item item = itemIt.value();

    item.setPayload<IncidencePtr>( IncidencePtr( incidence->clone() ) );
    item.setRev( itemIt.value().rev() );
    item.setReference( DataReference( itemIt.value().reference().id(),
                                      itemIt.value().reference().remoteId() ) );

    ItemStoreJob *storeJob = new ItemStoreJob( item, this );
    storeJob->storePayload();
    storeJob->setCollection( d->mCollection );

    job = storeJob;
  }

  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( saveResult( KJob* ) ) );

  job->start();

  return true;
}

bool ResourceAkonadi::doOpen()
{
  if ( !d->mCollection.isValid() )
    return false;

  // TODO: probably check here if collection exists

  d->mMonitor->monitorCollection( d->mCollection );

  // activate reacting to changes
  d->mMonitor->blockSignals( false );

  return true;
}

void ResourceAkonadi::doClose()
{
  // deactivate reacting to changes
  d->mMonitor->blockSignals( true );

  // clear local caches
  d->mInternalDelete = true;
  d->mCalendar.close();
  d->mItems.clear();
  d->mInternalDelete = false;
}

void ResourceAkonadi::loadResult( KJob *job )
{
  kDebug(5800) << job->errorString();

  if ( job->error() != 0 ) {
    emit resourceLoadError( this, job->errorString() );
    return;
  }

  ItemFetchJob *fetchJob = dynamic_cast<ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  Item::List items = fetchJob->items();

  kDebug(5800) << "Item fetch produced" << items.count() << "items";

  foreach ( const Item& item, items ) {
    if ( item.hasPayload<IncidencePtr>() ) {
      IncidencePtr incidence = item.payload<IncidencePtr>();

      d->mCalendar.addIncidence( incidence->clone() );
      d->mItems.insert( incidence->uid(), item );
    }
  }

  emit resourceLoaded( this );
}

void ResourceAkonadi::saveResult( KJob *job )
{
  kDebug(5800) << job->errorString();

  if ( job->error() != 0 ) {
    emit resourceSaveError( this, job->errorString() );
  } else {
    emit resourceSaved( this );
  }
}

void ResourceAkonadi::init()
{
  // deactivate reacting to changes, will be enabled in doOpen()
  d->mMonitor->blockSignals( true );

  d->mMonitor->monitorMimeType( "text/calendar" );
  d->mMonitor->fetchAllParts();

  connect( d->mMonitor,
           SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
           this,
           SLOT( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );

  connect( d->mMonitor,
           SIGNAL( itemChanged( const Akonadi::Item&, const QStringList& ) ),
           this,
           SLOT( itemChanged( const Akonadi::Item&, const QStringList& ) ) );

  connect( d->mMonitor,
           SIGNAL( itemRemoved( const Akonadi::DataReference&) ),
           this,
           SLOT( itemRemoved( const Akonadi::DataReference& ) ) );

  connect( d->mMonitor,
           SIGNAL( itemRemoved( const Akonadi::DataReference&) ),
           this,
           SLOT( itemRemoved( const Akonadi::DataReference& ) ) );

  connect( &d->mAutoSaveOnDeleteTimer, SIGNAL( timeout() ),
           this, SLOT( delayedAutoSaveOnDelete() ) );
}

void ResourceAkonadi::Private::itemAdded( const Akonadi::Item &item,
                                          const Akonadi::Collection &collection )
{
  kDebug(5800);
  if ( collection != mCollection )
    return;

  if ( !item.hasPayload<IncidencePtr>() ) {
    kError(5800) << "Item does not have IncidencePtr payload";
    return;
  }

  IncidencePtr incidence = item.payload<IncidencePtr>();

  kDebug(5800) << "Incidence" << incidence->uid();

  mItems.insert( incidence->uid(), item );

  // might be the result of our own saving
  if ( mCalendar.incidence( incidence->uid() ) == 0 ) {
    mCalendar.addIncidence( incidence->clone() );

    emit mParent->resourceChanged( mParent );
  }
}

void ResourceAkonadi::Private::itemChanged( const Akonadi::Item &item,
                                            const QStringList &partIdentifiers )
{
  kDebug(5800) << partIdentifiers;

  // check if this is one of ours (should be, we are just monitoring our collection)
  ItemHash::iterator itemIt = mItems.find( item.reference().remoteId() );
  if ( itemIt == mItems.end() || !( itemIt.value() == item ) ) {
    kWarning(5800) << "No matching local item for item: id="
                   << item.reference().id() << ", remoteId="
                   << item.reference().remoteId();
    return;
  }

  itemIt.value() = item;

  if ( !partIdentifiers.contains( Akonadi::Item::PartBody ) ) {
    kDebug(5800) << "No update to the item body";
    // FIXME find out why payload updates do not contain PartBody?
    //return;
  }

  if ( !item.hasPayload<IncidencePtr>() ) {
    kError(5800) << "Item does not have IncidencePtr payload";
    return;
  }

  IncidencePtr incidence = item.payload<IncidencePtr>();

  kDebug(5800) << "Incidence" << incidence->uid();

  Incidence *cachedIncidence = mCalendar.incidence( incidence->uid() );
  if ( cachedIncidence == 0 ) {
    kWarning(5800) << "Incidence" << incidence->uid()
                   << "changed but no longer in local list";
    return;
  }

  mInternalDelete = true;
  mCalendar.deleteIncidence( cachedIncidence );
  delete cachedIncidence;
  mInternalDelete = false;

  mCalendar.addIncidence( incidence.get()->clone() );

  emit mParent->resourceChanged( mParent );
}

void ResourceAkonadi::Private::itemRemoved( const Akonadi::DataReference &reference )
{
  kDebug(5800);

  ItemHash::iterator itemIt = mItems.find( reference.remoteId() );
  if ( itemIt == mItems.end() )
    return;

  if ( itemIt.value().reference() != reference )
    return;

  mItems.erase( itemIt );

  Incidence *cachedIncidence = mCalendar.incidence( reference.remoteId() );

  // if it does not exist as an addressee we already removed it locally
  if ( cachedIncidence == 0 )
    return;

  kDebug(5800) << "Incidence" << cachedIncidence->uid();

  mInternalDelete = true;
  mCalendar.deleteIncidence( cachedIncidence );
  delete cachedIncidence;
  mInternalDelete = false;

  emit mParent->resourceChanged( mParent );
}

void ResourceAkonadi::Private::delayedAutoSaveOnDelete()
{
  kDebug(5800);

  mParent->doSave( false );
}

KJob *ResourceAkonadi::Private::createSaveSequence()
{
  kDebug(5800);
  mAutoSaveOnDeleteTimer.stop();

  Item::List items;

  Incidence::List incidences = mCalendar.rawIncidences();

  Incidence::List::const_iterator incidenceIt    = incidences.begin();
  Incidence::List::const_iterator incidenceEndIt = incidences.end();
  for ( ; incidenceIt != incidenceEndIt; ++incidenceIt ) {
    Incidence *incidence = *incidenceIt;
    ItemHash::const_iterator itemIt = mItems.find( incidence->uid() );

    if ( itemIt == mItems.end() ) {
      Item item( "text/calendar" );
      item.setPayload<IncidencePtr>( IncidencePtr( incidence->clone() ) );

      DataReference reference = item.reference();
      reference.setRemoteId( incidence->uid() );
      item.setReference( reference );

      items << item;
    } else {
      Item item = itemIt.value();
      item.setPayload<IncidencePtr>( IncidencePtr( incidence->clone() ) );

      items << item;
    }
  }

  SaveSequence *job = new SaveSequence( mCollection, mParent );
  job->setRemoteItems( items );

  return job;
}

void ResourceAkonadi::Private::calendarIncidenceDeleted( Incidence *incidence )
{
  if ( mInternalDelete )
    return;

  kDebug(5800) << incidence->uid();

  mItems.remove( incidence->uid() );

  mAutoSaveOnDeleteTimer.start( 5000 ); // FIXME: configurable if needed at all
}

#include "resourceakonadi.moc"
