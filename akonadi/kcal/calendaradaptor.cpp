/*
  Copyright (C) 2010 Kevin Ottens <ervin@kde.org>

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

#include "kcalprefs.h"
#include "calendaradaptor.h"

using namespace Akonadi;
using namespace KCal;

template<class T> inline T* itemToIncidence(const Akonadi::Item &item) {
  if ( !item.hasPayload< boost::shared_ptr<T> >() )
    return 0;
  return item.payload< boost::shared_ptr<T> >().get();
}

template<class T> inline KCal::ListBase<T> itemsToIncidences(QList<Akonadi::Item> items) {
  KCal::ListBase<T> list;
  foreach( const Akonadi::Item &item, items )
    list.append( itemToIncidence<T>( item ) );
  return list;
}

template<class T> inline Akonadi::Item incidenceToItem(T* incidence) {
  Akonadi::Item item;
  item.setPayload< boost::shared_ptr<T> >( boost::shared_ptr<T>( incidence->clone() ) );
  return item;
}


CalendarAdaptor::CalendarAdaptor( Akonadi::Calendar *calendar, QWidget *parent,
                                  bool storeDefaultCollection )
  : KCal::Calendar( KCalPrefs::instance()->timeSpec() )
  , mCalendar( calendar ), mParent( parent ), mDeleteCalendar( false ),
    mStoreDefaultCollection( storeDefaultCollection )
{
  Q_ASSERT( mCalendar );
}

CalendarAdaptor::~CalendarAdaptor() {}

bool CalendarAdaptor::save() {
  return true;
}

bool CalendarAdaptor::reload()
{
  return true;
}

void CalendarAdaptor::close()
{

}

bool CalendarAdaptor::addEvent( KCal::Event *event )
{
  return addIncidence( Incidence::Ptr( event->clone() ) );
}

bool CalendarAdaptor::deleteEvent( KCal::Event *event )
{
  return deleteIncidence( incidenceToItem( event ) );
}

void CalendarAdaptor::deleteAllEvents()
{
  Q_ASSERT( false );
} //unused

Event::List CalendarAdaptor::rawEvents( KCal::EventSortField sortField,
                                        KCal::SortDirection sortDirection)
{
  return itemsToIncidences<Event>( mCalendar->rawEvents( ( Akonadi::EventSortField ) sortField,
                                                         ( Akonadi::SortDirection ) sortDirection ) );
}

KCal::Event::List CalendarAdaptor::rawEventsForDate( const KDateTime &dt )
{
  return itemsToIncidences<Event>( mCalendar->rawEventsForDate( dt ) );
}

KCal::Event::List CalendarAdaptor::rawEvents( const QDate &start, const QDate &end,
                                              const KDateTime::Spec &timeSpec,
                                              bool inclusive )
{
  return itemsToIncidences<KCal::Event>( mCalendar->rawEvents( start, end, timeSpec, inclusive ) );
}

KCal::Event::List CalendarAdaptor::rawEventsForDate( const QDate &date,
                                                     const KDateTime::Spec &timeSpec,
                                                     KCal::EventSortField sortField,
                                                     KCal::SortDirection sortDirection )
{
  return itemsToIncidences<Event>( mCalendar->rawEventsForDate( date, timeSpec,
                                                                ( Akonadi::EventSortField ) sortField,
                                                                ( Akonadi::SortDirection ) sortDirection ) );
}

KCal::Event *CalendarAdaptor::event( const QString &uid )
{
  return itemToIncidence<Event>( mCalendar->event( mCalendar->itemIdForIncidenceUid(uid) ) );
}

bool CalendarAdaptor::addTodo( KCal::Todo *todo )
{
  return addIncidence( Incidence::Ptr( todo->clone() ) );
}

bool CalendarAdaptor::deleteTodo( KCal::Todo *todo )
{
  return deleteIncidence( incidenceToItem( todo ) );
}

void CalendarAdaptor::deleteAllTodos()
{
  Q_ASSERT( false );
} //unused

Todo::List CalendarAdaptor::rawTodos( KCal::TodoSortField sortField,
                                      KCal::SortDirection sortDirection )
{
  return itemsToIncidences<Todo>( mCalendar->rawTodos( ( Akonadi::TodoSortField ) sortField,
                                                       ( Akonadi::SortDirection ) sortDirection ) );
}

Todo::List CalendarAdaptor::rawTodosForDate( const QDate &date )
{
  return itemsToIncidences<Todo>( mCalendar->rawTodosForDate( date ) );
}

Todo *CalendarAdaptor::todo( const QString &uid )
{
  return itemToIncidence<Todo>( mCalendar->todo( mCalendar->itemIdForIncidenceUid( uid ) ) );
}

bool CalendarAdaptor::addJournal( KCal::Journal *journal )
{
  return addIncidence( Incidence::Ptr( journal->clone() ) );
}

bool CalendarAdaptor::deleteJournal( KCal::Journal *journal )
{
  return deleteIncidence( incidenceToItem( journal ) );
}

void CalendarAdaptor::deleteAllJournals()
{
  Q_ASSERT( false );
} //unused

Journal::List CalendarAdaptor::rawJournals( KCal::JournalSortField sortField,
                                            KCal::SortDirection sortDirection )
{
  return itemsToIncidences<Journal>( mCalendar->rawJournals( ( Akonadi::JournalSortField ) sortField,
                                                             ( Akonadi::SortDirection ) sortDirection ) );
}

KCal::Journal::List CalendarAdaptor::rawJournalsForDate( const QDate &dt )
{
  return itemsToIncidences<Journal>( mCalendar->rawJournalsForDate( dt ) );
}

KCal::Journal *CalendarAdaptor::journal( const QString &uid )
{
  return itemToIncidence<Journal>( mCalendar->journal( mCalendar->itemIdForIncidenceUid( uid ) ) );
}

Alarm::List CalendarAdaptor::alarms( const KDateTime &from, const KDateTime &to )
{
  return mCalendar->alarms( from, to );
}


// From IncidenceChanger
bool CalendarAdaptor::addIncidence( const Incidence::Ptr &incidence )
{
  if( !incidence ) {
    return false;
  }
  Akonadi::Collection collection;

  const QString incidenceMimeType = Akonadi::subMimeTypeForIncidence( incidence.get() );

  if ( mStoreDefaultCollection && mDefaultCollection.isValid() ) {
    collection = mDefaultCollection;
  } else {
    int dialogCode = 0;
    QStringList mimeTypes( incidenceMimeType );
    collection = Akonadi::selectCollection( mParent, dialogCode, mimeTypes );
  }

  if ( !collection.isValid() ) {
    return false;
  }

  if ( mStoreDefaultCollection && !mDefaultCollection.isValid() ) {
    mDefaultCollection = collection;
  }

  kDebug() << "\"" << incidence->summary() << "\"";

  Akonadi::Item item;
  item.setPayload( incidence );

  item.setMimeType( incidenceMimeType );
  Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( item, collection );
  // The connection needs to be queued to be sure addIncidenceFinished is called after the kjob finished
  // it's eventloop. That's needed cause Groupware uses synchron job->exec() calls.
  connect( job, SIGNAL( result(KJob*)), this, SLOT(addIncidenceFinished(KJob*)), Qt::QueuedConnection );
  return true;
}

bool CalendarAdaptor::deleteIncidence( const Akonadi::Item &aitem, bool deleteCalendar )
{
  mDeleteCalendar = deleteCalendar;
  const Incidence::Ptr incidence = Akonadi::incidence( aitem );
  if ( !incidence ) {
    return true;
  }

  kDebug() << "\"" << incidence->summary() << "\"";
  bool doDelete = sendGroupwareMessage( aitem, KCal::iTIPCancel,
                                        IncidenceChanger::INCIDENCEDELETED );
  if( !doDelete ) {
    if ( mDeleteCalendar )
      deleteLater();
    return false;
  }
  Akonadi::ItemDeleteJob* job = new Akonadi::ItemDeleteJob( aitem );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(deleteIncidenceFinished(KJob*)) );
  return true;
}


void CalendarAdaptor::addIncidenceFinished( KJob* j )
{
  kDebug();
  const Akonadi::ItemCreateJob* job = qobject_cast<const Akonadi::ItemCreateJob*>( j );
  Q_ASSERT( job );
  Incidence::Ptr incidence = Akonadi::incidence( job->item() );

  if  ( job->error() ) {
    KMessageBox::sorry(
      mParent,
      i18n( "Unable to save %1 \"%2\": %3",
            i18n( incidence->type() ),
            incidence->summary(),
            job->errorString() ) );
    if ( mDeleteCalendar ) {
      deleteLater();
    }
    return;
  }

  Q_ASSERT( incidence );
  if ( KCalPrefs::instance()->mUseGroupwareCommunication ) {
    if ( !Groupware::instance()->sendICalMessage(
           mParent,
           KCal::iTIPRequest,
           incidence.get(), IncidenceChanger::INCIDENCEADDED, false ) ) {
      kError() << "sendIcalMessage failed.";
    }
  }
  if ( mDeleteCalendar ) {
    deleteLater();
  }
}

void CalendarAdaptor::deleteIncidenceFinished( KJob* j )
{
  kDebug();
  const Akonadi::ItemDeleteJob* job = qobject_cast<const Akonadi::ItemDeleteJob*>( j );
  Q_ASSERT( job );
  const Akonadi::Item::List items = job->deletedItems();
  Q_ASSERT( items.count() == 1 );
  Incidence::Ptr tmp = Akonadi::incidence( items.first() );
  Q_ASSERT( tmp );
  if ( job->error() ) {
    KMessageBox::sorry( mParent,
                        i18n( "Unable to delete incidence %1 \"%2\": %3",
                              i18n( tmp->type() ),
                              tmp->summary(),
                              job->errorString( )) );
    return;
  }

  if ( !KCalPrefs::instance()->thatIsMe( tmp->organizer().email() ) ) {
    const QStringList myEmails = KCalPrefs::instance()->allEmails();
    bool notifyOrganizer = false;
    for ( QStringList::ConstIterator it = myEmails.begin(); it != myEmails.end(); ++it ) {
      QString email = *it;
      Attendee *me = tmp->attendeeByMail( email );
      if ( me ) {
        if ( me->status() == KCal::Attendee::Accepted ||
             me->status() == KCal::Attendee::Delegated ) {
          notifyOrganizer = true;
        }
        Attendee *newMe = new Attendee( *me );
        newMe->setStatus( KCal::Attendee::Declined );
        tmp->clearAttendees();
        tmp->addAttendee( newMe );
        break;
      }
    }

    if ( !Groupware::instance()->doNotNotify() && notifyOrganizer ) {
      MailScheduler scheduler( mCalendar );
      scheduler.performTransaction( tmp.get(), KCal::iTIPReply );
    }
    //reset the doNotNotify flag
    Groupware::instance()->setDoNotNotify( false );
  }
}

bool CalendarAdaptor::sendGroupwareMessage( const Akonadi::Item &aitem,
                                            KCal::iTIPMethod method,
                                            IncidenceChanger::HowChanged action )
{
  const Incidence::Ptr incidence = Akonadi::incidence( aitem );
  if ( !incidence ) {
    return false;
  }

  if ( KCalPrefs::instance()->thatIsMe( incidence->organizer().email() ) &&
       incidence->attendeeCount() > 0 &&
       !KCalPrefs::instance()->mUseGroupwareCommunication ) {
    schedule( method, aitem );
    return true;
  } else if ( KCalPrefs::instance()->mUseGroupwareCommunication ) {
    return Groupware::instance()->sendICalMessage( mParent, method,
                                                   incidence.get(), action,  false );
  }
  return true;
}

//Coming from CalendarView
void CalendarAdaptor::schedule( iTIPMethod method, const Akonadi::Item &item )
{
  Incidence::Ptr incidence = Akonadi::incidence( item );

  if ( incidence->attendeeCount() == 0 && method != iTIPPublish ) {
    KMessageBox::information( mParent, i18n( "The item has no attendees." ),
                              QLatin1String( "ScheduleNoIncidences" ) );
    return;
  }

  Incidence *inc = incidence->clone();
  inc->registerObserver( 0 );
  inc->clearAttendees();

  // Send the mail
  MailScheduler scheduler( mCalendar );
  if ( scheduler.performTransaction( incidence.get(), method ) ) {
    KMessageBox::information( mParent,
                              i18n( "The groupware message for item '%1' "
                                    "was successfully sent.\nMethod: %2",
                                    incidence->summary(),
                                    Scheduler::methodName( method ) ),
                              i18n( "Sending Free/Busy" ),
                              QLatin1String( "FreeBusyPublishSuccess" ) );
  } else {
    KMessageBox::error( mParent,
                        i18nc( "Groupware message sending failed. "
                               "%2 is request/reply/add/cancel/counter/etc.",
                               "Unable to send the item '%1'.\nMethod: %2",
                               incidence->summary(),
                               Scheduler::methodName( method ) ) );
  }
}

void CalendarAdaptor::incidenceUpdate( const QString & )
{

}

void CalendarAdaptor::incidenceUpdated( const QString & )
{
}

