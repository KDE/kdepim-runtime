/*
  This file is part of the kcal library.

  Copyright (c) 1998 Preston Brown <pbrown@kde.org>
  Copyright (c) 2001,2002 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>
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

  @brief
  vCalendar/iCalendar Drag-and-Drop object factory.

  @author Preston Brown \<pbrown@kde.org\>
  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
*/

#include "dndfactory.h"

#include <kcalutils/dndfactory.h>
#include <kcalutils/vcaldrag.h>
#include <kcalutils/icaldrag.h>

#include <kiconloader.h>
#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>

#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QDropEvent>
#include <QtGui/QPixmap>

using namespace KCalCore;
using namespace Akonadi;

/**
  Private class that helps to provide binary compatibility between releases.
  @internal
*/
//@cond PRIVATE
class Akonadi::DndFactory::Private
{
  public:
  Private( const Akonadi::CalendarAdaptor::Ptr &cal, bool deleteCalendar )
    : mDeleteCalendar( deleteCalendar ), mCalendar( cal ),
      mDndFactory( new KCalUtils::DndFactory( cal ) )
    {}

  ~Private() {
    delete mDndFactory;
  }

  bool mDeleteCalendar;
  CalendarAdaptor::Ptr mCalendar;
  KCalUtils::DndFactory *mDndFactory;

};
//@endcond
namespace Akonadi {
DndFactory::DndFactory( const Akonadi::CalendarAdaptor::Ptr &cal, bool deleteCalendarHere )
  : d( new Akonadi::DndFactory::Private ( cal, deleteCalendarHere ) )
{
}

DndFactory::~DndFactory()
{
  delete d;
}

QMimeData *DndFactory::createMimeData()
{
  return d->mDndFactory->createMimeData();
}

QDrag *DndFactory::createDrag( QWidget *owner )
{
  return d->mDndFactory->createDrag( owner );
}

QMimeData *DndFactory::createMimeData( const Incidence::Ptr &incidence )
{
  return d->mDndFactory->createMimeData( incidence );
}

QDrag *DndFactory::createDrag( const Incidence::Ptr &incidence, QWidget *owner )
{
  return d->mDndFactory->createDrag( incidence, owner );
}

KCalCore::MemoryCalendar::Ptr DndFactory::createDropCalendar( const QMimeData *md )
{
  return d->mDndFactory->createDropCalendar( md );
}

/* static */
KCalCore::MemoryCalendar::Ptr DndFactory::createDropCalendar( const QMimeData *md, const KDateTime::Spec &timeSpec )
{
 KCalCore::MemoryCalendar::Ptr cal( new KCalCore::MemoryCalendar( timeSpec ) );

 if ( KCalUtils::ICalDrag::fromMimeData( md, cal ) ||
      KCalUtils::VCalDrag::fromMimeData( md, cal ) ) {
   return cal;
 }

 return MemoryCalendar::Ptr();
}

KCalCore::MemoryCalendar::Ptr DndFactory::createDropCalendar( QDropEvent *de )
{
  return d->mDndFactory->createDropCalendar( de );
}

KCalCore::Event::Ptr DndFactory::createDropEvent( const QMimeData *md )
{
  return d->mDndFactory->createDropEvent( md );
}

KCalCore::Event::Ptr DndFactory::createDropEvent( QDropEvent *de )
{
  return d->mDndFactory->createDropEvent( de );
}

KCalCore::Todo::Ptr DndFactory::createDropTodo( const QMimeData *md )
{
  return d->mDndFactory->createDropTodo( md );
}

KCalCore::Todo::Ptr DndFactory::createDropTodo( QDropEvent *de )
{
  return d->mDndFactory->createDropTodo( de );
}

void DndFactory::cutIncidence( const Akonadi::Item &selectedInc )
{
  if ( copyIncidence( selectedInc ) ) {
    // Don't call the kcal's version, call deleteIncidence( Item, )
    // which creates a ItemDeleteJob.
    d->mCalendar->deleteIncidence( selectedInc, d->mDeleteCalendar );
  }
}

bool DndFactory::copyIncidence( const Akonadi::Item &item )
{
  if ( Akonadi::hasIncidence( item ) ) {
    return d->mDndFactory->copyIncidence( Akonadi::incidence( item ) );
  } else {
    return false;
  }
}

Incidence::Ptr DndFactory::pasteIncidence( const QDate &newDate, const QTime *newTime )
{
  return d->mDndFactory->pasteIncidence( newDate, newTime );
}

bool DndFactory::copyIncidences( const Item::List &items )
{
  Incidence::List incList;
  Q_FOREACH ( const Item &item, items ) {
    if ( Akonadi::hasIncidence( item ) ) {
      incList.append( Akonadi::incidence( item ) );
    }
  }

  return d->mDndFactory->copyIncidences( incList );
}

bool DndFactory::cutIncidences( const Item::List &items )
{
  if ( copyIncidences( items ) ) {
    Item::List::ConstIterator it;
    for ( it = items.constBegin(); it != items.constEnd(); ++it ) {
      // Don't call the kcal's version, call deleteIncidence( Item, )
      // which creates a ItemDeleteJob.
      d->mCalendar->deleteIncidence( *it );
    }
    return true;
  } else {
    return false;
  }
}

KCalCore::Incidence::List DndFactory::pasteIncidences( const QDate &newDate,
                                                   const QTime *newTime )
{
  return d->mDndFactory->pasteIncidences( newDate, newTime );
}

} // namespace
