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
#include "settingsdialog.h"

#include <libkgoogle/common.h>
#include <libkgoogle/account.h>
#include <libkgoogle/accessmanager.h>
#include <libkgoogle/auth.h>
#include <libkgoogle/fetchlistjob.h>
#include <libkgoogle/request.h>
#include <libkgoogle/reply.h>
#include <libkgoogle/objects/calendar.h>
#include <libkgoogle/objects/event.h>
#include <libkgoogle/objects/task.h>
#include <libkgoogle/objects/tasklist.h>
#include <libkgoogle/services/calendar.h>
#include <libkgoogle/services/tasks.h>

#include <QtCore/QStringList>
#include <QtCore/QMetaType>

#include <KLocalizedString>
#include <KDialog>
#include <Akonadi/Attribute>
#include <Akonadi/AttributeFactory>
#include <Akonadi/CachePolicy>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <KCalCore/Calendar>

using namespace KCalCore;
using namespace Akonadi;
using namespace KGoogle;

CalendarResource::CalendarResource( const QString &id ):
  ResourceBase( id ),
  m_account( 0 ),
  m_fetchedCalendars( false ),
  m_fetchedTaskLists( false )
{
  qRegisterMetaType< KGoogle::Services::Calendar >( "Calendar" );
  qRegisterMetaType< KGoogle::Services::Tasks >( "Tasks" );
  AttributeFactory::registerAttribute< DefaultReminderAttribute >();

  Auth *auth = Auth::instance();
  auth->init( "Akonadi Google", Settings::self()->clientId(), Settings::self()->clientSecret() );

  setNeedsNetwork( true );
  setOnline( true );

  m_gam = new AccessManager();
  connect( m_gam, SIGNAL(error(KGoogle::Error,QString)),
           this, SLOT(error(KGoogle::Error,QString)) );
  connect( m_gam, SIGNAL(replyReceived(KGoogle::Reply*)),
           this, SLOT(replyReceived(KGoogle::Reply*)) );

  connect( this, SIGNAL(abortRequested()),
           this, SLOT(slotAbortRequested()) );
  connect( this, SIGNAL(reloadConfiguration()),
           this, SLOT(reloadConfig()) );

  changeRecorder()->itemFetchScope().fetchFullPayload( true );
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::All );
  changeRecorder()->fetchCollection( true );
  changeRecorder()->collectionFetchScope().setAncestorRetrieval( CollectionFetchScope::All );

  if ( !Settings::self()->account().isEmpty() ) {
    if ( !getAccount().isNull() ) {
      synchronize();
    }
  }
}

CalendarResource::~CalendarResource()
{
  delete m_gam;
}

void CalendarResource::aboutToQuit()
{
  slotAbortRequested();
}

void CalendarResource::abort()
{
  cancelTask( i18n( "Aborted" ) );
}

void CalendarResource::slotAbortRequested()
{
  abort();
}

void CalendarResource::error( const KGoogle::Error errCode, const QString &msg )
{
  cancelTask( msg );

  if ( ( errCode == AuthError ) || ( errCode == BackendNotReady ) ) {
    status( Broken, msg );
  }
}

void CalendarResource::configure( WId windowId )
{
  SettingsDialog *settingsDialog = new SettingsDialog( windowId );

  if ( settingsDialog->exec() == KDialog::Accepted ) {
    Q_EMIT configurationDialogAccepted();

    delete settingsDialog;

    if( !getAccount().isNull() ) {
      synchronize();
    }

  } else {
    Q_EMIT configurationDialogRejected();

    delete settingsDialog;
  }
}

void CalendarResource::reloadConfig()
{
  if ( getAccount().isNull() ) {
    return;
  }

  synchronize();
}

Account::Ptr CalendarResource::getAccount()
{
  if ( !m_account.isNull() )
    return m_account;

  Auth *auth = Auth::instance();
  try {
    m_account = auth->getAccount( Settings::self()->account() );
  } catch( KGoogle::Exception::BaseException &e ) {
    Q_EMIT status( Broken, e.what() );
    return Account::Ptr();
  }

  return m_account;
}

void CalendarResource::retrieveItems( const Akonadi::Collection &collection )
{
  /* Do not initiate item-retrieval for the root collection as this
   * collection is only used to authenticate agains google and to hold
   * all the calendars associated with this account. There are no items
   * to be fetched from this collection! */
  if ( collection.parentCollection() != Akonadi::Collection::root() ) {
    ItemFetchJob *fetchJob = new ItemFetchJob( collection, this );
    connect( fetchJob, SIGNAL(finished(KJob*)),
             this, SLOT(cachedItemsRetrieved(KJob*)) );
    connect( fetchJob, SIGNAL(finished(KJob*)),
             fetchJob, SLOT(deleteLater()) );

    fetchJob->fetchScope().fetchFullPayload( false );
    fetchJob->setProperty( "collection", qVariantFromValue( collection ) );
    fetchJob->start();

    Q_EMIT percent( 0 );
  } else {
    itemsRetrievalDone();
  }
}

void CalendarResource::cachedItemsRetrieved( KJob *job )
{
  QUrl url;
  QString service;
  QString lastSync;

  Collection collection = job->property( "collection" ).value<Collection>();

  if ( collection.contentMimeTypes().contains( Event::eventMimeType() ) ) {

    service = "Calendar";
    url = Services::Calendar::fetchEventsUrl( collection.remoteId() );

  } else if ( collection.contentMimeTypes().contains( Todo::todoMimeType() ) ) {

    service = "Tasks";
    url = Services::Tasks::fetchAllTasksUrl( collection.remoteId() );

  } else {

    Q_EMIT cancelTask( i18n( "Invalid collection" ) );
    return;

  }

  lastSync = collection.remoteRevision();
  if ( !lastSync.isEmpty() ) {
    KDateTime dt;
    dt.setTime_t( lastSync.toInt() );
    lastSync = AccessManager::dateToRFC3339String( dt );

    url.addQueryItem( "updatedMin", lastSync );
  }

  url.addQueryItem( "showDeleted", "true" );

  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  FetchListJob *fetchJob = new FetchListJob( url, service, account->accountName() );
  fetchJob->setProperty( "collection", qVariantFromValue( collection ) );
  connect( fetchJob, SIGNAL(finished(KJob*)),
           this, SLOT(itemsReceived(KJob*)) );
  connect( fetchJob, SIGNAL(percent(KJob*,ulong)),
           this, SLOT(emitPercent(KJob*,ulong)) );
  fetchJob->start();
}


bool CalendarResource::retrieveItem( const Akonadi::Item &item, const QSet< QByteArray >& parts )
{
  Q_UNUSED( parts );

  QString service;
  QUrl url;

  if ( item.parentCollection().contentMimeTypes().contains( Event::eventMimeType() ) ) {

    service = "Calendar";
    url = Services::Calendar::fetchEventUrl( item.parentCollection().remoteId(), item.remoteId() );

  } else if ( item.parentCollection().contentMimeTypes().contains( Todo::todoMimeType() ) ) {

    service = "Tasks";
    url = Services::Tasks::fetchTaskUrl( item.parentCollection().remoteId(), item.remoteId() );

  }

  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return true;
  }

  Request *request = new Request( url, KGoogle::Request::Fetch, service, account );
  request->setProperty( "Item", QVariant::fromValue( item ) );
  m_gam->sendRequest( request );

  return true;
}

void CalendarResource::retrieveCollections()
{
  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  Akonadi::EntityDisplayAttribute *attr = new Akonadi::EntityDisplayAttribute();
  attr->setDisplayName( account->accountName() );

  Collection collection;
  collection.setName( identifier() );
  collection.setRemoteId( identifier() );
  collection.setParentCollection( Akonadi::Collection::root() );
  collection.setContentMimeTypes( QStringList() << Collection::mimeType()
                                  << Event::eventMimeType()
                                  << Todo::todoMimeType() );
  collection.addAttribute( attr );
  collection.setRights( Collection::ReadOnly );

  m_collections.clear();
  m_collections.append( collection );

  FetchListJob *fetchJob;

  fetchJob = new FetchListJob( Services::Calendar::fetchCalendarsUrl(), "Calendar", account->accountName() );
  connect( fetchJob, SIGNAL(finished(KJob*)),
           this, SLOT(calendarsReceived(KJob*)) );
  fetchJob->start();

  fetchJob = new FetchListJob( Services::Tasks::fetchTaskListsUrl(), "Tasks", account->accountName() );
  connect( fetchJob, SIGNAL(finished(KJob*)),
           this, SLOT(taskListReceived(KJob*)) );
  fetchJob->start();
}

void CalendarResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  QString service;
  QUrl url;
  QByteArray data;

  if ( collection.parentCollection() == Akonadi::Collection::root() ) {
    cancelTask( i18n( "The top-level collection cannot contain any tasks or events" ) );
    return;
  }

  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  if ( item.mimeType() == Event::eventMimeType() ) {

    Event::Ptr event = item.payload< Event::Ptr >();
    Objects::Event kevent( *event );

    service = "Calendar";
    url = Services::Calendar::createEventUrl( collection.remoteId() );

    Services::Calendar service;
    kevent.setUid( "" );
    data = service.objectToJSON( static_cast< KGoogle::Object * >( &kevent ) );

  } else if ( item.mimeType() == Todo::todoMimeType() ) {

    Todo::Ptr todo = item.payload< Todo::Ptr >();
    todo->setUid( "" );
    Objects::Task ktodo( *todo );

    service = "Tasks";
    url = Services::Tasks::createTaskUrl( collection.remoteId() );
    if ( !todo->relatedTo( Incidence::RelTypeParent ).isEmpty() )
      url.addQueryItem( "parent", todo->relatedTo( Incidence::RelTypeParent ) );

    Services::Tasks service;
    data = service.objectToJSON( static_cast< KGoogle::Object * >( &ktodo ) );

  } else {
    cancelTask( i18n( "Unknown payload type '%1'" ).arg( item.mimeType() ) );
    return;
  }

  Request *request = new Request( url, Request::Create, service, account );
  request->setRequestData( data, "application/json" );
  request->setProperty( "Item", QVariant::fromValue( item ) );
  request->setProperty( "Collection", QVariant::fromValue( collection ) );

  m_gam->sendRequest( request );
}

void CalendarResource::itemChanged( const Akonadi::Item &item, const QSet< QByteArray >& partIdentifiers )
{
  QUrl url;
  QByteArray data;

  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  if ( item.mimeType() == Event::eventMimeType() ) {

    Event::Ptr event = item.payload< Event::Ptr >();
    Objects::Event kevent( *event );

    /* Akonadi stores the event with it's own UID when it's created,
     * but we need to make it same as remoteId, which is the event's
     * original UID on Google.
     *
     * We could update the UID of newly created event after receiving it's
     * proper remoteId (and thus Google's UID), but KOrganizer does not cope
     * well with it (see bug #298518). */
    kevent.setUid( item.remoteId() );

    url = Services::Calendar::updateEventUrl( item.parentCollection().remoteId(), item.remoteId() );

    Services::Calendar service;
    data = service.objectToJSON( static_cast< KGoogle::Object * >( &kevent ) );

    Request *request = new Request( url, Request::Patch, "Calendar", account );
    request->setRequestData( data, "application/json" );
    request->setProperty( "Item", QVariant::fromValue( item ) );

    m_gam->sendRequest( request );

  } else if ( item.mimeType() == Todo::todoMimeType() ) {

    Todo::Ptr todo = item.payload< Todo::Ptr >();
    Objects::Task ktodo( *todo );

    /* See the comment above for explanation why we do this here */
    ktodo.setUid( item.remoteId() );

    QString parentUid = todo->relatedTo( KCalCore::Incidence::RelTypeParent );
    QUrl moveUrl = Services::Tasks::moveTaskUrl( item.parentCollection().remoteId(),
                   todo->uid(),
                   parentUid );
    Request *request = new Request( moveUrl, Request::Move, "Tasks", account );
    request->setProperty( "Item", QVariant::fromValue( item ) );

    m_gam->sendRequest( request );

  } else {
    cancelTask( i18n( "Unknown payload type '%1'" ).arg( item.mimeType() ) );
    return;
  }

  Q_UNUSED( partIdentifiers );
}

void CalendarResource::itemRemoved( const Akonadi::Item &item )
{
  QString service;
  QUrl url;


  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  if ( item.mimeType() == Event::eventMimeType() ) {

    url = Services::Calendar::removeEventUrl( item.parentCollection().remoteId(), item.remoteId() );
    Request *request = new Request( url, Request::Remove, "Calendar", account );
    request->setProperty( "Item", QVariant::fromValue( item ) );
    m_gam->sendRequest( request );

  } else if ( item.mimeType() == Todo::todoMimeType() ) {

    /* Google always automatically removes tasks with all their subtasks. In KOrganizer
     * by default we only remove the item we are given. For this reason we have to first
     * fetch all tasks, find all sub-tasks for the task being removed and detach them
     * from the task. Only then the task can be safely removed. */

    ItemFetchJob *fetchJob = new ItemFetchJob( item.parentCollection() );
    fetchJob->setAutoDelete( true );
    fetchJob->fetchScope().fetchFullPayload( true );
    fetchJob->setProperty( "Item", qVariantFromValue( item ) );
    connect( fetchJob, SIGNAL(finished(KJob*)),
             this, SLOT(removeTaskFetchJobFinished(KJob*)) );
    fetchJob->start();

  } else {
    cancelTask( i18n( "Unknown payload type '%1'" ).arg( item.mimeType() ) );
    return;
  }

}

void CalendarResource::itemMoved( const Item &item, const Collection &collectionSource, const Collection &collectionDestination )
{
  QString service;
  QUrl url;

  if ( collectionDestination.parentCollection() == Akonadi::Collection::root() ) {
    cancelTask( i18n( "The top-level collection cannot contain any tasks or events" ) );
    return;
  }

  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  /* Moving tasks between task lists is not supported */
  if ( item.mimeType() != Event::eventMimeType() )
    return;

  url = Services::Calendar::moveEventUrl( collectionSource.remoteId(), collectionDestination.remoteId(), item.remoteId() );
  Request *request = new Request( url, KGoogle::Request::Move, "Calendar", account );
  request->setProperty( "Item", qVariantFromValue( item ) );

  m_gam->sendRequest( request );
}


void CalendarResource::replyReceived( KGoogle::Reply *reply )
{
  switch ( reply->requestType() ) {
    case Request::FetchAll:
      /* Handled by FetchListJob */
      break;

    case Request::Fetch:
      itemReceived( reply );
      break;

    case Request::Create:
      itemCreated( reply );
      break;

    case Request::Update:
    case Request::Patch:
      itemUpdated( reply );
      break;

    case Request::Remove:
      itemRemoved( reply );
      break;

    case Request::Move:
      itemMoved( reply );
      break;
  }
}

void CalendarResource::itemMoved( Reply *reply )
{
  if ( reply->serviceName() == "Calendar" ) {

    eventMoved( reply );

  } else if ( reply->serviceName() == "Tasks" ) {

    taskDoUpdate( reply );

  } else {

    cancelTask( i18n( "Received invalid reply" ) );

  }
}

void CalendarResource::itemsReceived( KJob *job )
{
  FetchListJob *fetchJob = dynamic_cast< FetchListJob * >( job );

  if ( fetchJob->service() == "Calendar" ) {

    eventsReceived( job );

  } else if ( fetchJob->service() == "Tasks" ) {

    tasksReceived( job );

  } else {

    cancelTask( i18n( "Received invalid reply" ) );

  }
}

void CalendarResource::itemReceived( Reply *reply )
{
  if ( reply->serviceName() == "Calendar" ) {

    eventReceived( reply );

  } else if ( reply->serviceName() == "Tasks" ) {

    taskReceived( reply );

  } else {

    cancelTask( i18n( "Received an invalid reply" ) );

  }

  delete reply;

}

void CalendarResource::itemCreated( Reply *reply )
{
  if ( reply->serviceName() == "Calendar" ) {

    eventCreated( reply );

  } else if ( reply->serviceName() == "Tasks" ) {

    taskCreated( reply );

  } else {

    cancelTask( i18n( "Received an invalid reply" ) );

  }

  delete reply;

}

void CalendarResource::itemUpdated( Reply *reply )
{
  if ( reply->serviceName() == "Calendar" ) {

    eventUpdated( reply );

  } else if ( reply->serviceName() == "Tasks" ) {

    taskUpdated( reply );

  } else {

    cancelTask( i18n( "Received an invalid reply" ) );

  }

  delete reply;

}

void CalendarResource::itemRemoved( Reply *reply )
{
  if ( reply->serviceName() == "Calendar" ) {

    eventRemoved( reply );

  } else if ( reply->serviceName() == "Tasks" ) {

    taskRemoved( reply );

  } else {

    cancelTask( i18n( "Received an invalid reply" ) );

  }

  delete reply;
}

void CalendarResource::emitPercent( KJob *job, ulong progress )
{
  Q_UNUSED( job );

  Q_EMIT percent( progress );
}


AKONADI_RESOURCE_MAIN( CalendarResource );
