/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "calendarresource.h"
#include "defaultreminderattribute.h"
#include "settings.h"

#include <libkgoogle/fetchlistjob.h>
#include <libkgoogle/reply.h>
#include <libkgoogle/request.h>
#include <libkgoogle/objects/event.h>
#include <libkgoogle/objects/calendar.h>
#include <libkgoogle/services/calendar.h>

#include <KLocalizedString>

#include <Akonadi/ItemModifyJob>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/CollectionModifyJob>

#include <KCalCore/Event>
#include <KCalCore/Calendar>
#include <KCalCore/Todo>

using namespace Akonadi;
using namespace KGoogle;

void CalendarResource::calendarsReceived( KJob *job )
{
  if ( job->error() ) {
    Q_EMIT status( Broken, i18n( "Failed to fetch calendars: %1", job->errorString() ) );
    cancelTask();
    return;
  }

  FetchListJob *fetchJob = dynamic_cast< FetchListJob * >( job );

  QStringList calendars = Settings::self()->calendars();

  QList< KGoogle::Object *> allData = fetchJob->items();
  Q_FOREACH ( KGoogle::Object * replyData, allData ) {

    Objects::Calendar *calendar = static_cast< Objects::Calendar * >( replyData );

    if ( !calendars.contains( calendar->uid() ) ) {
      continue;
    }

    Collection collection;
    collection.setRemoteId( calendar->uid() );
    collection.setParentCollection( m_collections.first() );
    collection.setContentMimeTypes( QStringList() << Event::eventMimeType() );
    collection.setName( calendar->title() );
    collection.setRights( Collection::CanCreateItem |
                          Collection::CanChangeItem |
                          Collection::CanDeleteItem );

    EntityDisplayAttribute *attr = new EntityDisplayAttribute;
    attr->setDisplayName( calendar->title() );
    attr->setIconName( "text-calendar" );
    collection.addAttribute( attr );

    DefaultReminderAttribute *reminderAttr =
      new DefaultReminderAttribute( calendar->defaultReminders() );

    collection.addAttribute( reminderAttr );

    m_collections.append( collection );

  }

  m_fetchedCalendars = true;

  if ( m_fetchedTaskLists && m_fetchedCalendars ) {
    collectionsRetrieved( m_collections );
    m_collections.clear();

    m_fetchedCalendars = false;
    m_fetchedTaskLists = false;
  }
}

void CalendarResource::eventReceived( KGoogle::Reply *reply )
{
  if ( reply->error() != OK ) {
    cancelTask();
    Q_EMIT status( Broken, i18n( "Failed to fetch event: %1", reply->errorString() ) );
    return;
  }

  QList< Object * > data = reply->replyData();
  if ( data.length() != 1 ) {
    kWarning() << "Server send " << data.length() << "items, which is not OK";
    cancelTask( i18n( "Expected a single item, server sent %1 items.", data.length() ) );
    return;
  }

  Item item = reply->request()->property( "Item" ).value<Item>();

  Objects::Event *event = static_cast< Objects::Event * >( data.first() );

  if ( event->useDefaultReminders() ) {
    Collection collection = item.parentCollection();
    DefaultReminderAttribute *attr =
      dynamic_cast< DefaultReminderAttribute * >( collection.attribute( "defaultReminders" ) );
    if ( attr ) {
      Alarm::List alarms = attr->alarms( event );
      Q_FOREACH ( Alarm::Ptr alarm, alarms ) {
        event->addAlarm( alarm );
      }
    }
  }

  item.setRemoteId( event->uid() );
  item.setRemoteRevision( event->etag() );
  item.setMimeType( Event::eventMimeType() );
  item.setPayload< Event::Ptr >( EventPtr( event ) );

  if ( event->deleted() ) {
    itemsRetrievedIncremental( Item::List(), Item::List() << item );
  } else {
    itemRetrieved( item );
  }
}

void CalendarResource::eventsReceived( KJob *job )
{
  if ( job->error() ) {
    cancelTask( i18n( "Failed to fetch events: %1", job->errorString() ) );
    return;
  }

  FetchListJob *fetchJob = dynamic_cast< FetchListJob * >( job );
  Collection collection = fetchJob->property( "collection" ).value< Collection >();
  DefaultReminderAttribute *attr =
    dynamic_cast< DefaultReminderAttribute * >( collection.attribute( "defaultReminders" ) );

  Item::List removed;
  Item::List changed;
  QMap< QString, Objects::Event * > recurrentEvents;

  QList< Object *> allData = fetchJob->items();
  Q_FOREACH ( Object * replyData, allData ) {

    Objects::Event *event = static_cast< Objects::Event * >( replyData );

    if ( event->useDefaultReminders() && attr ) {
      Alarm::List alarms = attr->alarms( event );
      Q_FOREACH ( Alarm::Ptr alarm, alarms ) {
        event->addAlarm( alarm );
      }
    }

    /* If this is a recurrent event then put it to map and continue with
     * next event. We will return to this later... */
    if ( event->recurs() && !event->deleted() ) {
      recurrentEvents.insert( event->uid(), event );
      continue;
    }

    /* If the event is deleted, but it has same ID as some of the recurrent
     * events stored in the map, then take the original recurrent event from the map
     * and set date of this particular instance as an exception date and continue.
     * We will process content of the map later */
    if ( event->deleted() && recurrentEvents.contains( event->uid() ) ) {
      Objects::Event *rEvent = recurrentEvents.value( event->uid() );

      rEvent->recurrence()->addExDate( event->dtStart().date() );
      continue;
    }

    Item item;
    item.setRemoteId( event->uid() );
    item.setRemoteRevision( event->etag() );
    item.setPayload< Event::Ptr >( EventPtr( event ) );
    item.setMimeType( Event::eventMimeType() );
    item.setParentCollection( collection );

    if ( event->deleted() ) {
      removed << item;
    } else {
      changed << item;
    }
  }

  /* Now process the recurrent events */
  Q_FOREACH ( Objects::Event * event, recurrentEvents.values() ) {

    Item item;
    item.setRemoteId( event->uid() );
    item.setRemoteRevision( event->etag() );
    item.setPayload< Event::Ptr >( EventPtr( event ) );
    item.setMimeType( Event::eventMimeType() );
    item.setParentCollection( collection );

    changed << item;
  }

  itemsRetrievedIncremental( changed, removed );

  collection.setRemoteRevision( QString::number( KDateTime::currentUtcDateTime().toTime_t() ) );
  CollectionModifyJob *modifyJob = new CollectionModifyJob( collection, this );
  modifyJob->setAutoDelete( true );
  modifyJob->start();
}

void CalendarResource::eventCreated( KGoogle::Reply *reply )
{
  if ( reply->error() != OK ) {
    cancelTask( i18n( "Failed to create a new event: %1", reply->errorString() ) );
    return;
  }

  QList< Object * > data = reply->replyData();
  if ( data.length() != 1 ) {
    kWarning() << "Server send " << data.length() << "items, which is not OK";
    cancelTask( i18n( "Expected a single item, server sent %1 items.", data.length() ) );
    return;
  }

  Objects::Event *event = static_cast< Objects::Event * >( data.first() );

  Item item = reply->request()->property( "Item" ).value<Item>();
  item.setPayload< Event::Ptr >( EventPtr( event ) );
  item.setRemoteId( event->uid() );
  item.setRemoteRevision( event->etag() );
  item.setMimeType( Event::eventMimeType() );
  item.setParentCollection( reply->request()->property( "Collection" ).value<Collection>() );

  changeCommitted( item );
}

void CalendarResource::eventUpdated( KGoogle::Reply *reply )
{
  if ( reply->error() != OK ) {
    cancelTask( i18n( "Failed to update an event: %1", reply->errorString() ) );
    return;
  }

  QList< Object * > data = reply->replyData();
  if ( data.length() != 1 ) {
    kWarning() << "Server send " << data.length() << "items, which is not OK";
    cancelTask( i18n( "Expected a single item, server sent %1 items.", data.length() ) );
    return;
  }

  Objects::Event *event = static_cast< Objects::Event * >( data.first() );

  Item item = reply->request()->property( "Item" ).value<Item>();
  item.setRemoteRevision( event->etag() );

  changeCommitted( item );
}

void CalendarResource::eventRemoved( KGoogle::Reply *reply )
{
  if ( reply->error() != NoContent ) {
    cancelTask( i18n( "Failed to delete event: %1", reply->errorString() ) );
    return;
  }

  Item item = reply->request()->property( "Item" ).value<Item>();
  changeCommitted( item );
}

void CalendarResource::eventMoved( KGoogle::Reply *reply )
{
  if ( reply->error() != OK ) {
    cancelTask( i18n( "Failed to move event: %1", reply->errorString() ) );
    return;
  }

  Item item = reply->request()->property( "Item" ).value<Item>();
  changeCommitted( item );
}

