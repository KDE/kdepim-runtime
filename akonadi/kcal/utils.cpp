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

#include <Akonadi/Item>

KCal::Incidence::Ptr Akonadi::incidence( const Akonadi::Item &item ) {
  return item.hasPayload<KCal::Incidence::Ptr>() ? item.payload<KCal::Incidence::Ptr>() : KCal::Incidence::Ptr();
}

KCal::Event::Ptr Akonadi::event( const Akonadi::Item &item ) {
  return item.hasPayload<KCal::Event::Ptr>() ? item.payload<KCal::Event::Ptr>() : KCal::Event::Ptr();
}

KCal::Todo::Ptr Akonadi::todo( const Akonadi::Item &item ) {
  return item.hasPayload<KCal::Todo::Ptr>() ? item.payload<KCal::Todo::Ptr>() : KCal::Todo::Ptr();
}

KCal::Journal::Ptr Akonadi::journal( const Akonadi::Item &item ) {
  return item.hasPayload<KCal::Journal::Ptr>() ? item.payload<KCal::Journal::Ptr>() : KCal::Journal::Ptr();
}
