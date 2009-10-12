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
#include <KCal/ICalDrag>
#include <KCal/VCalDrag>

#include <Akonadi/Item>

#include <KIconLoader>
#include <KUrl>

#include <QDrag>
#include <QMimeData>
#include <QPixmap>

#include <boost/bind.hpp>\

#include <algorithm>
#include <memory>
#include <cassert>

using namespace boost;
using namespace KCal;
using namespace Akonadi;

Incidence::Ptr Akonadi::incidence( const Item &item ) {
  return item.hasPayload<Incidence::Ptr>() ? item.payload<Incidence::Ptr>() : Incidence::Ptr();
}

Event::Ptr Akonadi::event( const Item &item ) {
  return item.hasPayload<Event::Ptr>() ? item.payload<Event::Ptr>() : Event::Ptr();
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
