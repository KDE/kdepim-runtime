/*
    Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "birthdaysresource.h"
#include "settings.h"
#include "settingsadaptor.h"
#include "configdialog.h"

#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>

#include <kabc/addressee.h>
#include <kcal/event.h>

#include <KDebug>
#include <KLocale>
#include <KWindowSystem>

#include <boost/shared_ptr.hpp>

using namespace Akonadi;
using namespace KABC;
using namespace KCal;

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

BirthdaysResource::BirthdaysResource(const QString& id) :
  ResourceBase( id )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  setName( i18n( "Birthdays & Anniversaries" ) );

  Monitor *monitor = new Monitor( this );
  monitor->setMimeTypeMonitored( Addressee::mimeType() );
  monitor->itemFetchScope().fetchFullPayload();
  connect( monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)),
           SLOT(contactChanged(Akonadi::Item)) );
  connect( monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)),
           SLOT(contactChanged(Akonadi::Item)) );
  connect( monitor, SIGNAL(itemRemoved(Akonadi::Item)),
           SLOT(contactRemoved(Akonadi::Item)) );

  connect( this, SIGNAL(reloadConfiguration()), SLOT(doFullSearch()) );
}

BirthdaysResource::~BirthdaysResource()
{
}

void BirthdaysResource::configure( WId windowId )
{
  ConfigDialog dlg;
  if ( windowId )
    KWindowSystem::setMainWindow( &dlg, windowId );
  dlg.exec();
  doFullSearch();
}

void BirthdaysResource::retrieveCollections()
{
  Collection c;
  c.setParent( Collection::root() );
  c.setRemoteId( "akonadi_birthdays_resource" );
  c.setName( name() );
  c.setContentMimeTypes( QStringList() << "application/x-vnd.akonadi.calendar.event" );
  c.setRights( Collection::ReadOnly );
  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

void BirthdaysResource::retrieveItems(const Akonadi::Collection& collection)
{
  Q_UNUSED( collection );
  itemsRetrievedIncremental( mPendingItems, mDeletedItems );
  mPendingItems.clear();
  mDeletedItems.clear();
}

bool BirthdaysResource::retrieveItem(const Akonadi::Item& item, const QSet< QByteArray > &parts)
{
  Q_UNUSED( parts );
  qint64 contactId = item.remoteId().mid( 1 ).toLongLong();
  ItemFetchJob *job = new ItemFetchJob( Item( contactId ), this );
  job->fetchScope().fetchFullPayload();
  connect( job, SIGNAL(result(KJob*)), SLOT(contactRetrieved(KJob*)) );
  return true;
}

void BirthdaysResource::contactRetrieved(KJob* job)
{
  ItemFetchJob *fj = static_cast<ItemFetchJob*>( job );
  if ( job->error() ) {
    emit error( job->errorText() );
    cancelTask();
  } else if ( fj->items().count() != 1 ) {
    cancelTask();
  } else {
    KCal::Event *ev = 0;
    if ( currentItem().remoteId().startsWith( 'b' ) )
      ev = createBirthday( fj->items().first() );
    else if ( currentItem().remoteId().startsWith( 'a' ) )
      ev = createAnniversary( fj->items().first() );
    if ( !ev ) {
      cancelTask();
    } else {
      Item i( currentItem() );
      i.setPayload<IncidencePtr>( IncidencePtr( ev ) );
      itemRetrieved( i );
    }
  }
}

void BirthdaysResource::contactChanged( const Akonadi::Item& item )
{
  if ( !item.hasPayload<KABC::Addressee>() )
    return;

  KABC::Addressee contact = item.payload<KABC::Addressee>();

  if ( Settings::self()->filterOnCategories() ) {
    bool hasCategory = false;
    const QStringList categories = contact.categories();
    foreach ( const QString &cat, Settings::self()->filterCategories() ) {
      if ( categories.contains( cat ) ) {
        hasCategory = true;
        break;
      }
    }

    if ( !hasCategory )
      return;
  }

  Event *event = createBirthday( item );
  if ( event )
    addPendingEvent( event, QString::fromLatin1( "b%1" ).arg( item.id() ) );

  event = createAnniversary( item );
  if ( event )
    addPendingEvent( event, QString::fromLatin1( "a%1" ).arg( item.id() ) );
}

void BirthdaysResource::addPendingEvent( KCal::Event* event, const QString &remoteId )
{
  boost::shared_ptr<KCal::Incidence> evptr( event );
  Item i( "application/x-vnd.akonadi.calendar.event" );
  i.setRemoteId( remoteId );
  i.setPayload( evptr );
  mPendingItems << i; // TODO check if we have that item already
  synchronize();
}


void BirthdaysResource::contactRemoved( const Akonadi::Item& item )
{
  Item i( "application/x-vnd.akonadi.calendar.event" );
  i.setRemoteId( QString::fromLatin1( "b%1" ).arg( item.id() ) );
  mDeletedItems << i;
  i.setRemoteId( QString::fromLatin1( "a%1" ).arg( item.id() ) );
  mDeletedItems << i;
  synchronize();
}


void BirthdaysResource::doFullSearch()
{
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, this );
  connect( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)), SLOT(listContacts(Akonadi::Collection::List)) );
}

void BirthdaysResource::listContacts(const Akonadi::Collection::List &cols)
{
  foreach ( const Collection &col, cols ) {
    if ( !col.contentMimeTypes().contains( Addressee::mimeType() ) )
      continue;
    ItemFetchJob *job = new ItemFetchJob( col, this );
    job->fetchScope().fetchFullPayload();
    connect( job, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(createEvents(Akonadi::Item::List)) );
  }
}

void BirthdaysResource::createEvents(const Akonadi::Item::List &items)
{
  foreach ( const Item &item, items )
    contactChanged( item );
}

KCal::Event* BirthdaysResource::createBirthday(const Akonadi::Item& contactItem)
{
  if ( !contactItem.hasPayload<KABC::Addressee>() )
    return 0;
  KABC::Addressee contact = contactItem.payload<KABC::Addressee>();

  const QString name = contact.realName().isEmpty() ? contact.nickName() : contact.realName();
  if ( name.isEmpty() ) {
    kDebug() << "contact " << contact.uid() << contactItem.id() << " has no name, skipping.";
    return 0;
  }

  const QDate birthdate = contact.birthday().date();
  if ( birthdate.isValid() ) {
    const QString summary = i18n( "%1's birthday", name );

    Event *ev = createEvent( birthdate );
    ev->setUid( contact.uid() + "_KABC_Birthday" );

    ev->setCustomProperty( "KABC", "BIRTHDAY", "YES" );
    ev->setCustomProperty( "KABC", "UID-1", contact.uid() );
    ev->setCustomProperty( "KABC", "NAME-1", name );
    ev->setCustomProperty( "KABC", "EMAIL-1", contact.fullEmail() );
    ev->setSummary( summary );

    ev->setCategories( i18n( "Birthday" ) );
    return ev;
  }
  return 0;
}

KCal::Event* BirthdaysResource::createAnniversary(const Akonadi::Item& contactItem)
{
  if ( !contactItem.hasPayload<KABC::Addressee>() )
    return 0;
  KABC::Addressee contact = contactItem.payload<KABC::Addressee>();

  const QString name = contact.realName().isEmpty() ? contact.nickName() : contact.realName();
  if ( name.isEmpty() ) {
    kDebug() << "contact " << contact.uid() << contactItem.id() << " has no name, skipping.";
    return 0;
  }

  const QString anniversary_string = contact.custom( "KADDRESSBOOK", "X-Anniversary" );
  if ( anniversary_string.isEmpty() )
    return 0;
  const QDate anniversary = QDate::fromString( anniversary_string, Qt::ISODate );
  if ( anniversary.isValid() ) {
    const QString spouseName = contact.custom( "KADDRESSBOOK", "X-SpousesName" );

    QString summary;
    if ( !spouseName.isEmpty() ) {
      summary = i18nc( "insert names of both spouses",
                       "%1's & %2's anniversary", name, spouseName );
    } else {
      summary = i18nc( "only one spouse in addressbook, insert the name",
                       "%1's anniversary", name );
    }

    Event *event = createEvent( anniversary );
    event->setUid( contact.uid() + "_KABC_Anniversary" );
    event->setSummary( summary );

    event->setCustomProperty( "KABC", "UID-1", contact.uid() );
    event->setCustomProperty( "KABC", "NAME-1", name );
    event->setCustomProperty( "KABC", "EMAIL-1", contact.fullEmail() );
    event->setCustomProperty( "KABC", "ANNIVERSARY", "YES" );
    // insert category
    event->setCategories( i18n( "Anniversary" ) );
    return event;
  }
  return 0;
}

KCal::Event* BirthdaysResource::createEvent(const QDate& date)
{
  Event *event = new Event();
  event->setDtStart( KDateTime( date, KDateTime::ClockTime ) );
  event->setDtEnd( KDateTime( date, KDateTime::ClockTime ) );
  event->setHasEndDate( true );
  event->setAllDay( true );
  event->setTransparency( Event::Transparent );

  // Set the recurrence
  Recurrence *recurrence = event->recurrence();
  recurrence->setStartDateTime( KDateTime( date, KDateTime::ClockTime ) );
  recurrence->setYearly( 1 );
  if ( date.month() == 2 && date.day() == 29 )
    recurrence->addYearlyDay( 60 );

  // Set the alarm
  event->clearAlarms();
  if ( Settings::self()->enableAlarm() ) {
    Alarm *alarm = event->newAlarm();
    alarm->setType( Alarm::Display );
    alarm->setText( event->summary() );
    alarm->setTime( KDateTime( date, KDateTime::ClockTime ) );
    // N days before
    alarm->setStartOffset( Duration( -Settings::self()->alarmDays(), Duration::Days ) );
    alarm->setEnabled( true );
  }

  return event;
}


AKONADI_RESOURCE_MAIN( BirthdaysResource )

#include "birthdaysresource.moc"
