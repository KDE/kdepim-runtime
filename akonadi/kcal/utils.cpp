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

#include <KCal/CalFilter>

#include <Akonadi/Item>

#include <KUrl>

#include <boost/bind.hpp>\

#include <algorithm>
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
