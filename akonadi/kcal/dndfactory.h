/*
  This file is part of the kcal library.

  Copyright (c) 1998 Preston Brown <pbrown@kde.org>
  Copyright (c) 2001,2002,2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (c) 2008 Thomas Thrainer <tom_t@gmx.at>
  Copyright (c) 2010 Laurent Montel <montel@kde.org>

  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  Author: Sergio Martins <sergio@kdab.com>

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
  defines the DndFactory class.

  @author Preston Brown \<pbrown@kde.org\>
  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
*/

#ifndef AKONADI_KCAL_DNDFACTORY_H
#define AKONADI_KCAL_DNDFACTORY_H

#include "akonadi-kcal_next_export.h"

#include "calendaradaptor.h"

#include <KCalUtils/DndFactory>
#include <KCalCore/MemoryCalendar>

#include <Akonadi/Item>

#include <KDE/KDateTime>

class QDate;
class QTime;
class QDrag;
class QWidget;
class QDropEvent;
class QMimeData;

namespace KCalCore {

class Event;
class Todo;
class Incidence;
}

namespace Akonadi {

  class CalendarAdaptor;

/**
  @brief
  vCalendar/iCalendar Drag-and-Drop object factory.

  This class implements functions to create Drag and Drop objects used for
  Drag-and-Drop and Copy-and-Paste.
*/
class AKONADI_KCAL_NEXT_EXPORT DndFactory
{
  public:
    explicit DndFactory( const Akonadi::CalendarAdaptor::Ptr & ,
                         bool deleteCalendarHere = false );

    ~DndFactory();

    /**
      Create the calendar that is contained in the drop event's data.
     */
    KCalCore::MemoryCalendar::Ptr createDropCalendar( QDropEvent *de );

    /**
      Create the calendar that is contained in the mime data.
     */
    KCalCore::MemoryCalendar::Ptr createDropCalendar( const QMimeData *md );

     /**
      Create the calendar that is contained in the mime data.
     */
    static KCalCore::MemoryCalendar::Ptr createDropCalendar( const QMimeData *md, const KDateTime::Spec &timeSpec );

    /**
      Create the mime data for the whole calendar.
    */
    QMimeData *createMimeData();

    /**
      Create a drag object for the whole calendar.
    */
    QDrag *createDrag( QWidget *owner );

    /**
      Create the mime data for a single incidence.
    */
  QMimeData *createMimeData( const KCalCore::Incidence::Ptr &incidence );

    /**
      Create a drag object for a single incidence.
    */
  QDrag *createDrag( const KCalCore::Incidence::Ptr &incidence, QWidget *owner );

    /**
      Create Todo object from mime data.
    */
  KCalCore::Todo::Ptr createDropTodo( const QMimeData *md );

    /**
      Create Todo object from drop event.
    */
  KCalCore::Todo::Ptr createDropTodo( QDropEvent *de );

    /**
      Create Event object from mime data.
    */
  KCalCore::Event::Ptr createDropEvent( const QMimeData *md );

    /**
      Create Event object from drop event.
    */
  KCalCore::Event::Ptr createDropEvent( QDropEvent *de );

    /**
      Cut the incidence to the clipboard.
    */
    void cutIncidence( const Akonadi::Item & );

    /**
      Copy the incidence to clipboard/
    */
    bool copyIncidence( const Akonadi::Item & );

    /**
      Paste the event or todo and return a pointer to the new incidence pasted.
    */
    KCalCore::Incidence::Ptr pasteIncidence( const KDateTime &,
                                             const KCalUtils::DndFactory::PasteFlags & = KCalUtils::DndFactory::PasteFlags() );

  /** pastes and returns the incidences from the clipboard
      If no date and time are given, the incidences will be pasted at
      their original date/time */
    KCalCore::Incidence::List pasteIncidences( const KDateTime &,
                                               const KCalUtils::DndFactory::PasteFlags & = KCalUtils::DndFactory::PasteFlags() );

  /** cuts a list of incidences to the clipboard */
  bool cutIncidences( const Akonadi::Item::List &item );

  /** copies a list of incidences to the clipboard */
  bool copyIncidences( const Akonadi::Item::List &item );

  private:
    //@cond PRIVATE
    Q_DISABLE_COPY( DndFactory )
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
