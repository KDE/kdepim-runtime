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
#include "settings.h"

#include <libkgoogle/accessmanager.h>
#include <libkgoogle/auth.h>
#include <libkgoogle/fetchlistjob.h>
#include <libkgoogle/reply.h>
#include <libkgoogle/objects/task.h>
#include <libkgoogle/objects/tasklist.h>
#include <libkgoogle/services/tasks.h>

#include <KLocalizedString>
#include <KDebug>

#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/ItemFetchScope>


using namespace Akonadi;
using namespace KGoogle;
using namespace KCalCore;

void CalendarResource::taskDoUpdate( Reply *reply )
{
  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  Item item = reply->request()->property( "Item" ).value< Item >();
  Todo::Ptr todo = item.payload< TodoPtr >();
  Objects::Task ktodo( *todo );

  QUrl url = Services::Tasks::updateTaskUrl( item.parentCollection().remoteId(), item.remoteId() );

  Services::Tasks service;
  QByteArray data = service.objectToJSON( static_cast< KGoogle::Object *>( &ktodo ) );

  Request *request = new Request( url, Request::Update, "Tasks", account );
  request->setRequestData( data, "application/json" );
  request->setProperty( "Item", QVariant::fromValue( item ) );
  m_gam->sendRequest( request );
}


void CalendarResource::taskListReceived( KJob *job )
{
  if ( job->error() ) {
    cancelTask();
    Q_EMIT status( Broken, i18n( "Failed to fetch task lists" ) );
    return;
  }

  FetchListJob *fetchJob = dynamic_cast< FetchListJob * >( job );

  QStringList taskLists = Settings::self()->taskLists();

  QList< Object * > data = fetchJob->items();
  Q_FOREACH ( Object * replyData, data ) {

    Objects::TaskList *taskList = static_cast< Objects::TaskList * >( replyData );

    if ( !taskLists.contains( taskList->uid() ) )
      continue;

    Collection collection;
    collection.setRemoteId( taskList->uid() );
    collection.setParentCollection( m_collections.first() );
    collection.setContentMimeTypes( QStringList() << Todo::todoMimeType() );
    collection.setName( taskList->title() );
    collection.setRights( Collection::AllRights );

    EntityDisplayAttribute *attr = new EntityDisplayAttribute;
    attr->setDisplayName( taskList->title() );
    attr->setIconName( "text-calendar" );
    collection.addAttribute( attr );

    m_collections.append( collection );

  }

  m_fetchedTaskLists = true;

  if ( m_fetchedTaskLists && m_fetchedCalendars ) {
    collectionsRetrieved( m_collections );
    m_collections.clear();

    m_fetchedCalendars = false;
    m_fetchedTaskLists = false;
  }
}

void CalendarResource::taskReceived( KGoogle::Reply *reply )
{
  if ( reply->error() != OK ) {
    cancelTask( i18n( "Failed to fetch task: %1", reply->errorString() ) );
    return;
  }

  QList< Object * > data = reply->replyData();
  if ( data.length() != 1 ) {
    kWarning() << "Server send " << data.length() << "items, which is not OK";
    cancelTask( i18n( "Expected a single item, server sent %1 items.", data.length() ) );
    return;
  }

  Objects::Task *task = static_cast< Objects::Task * >( data.first() );

  Item item = reply->request()->property( "Item" ).value<Item>();
  item.setRemoteId( task->uid() );
  item.setRemoteRevision( task->etag() );
  item.setMimeType( Todo::todoMimeType() );
  item.setPayload< Todo::Ptr >( Todo::Ptr( task ) );

  if ( static_cast< Objects::Task * >( task )->deleted() ) {
    itemsRetrievedIncremental( Item::List(), Item::List() << item );
  } else {
    itemRetrieved( item );
  }
}

void CalendarResource::tasksReceived( KJob *job )
{
  if ( job->error() ) {
    cancelTask( i18n( "Failed to fetch tasks: %1", job->errorString() ) );
    return;
  }

  FetchListJob *fetchJob = dynamic_cast< FetchListJob * >( job );
  Collection collection = fetchJob->property( "collection" ).value< Collection >();

  Item::List removed;
  Item::List changed;

  QList< Object * > data = fetchJob->items();
  Q_FOREACH ( Object * replyData, data ) {

    Objects::Task *task = static_cast< Objects::Task * >( replyData );

    Item item;
    item.setRemoteId( task->uid() );
    item.setRemoteRevision( task->etag() );
    item.setPayload< Todo::Ptr >( Todo::Ptr( task ) );
    item.setMimeType( Todo::todoMimeType() );
    item.setParentCollection( collection );

    if ( task->deleted() ) {
      removed << item;
    } else {
      changed << item;
    }

  }

  itemsRetrievedIncremental( changed, removed );

  collection.setRemoteRevision( QString::number( KDateTime::currentUtcDateTime().toTime_t() ) );
  CollectionModifyJob *modifyJob = new CollectionModifyJob( collection, this );
  modifyJob->setAutoDelete( true );
  modifyJob->start();
}

void CalendarResource::taskCreated( KGoogle::Reply *reply )
{
  if ( reply->error() != OK ) {
    cancelTask( i18n( "Failed to create a task: %1", reply->errorString() ) );
    return;
  }

  QList< Object * > data = reply->replyData();
  if ( data.length() != 1 ) {
    kWarning() << "Server send " << data.length() << "items, which is not OK";
    cancelTask( i18n( "Expected a single item, server sent %1 items.",  data.length() ) );
    return;
  }

  Objects::Task *task = static_cast< Objects::Task * >( data.first() );

  Item item = reply->request()->property( "Item" ).value<Item>();
  item.setRemoteId( task->uid() );
  item.setRemoteRevision( task->etag() );
  item.setMimeType( Todo::todoMimeType() );
  item.setParentCollection( reply->request()->property( "Collection" ).value<Collection>() );

  changeCommitted( item );
}

void CalendarResource::taskUpdated( KGoogle::Reply *reply )
{
  if ( reply->error() != OK ) {
    cancelTask( i18n( "Failed to update task: %1", reply->errorString() ) );
    return;
  }

  QList< Object * > data = reply->replyData();
  if ( data.length() != 1 ) {
    kWarning() << "Server send " << data.length() << "items, which is not OK";
    cancelTask( i18n( "Expected a single item, server sent %1 items.", data.length() ) );
    return;
  }

  Objects::Task *task = static_cast< Objects::Task * >( data.first() );

  Item item = reply->request()->property( "Item" ).value<Item>();
  item.setRemoteRevision( task->etag() );

  changeCommitted( item );
}

void CalendarResource::removeTaskFetchJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( i18n( "Failed to delete task (1): %1", job->errorString() ) );
    return;
  }

  ItemFetchJob *fetchJob = dynamic_cast< ItemFetchJob * >( job );
  Item removedItem = fetchJob->property( "Item" ).value< Item >();

  Item::List detachItems;

  Item::List items = fetchJob->items();
  Q_FOREACH ( Item item, items ) { //krazy:exclude=foreach
    if( !item.hasPayload< Todo::Ptr >() ) {
      kDebug() << "Item " << item.remoteId() << " does not have Todo payload";
      continue;
    }

    Todo::Ptr todo = item.payload< Todo::Ptr >();
    /* If this item is child of the item we want to remove then add it to detach list */
    if ( todo->relatedTo( KCalCore::Incidence::RelTypeParent ) == removedItem.remoteId() ) {
      todo->setRelatedTo( QString(), KCalCore::Incidence::RelTypeParent );
      item.setPayload( todo );
      detachItems << item;
    }
  }

  /* If there are no items do detach, then delete the task right now */
  if ( detachItems.isEmpty() ) {
    doRemoveTask( job );
    return;
  }

  /* Send modify request to detach all the sub-tasks from the task that is about to be
   * removed. */

  ItemModifyJob *modifyJob = new ItemModifyJob( detachItems );
  modifyJob->setProperty( "Item", qVariantFromValue( removedItem ) );
  modifyJob->setAutoDelete( true );
  connect( modifyJob, SIGNAL( finished( KJob * ) ), this, SLOT( doRemoveTask( KJob * ) ) );
  modifyJob->start();
}

void CalendarResource::doRemoveTask( KJob *job )
{
  if ( job->error() ) {
    cancelTask( i18n( "Failed to delete task (2): %1", job->errorString() ) );
    return;
  }

  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  Item item = job->property( "Item" ).value< Item >();

  /* Now finally we can safely remove the task we wanted to */
  Request *request = new Request( Services::Tasks::removeTaskUrl( item.parentCollection().remoteId(), item.remoteId() ),
                                  KGoogle::Request::Remove, "Tasks", account );
  request->setProperty( "Item", qVariantFromValue( item ) );
  m_gam->sendRequest( request );
}

void CalendarResource::taskRemoved( KGoogle::Reply *reply )
{
  if ( reply->error() != NoContent ) {
    cancelTask( i18n( "Failed to delete task (5): %1", reply->errorString() ) );
    return;
  }

  Item item = reply->request()->property( "Item" ).value<Item>();
  changeCommitted( item );
}
