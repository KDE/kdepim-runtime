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
#include <libakonadi/transactionjobs.h>

#include <kdebug.h>
#include <kconfiggroup.h>

#include <QHash>

using namespace Akonadi;
using namespace KCal;

typedef QHash<QString, Item> ItemHash;

class ResourceAkonadi::Private
{
  public:
    Private( ResourceAkonadi *parent )
      : mParent( parent ), mMonitor( new Monitor( parent ) ), mCalendar( "UTC" ),
        mLock( true )
    {
    }

  public:
    ResourceAkonadi *mParent;

    Monitor *mMonitor;

    CalendarLocal mCalendar;

    KABC::LockNull mLock;

    Collection mCollection;
    ItemHash mItems;

  public:
    void itemAdded( const Akonadi::Item& item, const Akonadi::Collection& collection );
    void itemChanged( const Akonadi::Item& item, const QStringList& partIdentifiers );
    void itemRemoved( const Akonadi::DataReference& reference );
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
  // TODO: reset?
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

  return false;
}

bool ResourceAkonadi::doSave( bool syncCache )
{
  kDebug(5800) << "syncCache=" << syncCache;

  return false;
}

bool ResourceAkonadi::doSave( bool syncCache, Incidence *incidence )
{
  Q_UNUSED( incidence );

  kDebug(5800) << "syncCache=" << syncCache;

  return false;
}

bool ResourceAkonadi::doOpen()
{
  if ( !d->mCollection.isValid() )
    return false;

  // TODO: probably check here if collection exists

  d->mMonitor->monitorCollection( d->mCollection );

  return true;
}

void ResourceAkonadi::doClose()
{
  d->mCalendar.close();
}

void ResourceAkonadi::loadResult( KJob *job )
{
  Q_UNUSED( job );
}

void ResourceAkonadi::saveResult( KJob *job )
{
  Q_UNUSED( job );
}

void ResourceAkonadi::init()
{
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
}

void ResourceAkonadi::Private::itemAdded( const Akonadi::Item& item,
                                          const Akonadi::Collection& collection )
{
  Q_UNUSED( item );
  Q_UNUSED( collection );
}

void ResourceAkonadi::Private::itemChanged( const Akonadi::Item& item,
                                            const QStringList& partIdentifiers )
{
  Q_UNUSED( item );
  Q_UNUSED( partIdentifiers );
}

void ResourceAkonadi::Private::itemRemoved( const Akonadi::DataReference& reference )
{
  Q_UNUSED( reference );
}

#include "resourceakonadi.moc"
