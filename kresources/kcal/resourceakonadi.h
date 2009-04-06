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

#ifndef KCAL_RESOURCEAKONADI_H
#define KCAL_RESOURCEAKONADI_H

#include "kcal/resourcecalendar.h"

#include "sharedresourceiface.h"

namespace Akonadi {
  class Collection;
  class Item;
}

class KJob;

class QModelIndex;

namespace KCal {

class ResourceAkonadi : public ResourceCalendar, public SharedResourceIface
{
  Q_OBJECT

  public:
    ResourceAkonadi();
    explicit ResourceAkonadi( const KConfigGroup &group );
    virtual ~ResourceAkonadi();

    virtual void writeConfig( KConfigGroup &group );

    void setStoreCollection( const Akonadi::Collection& collection );
    Akonadi::Collection storeCollection() const;

    virtual KABC::Lock *lock();

    virtual bool addEvent( Event *event );

    virtual bool deleteEvent( Event *event );

    virtual void deleteAllEvents();

    virtual Event *event( const QString &uid );

    virtual Event::List rawEvents(
      EventSortField sortField = EventSortUnsorted,
      SortDirection sortDirection = SortDirectionAscending );

    virtual Event::List rawEventsForDate(
      const QDate &date,
      const KDateTime::Spec &timespec = KDateTime::Spec(),
      EventSortField sortField = EventSortUnsorted,
      SortDirection sortDirection = SortDirectionAscending );

    virtual Event::List rawEventsForDate( const KDateTime &date );

    virtual Event::List rawEvents(
      const QDate &start, const QDate &end,
      const KDateTime::Spec &timespec = KDateTime::Spec(),
      bool inclusive = false );

    virtual bool addTodo( Todo *todo );

    virtual bool deleteTodo( Todo *todo );

    virtual void deleteAllTodos();

    virtual Todo *todo( const QString &uid );

    virtual Todo::List rawTodos(
      TodoSortField sortField = TodoSortUnsorted,
      SortDirection sortDirection = SortDirectionAscending );

    virtual Todo::List rawTodosForDate( const QDate &date );

    virtual bool addJournal( Journal *journal );

    virtual bool deleteJournal( Journal *journal );

    virtual void deleteAllJournals();

    virtual Journal *journal( const QString &uid );

    virtual Journal::List rawJournals(
      JournalSortField sortField = JournalSortUnsorted,
      SortDirection sortDirection = SortDirectionAscending );

    virtual Journal::List rawJournalsForDate( const QDate &date );

    virtual Alarm::List alarms( const KDateTime &from,
                                const KDateTime &to );

    virtual Alarm::List alarmsTo( const KDateTime &to );

    virtual void setTimeSpec( const KDateTime::Spec &timeSpec );

    virtual KDateTime::Spec timeSpec() const;

    virtual void setTimeZoneId( const QString &timeZoneId );

    virtual QString timeZoneId() const;

    virtual void shiftTimes( const KDateTime::Spec &oldSpec,
                             const KDateTime::Spec &newSpec );

    virtual bool canHaveSubresources() const;

    virtual QString labelForSubresource( const QString &resource ) const;

    virtual void setSubresourceActive( const QString &subResource, bool active );

    virtual bool subresourceActive( const QString &resource ) const;

    virtual QString subresourceIdentifier( Incidence *incidence );

    virtual QStringList subresources() const;

    virtual bool addSubresource( const QString &resource, const QString &parent );

    virtual bool removeSubresource( const QString &resource );

    virtual QString subresourceType( const QString &resource );

    virtual QString infoText() const;

  protected:
    virtual bool doLoad( bool syncCache );

    virtual bool doSave( bool syncCache );

    virtual bool doSave( bool syncCache, Incidence *incidence );

    virtual bool doOpen();

    virtual void doClose();

    using QObject::event;   // prevent warning about hidden virtual method

  private:
    //@cond PRIVATE
    Q_DISABLE_COPY( ResourceAkonadi )

    class Private;
    Private *const d;
    //@endcond
};

}

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
