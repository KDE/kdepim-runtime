/*
  This file is part of the kcal library.

  Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

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
/**
  @file
  This file is part of the API for handling calendar data and
  defines the CalendarNull class.

  @brief
  Represents a null calendar class; that is, a calendar which contains
  no information and provides no capabilities.

  @author Cornelius Schumacher \<schumacher@kde.org\>
*/
#include "calendarbase.h"
#include "calendarnull.h"

using namespace Akonadi;
using namespace KCal;
using namespace KOrg;

/**
  Private class that helps to provide binary compatibility between releases.
  @internal
*/
//@cond PRIVATE
class KOrg::CalendarNull::Private
{
};
static CalendarNull *mSelf = 0;
//@endcond

CalendarNull::CalendarNull( const KDateTime::Spec &timeSpec )
  : CalendarBase( timeSpec ),
    d( new KOrg::CalendarNull::Private )
{}

CalendarNull::CalendarNull( const QString &timeZoneId )
  : CalendarBase( timeZoneId ),
    d( new KOrg::CalendarNull::Private )
{}

CalendarNull::~CalendarNull()
{
  delete d;
}

CalendarNull *CalendarNull::self()
{
  if ( !mSelf ) {
    mSelf = new CalendarNull( KDateTime::UTC );
  }

  return mSelf;
}

void CalendarNull::close()
{
}

bool CalendarNull::save()
{
  return true;
}

bool CalendarNull::reload()
{
  return true;
}


bool CalendarNull::addEvent( const Event::Ptr &event )
{
  Q_UNUSED ( event );
  return false;
}


bool CalendarNull::deleteEvent( const Item &event )
{
  Q_UNUSED( event );
  return false;
}

void CalendarNull::deleteAllEvents()
{
}

Item::List CalendarNull::rawEvents( KOrg::EventSortField sortField,
                                     KOrg::SortDirection sortDirection )
{
  Q_UNUSED( sortField );
  Q_UNUSED( sortDirection );
  return Item::List();
}

Item::List CalendarNull::rawEvents( const QDate &start, const QDate &end,
                                     const KDateTime::Spec &timeSpec,
                                     bool inclusive )
{
  Q_UNUSED( start );
  Q_UNUSED( end );
  Q_UNUSED( timeSpec );
  Q_UNUSED( inclusive );
  return Item::List();
}

Item::List CalendarNull::rawEventsForDate( const QDate &date,
                                            const KDateTime::Spec &timeSpec,
                                            KOrg::EventSortField sortField,
                                            KOrg::SortDirection sortDirection )
{
  Q_UNUSED( date );
  Q_UNUSED( timeSpec );
  Q_UNUSED( sortField );
  Q_UNUSED( sortDirection );
  return Item::List();
}

Item::List CalendarNull::rawEventsForDate( const KDateTime &dt )
{
  Q_UNUSED( dt );
  return Item::List();
}

Item CalendarNull::event( const Item::Id &uid )
{
  Q_UNUSED( uid );
  return Item();
}

bool CalendarNull::addTodo( const Todo::Ptr &todo )
{
  Q_UNUSED( todo );
  return false;
}

bool CalendarNull::deleteTodo( const Item &todo )
{
  Q_UNUSED( todo );
  return false;
}

void CalendarNull::deleteAllTodos()
{
}

Item::List CalendarNull::rawTodos( KOrg::TodoSortField sortField,
                                   KOrg::SortDirection sortDirection )
{
  Q_UNUSED( sortField );
  Q_UNUSED( sortDirection );
  return Item::List();
}

Item::List CalendarNull::rawTodosForDate( const QDate &date )
{
  Q_UNUSED( date );
  return Item::List();
}

Item CalendarNull::todo( const Item::Id &uid )
{
  Q_UNUSED( uid );
  return Item();
}

bool CalendarNull::addJournal( const Journal::Ptr &journal )
{
  Q_UNUSED( journal );
  return false;
}

bool CalendarNull::deleteJournal( const Item &journal )
{
  Q_UNUSED( journal );
  return false;
}

void CalendarNull::deleteAllJournals()
{
}

Item::List CalendarNull::rawJournals( KOrg::JournalSortField sortField,
                                         KOrg::SortDirection sortDirection )
{
  Q_UNUSED( sortField );
  Q_UNUSED( sortDirection );
  return Item::List();
}

Item::List CalendarNull::rawJournalsForDate( const QDate &date )
{
  Q_UNUSED( date );
  return Item::List();
}

Item CalendarNull::journal( const Item::Id &uid )
{
  Q_UNUSED( uid );
  return Item();
}

Alarm::List CalendarNull::alarms( const KDateTime &from, const KDateTime &to )
{
  Q_UNUSED( from );
  Q_UNUSED( to );
  return Alarm::List();
}

void CalendarNull::incidenceUpdated( IncidenceBase *incidenceBase )
{
  Q_UNUSED( incidenceBase );
}

Item CalendarNull::findParent( const Item & ) const {
  return Item();
}

Item::List CalendarNull::findChildren( const Item & ) const {
  return Item::List();
}

bool CalendarNull::isChild( const Item &, const Item & ) const {
  return false;
}
