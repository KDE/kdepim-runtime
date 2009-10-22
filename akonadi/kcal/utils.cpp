/*
  This file is part of KOrganizer.

  Copyright (C) 2009 KDAB (author: Frank Osterfeld <osterfeld@kde.org>)
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "utils.h"

#include <KCal/CalendarLocal>
#include <KCal/CalFilter>
#include <KCal/DndFactory>
#include <KCal/ICalDrag>
#include <KCal/VCalDrag>

#include <Akonadi/Item>
#include <Akonadi/Collection>
#include <Akonadi/CollectionDialog>

#include <KIconLoader>
#include <KUrl>

#include <QDrag>
#include <QMimeData>
#include <QPixmap>
#include <QUrl>

#include <boost/bind.hpp>\

#include <algorithm>
#include <memory>
#include <cassert>

using namespace boost;
using namespace KCal;
using namespace Akonadi;

static QLatin1String sEventType( "application/x-vnd.akonadi.calendar.event" );
static QLatin1String sTodoType( "application/x-vnd.akonadi.calendar.todo" );
static QLatin1String sJournalType( "application/x-vnd.akonadi.calendar.journal" );
static QLatin1String sFreeBusyType( "application/x-vnd.akonadi.calendar.freebusy" );

static QStringList kcalTypes() {
  QStringList l;
  l << sEventType << sTodoType << sJournalType << sFreeBusyType;
  return l;
}

Incidence::Ptr Akonadi::incidence( const Item &item ) {
  return item.hasPayload<Incidence::Ptr>() ? item.payload<Incidence::Ptr>() : Incidence::Ptr();
}

Event::Ptr Akonadi::event( const Item &item ) {
  return item.hasPayload<Event::Ptr>() ? item.payload<Event::Ptr>() : Event::Ptr();
}

QList<Event::Ptr> Akonadi::eventsFromItems( const Item::List &items ) {
  QList<Event::Ptr> events;
  Q_FOREACH ( const Item &item, items )
    if ( const Event::Ptr e = Akonadi::event( item ) )
      events.push_back( e );
  return events;
}

Todo::Ptr Akonadi::todo( const Item &item ) {
  return item.hasPayload<Todo::Ptr>() ? item.payload<Todo::Ptr>() : Todo::Ptr();
}

Journal::Ptr Akonadi::journal( const Item &item ) {
  return item.hasPayload<Journal::Ptr>() ? item.payload<Journal::Ptr>() : Journal::Ptr();
}

bool Akonadi::hasIncidence( const Item& item ) {
  return item.hasPayload<Incidence::Ptr>();
}

bool Akonadi::hasEvent( const Item& item ) {
  return item.hasPayload<Event::Ptr>();
}

bool Akonadi::hasTodo( const Item& item ) {
  return item.hasPayload<Todo::Ptr>();
}

bool Akonadi::hasJournal( const Item& item ) {
  return item.hasPayload<Journal::Ptr>();
}

QMimeData* Akonadi::createMimeData( const Item::List &items, const KDateTime::Spec &timeSpec ) {
  if ( items.isEmpty() )
    return 0;

  KCal::CalendarLocal cal( timeSpec );

  QList<QUrl> urls;
  int incidencesFound = 0;
  Q_FOREACH ( const Item &item, items ) {
    const KCal::Incidence::Ptr incidence( Akonadi::incidence( item ) );
    if ( !incidence )
      continue;
    ++incidencesFound;
    urls.push_back( item.url() );
    Incidence *i = incidence->clone();
    cal.addIncidence( i );
  }

  if ( incidencesFound == 0 )
    return 0;

  std::auto_ptr<QMimeData> mimeData( new QMimeData );

  mimeData->setUrls( urls );

  ICalDrag::populateMimeData( mimeData.get(), &cal );
  VCalDrag::populateMimeData( mimeData.get(), &cal );

  return mimeData.release();
}

QMimeData* Akonadi::createMimeData( const Item &item, const KDateTime::Spec &timeSpec )  {
  return createMimeData( Item::List() << item, timeSpec );
}

QDrag* Akonadi::createDrag( const Item &item, const KDateTime::Spec &timeSpec, QWidget* parent ) {
  return createDrag( Item::List() << item, timeSpec, parent );
}

static QByteArray findMostCommonType( const Item::List &items ) {
  QByteArray prev;
  if ( items.isEmpty() )
    return "Incidence";
  Q_FOREACH( const Item &item, items ) {
    if ( !Akonadi::hasIncidence( item ) )
      continue;
    const QByteArray type = Akonadi::incidence( item )->type();
    if ( !prev.isEmpty() && type != prev )
      return "Incidence";
    prev = type;
  }
  return prev;
}

QDrag* Akonadi::createDrag( const Item::List &items, const KDateTime::Spec &timeSpec, QWidget* parent ) {
  std::auto_ptr<QDrag> drag( new QDrag( parent ) );
  drag->setMimeData( Akonadi::createMimeData( items, timeSpec ) );

  const QByteArray common = findMostCommonType( items );
  if ( common == "Event" ) {
    drag->setPixmap( BarIcon( QLatin1String("view-calendar-day") ) );
  } else if ( common == "Todo" ) {
    drag->setPixmap( BarIcon( QLatin1String("view-calendar-tasks") ) );
  }

  return drag.release();
}

static bool itemMatches( const Item& item, const CalFilter* filter ) {
  assert( filter );
  Incidence::Ptr inc = Akonadi::incidence( item );
  if ( !inc )
    return false;
  return filter->filterIncidence( inc.get() );
}

Item::List Akonadi::applyCalFilter( const Item::List &items_, const CalFilter* filter ) {
  assert( filter );
  Item::List items( items_ );
  items.erase( std::remove_if( items.begin(), items.end(), !bind( itemMatches, _1, filter ) ), items.end() );
  return items;
}

bool Akonadi::isValidIncidenceItemUrl( const KUrl &url, const QStringList &supportedMimeTypes ) {
  if ( !url.isValid() )
    return false;
  if ( url.scheme() != QLatin1String("akonadi") )
    return false;
  return supportedMimeTypes.contains( url.queryItem( QLatin1String("type") ) );
}

bool Akonadi::isValidIncidenceItemUrl( const KUrl &url ) {
  return isValidIncidenceItemUrl( url, kcalTypes() );
}

static bool containsValidIncidenceItemUrl( const QList<QUrl>& urls ) {
  return std::find_if( urls.begin(), urls.end(), bind( Akonadi::isValidIncidenceItemUrl, _1 ) ) != urls.constEnd();
}

bool Akonadi::isValidTodoItemUrl( const KUrl &url ) {
  if ( !url.isValid() )
    return false;
  if ( url.scheme() != QLatin1String("akonadi") )
    return false;
  return url.queryItem( QLatin1String("type") ) == sTodoType;
}

bool Akonadi::canDecode( const QMimeData* md ) {
  Q_ASSERT( md );
  return containsValidIncidenceItemUrl( md->urls() ) || ICalDrag::canDecode( md ) || VCalDrag::canDecode( md );
}

QList<KUrl> Akonadi::incidenceItemUrls( const QMimeData* mimeData ) {
  QList<KUrl> urls;
  Q_FOREACH( const KUrl& i, mimeData->urls() )
    if ( isValidIncidenceItemUrl( i ) )
      urls.push_back( i );
  return urls;
}

QList<KUrl> Akonadi::todoItemUrls( const QMimeData* mimeData ) {
  QList<KUrl> urls;
  Q_FOREACH( const KUrl& i, mimeData->urls() )
    if ( isValidIncidenceItemUrl( i , QStringList() << sTodoType ) )
      urls.push_back( i );
  return urls;
}

bool Akonadi::mimeDataHasTodo( const QMimeData* mimeData ) {
  return !todoItemUrls( mimeData ).isEmpty() || !todos( mimeData, KDateTime::Spec() ).isEmpty();
}

QList<Todo::Ptr> Akonadi::todos( const QMimeData* mimeData, const KDateTime::Spec &spec ) {
  std::auto_ptr<KCal::Calendar> cal( KCal::DndFactory::createDropCalendar( mimeData, spec ) );
  if ( !cal.get() )
    return QList<Todo::Ptr>();
  QList<Todo::Ptr> todos;
  Q_FOREACH( Todo* const i, cal->todos() )
      todos.push_back( Todo::Ptr( i->clone() ) );
  return todos;
}

Akonadi::Collection Akonadi::selectCollection( QWidget *parent )
{
  Akonadi::CollectionDialog dlg( parent );
  dlg.setMimeTypeFilter( QStringList() << QLatin1String( "text/calendar" ) );
  if ( ! dlg.exec() ) {
    return Akonadi::Collection();
  }
  const Akonadi::Collection collection = dlg.selectedCollection();
  Q_ASSERT( collection.isValid() );
  return collection;
}
    