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

#include <Akonadi/Item>

#include <KCal/Event>
#include <KCal/Incidence>
#include <KCal/Journal>
#include <KCal/Todo>

#include <KDateTime>

namespace KCal {
  class CalFilter;
}

class KUrl;

class QDrag;
class QMimeData;

namespace Akonadi
{

  /**
   * returns the incidence from an akonadi item, or a null pointer if the item has no such payload
   */
  AKONADI_KCAL_EXPORT KCal::Incidence::Ptr incidence( const Akonadi::Item &item );

  /**
   * returns the event from an akonadi item, or a null pointer if the item has no such payload
   */
  AKONADI_KCAL_EXPORT KCal::Event::Ptr event( const Akonadi::Item &item );

  /**
   * returns event pointers from an akonadi item, or a null pointer if the item has no such payload
   */
  AKONADI_KCAL_EXPORT QList<KCal::Event::Ptr> eventsFromItems( const Akonadi::Item::List &items );

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

 /**
  * returns @p true iff the URL represents an Akonadi item and has one of the given mimetypes.
  */
 AKONADI_KCAL_EXPORT bool isValidIncidenceItemUrl( const KUrl &url, const QStringList &supportedMimeTypes );

 AKONADI_KCAL_EXPORT bool isValidIncidenceItemUrl( const KUrl &url );

 /**
  * returns @p true iff the mime data object contains any of the following:
  *
  * * An akonadi item with a supported KCal mimetype
  * * an iCalendar
  * * a VCard
  */
 AKONADI_KCAL_EXPORT bool canDecode( const QMimeData* mimeData );

 AKONADI_KCAL_EXPORT QList<KUrl> incidenceItemUrls( const QMimeData* mimeData );

 AKONADI_KCAL_EXPORT QList<KUrl> todoItemUrls( const QMimeData* mimeData );

 AKONADI_KCAL_EXPORT bool mimeDataHasTodo( const QMimeData* mimeData );

 AKONADI_KCAL_EXPORT QList<KCal::Todo::Ptr> todos( const QMimeData* mimeData, const KDateTime::Spec &timeSpec );

 /**
  * returns @p true iff the URL represents an Akonadi item and has one of the given mimetypes.
  */
 AKONADI_KCAL_EXPORT bool isValidTodoItemUrl( const KUrl &url );

 /**
  * creates mime data object for dragging an akonadi item containing an incidence
  */
 AKONADI_KCAL_EXPORT QMimeData* createMimeData( const Akonadi::Item &item, const KDateTime::Spec &timeSpec );

 /**
  * creates mime data object for dragging akonadi items containing an incidence
  */
 AKONADI_KCAL_EXPORT QMimeData* createMimeData( const Akonadi::Item::List &items, const KDateTime::Spec &timeSpec );

 /**
  * creates a drag object for dragging an akonadi item containing an incidence
  */
 AKONADI_KCAL_EXPORT QDrag* createDrag( const Akonadi::Item &item, const KDateTime::Spec &timeSpec, QWidget* parent );

 /**
  * creates a drag object for dragging akonadi items containing an incidence
  */
 AKONADI_KCAL_EXPORT QDrag* createDrag( const Akonadi::Item::List &items, const KDateTime::Spec &timeSpec, QWidget* parent );

 /**
  * applies a filter to a list of items containing incidences. Items not containing incidences or not matching the filter are removed.
  * Helper method anologous to KCal::CalFilter::apply()
  * @see KCal::CalFilter::apply()
  * @param items the list of items to filter
  * @param filter the filter to apply to the list of items
  * @return the filtered list of items
  */
 AKONADI_KCAL_EXPORT Akonadi::Item::List applyCalFilter( const Akonadi::Item::List &items, const KCal::CalFilter* filter );

 /**
  * Shows a modal dialog that allows to select a collection.
  *
  * @param parent The optional parent of the modal dialog.
  * @return The select collection or an invalid collection if
  * there was no collection selected.
  */
 AKONADI_KCAL_EXPORT Akonadi::Collection selectCollection( QWidget *parent );

}

#endif
