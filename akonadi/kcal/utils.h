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
#ifndef AKONADI_KCAL_UTILS_H
#define AKONADI_KCAL_UTILS_H

#include "akonadi-kcal_export.h"

#include <KCal/Event>
#include <KCal/Incidence>
#include <KCal/Journal>
#include <KCal/Todo>

namespace Akonadi
{
  class Item;

  /**
   * returns the incidence from an akonadi item, or a null pointer if the item has no such payload
   */
  AKONADI_KCAL_EXPORT KCal::Incidence::Ptr incidence( const Akonadi::Item &item );

  /**
   * returns the event from an akonadi item, or a null pointer if the item has no such payload
   */
  AKONADI_KCAL_EXPORT KCal::Event::Ptr event( const Akonadi::Item &item );

 /**
  * returns the todo from an akonadi item, or a null pointer if the item has no such payload
  */
 AKONADI_KCAL_EXPORT KCal::Todo::Ptr todo( const Akonadi::Item &item );

 /**
  * returns the journal from an akonadi item, or a null pointer if the item has no such payload
  */
 AKONADI_KCAL_EXPORT KCal::Journal::Ptr journal( const Akonadi::Item &item );

 /**
  * returns whether an Akonadi item contains an incidence
  */
 AKONADI_KCAL_EXPORT bool hasIncidence( const Akonadi::Item &item );

 /**
  * returns whether an Akonadi item contains an event
  */
 AKONADI_KCAL_EXPORT bool hasEvent( const Akonadi::Item &item );

 /**
  * returns whether an Akonadi item contains a todo
  */
 AKONADI_KCAL_EXPORT bool hasTodo( const Akonadi::Item &item );

 /**
  * returns whether an Akonadi item contains a journal
  */
 AKONADI_KCAL_EXPORT bool hasJournal( const Akonadi::Item &item );

}

#endif
