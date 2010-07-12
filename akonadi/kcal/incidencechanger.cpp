/*
  This file is part of KOrganizer.

  Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

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

#include "incidencechanger.h"
#include "incidencechanger_p.h"
#include "groupware.h"
#include "kcalprefs.h"

#include <akonadi/kcal/calendar.h>
#include <akonadi/kcal/groupware.h>
#include <akonadi/kcal/mailscheduler.h>
#include <akonadi/kcal/utils.h>
#include <akonadi/kcal/dndfactory.h>

#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/Collection>

#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>

#include <kcalcore/memorycalendar.h>
#include <kcalcore/freebusy.h>
#include <kcalcore/incidence.h>

#include <KDebug>
#include <KLocale>
#include <KMessageBox>

using namespace KCalCore;
using namespace Akonadi;

bool IncidenceChanger::Private::myAttendeeStatusChanged( const Incidence* newInc,
                                                         const Incidence* oldInc )
{
  Attendee *oldMe = oldInc->attendeeByMails( KCalPrefs::instance()->allEmails() );
  Attendee *newMe = newInc->attendeeByMails( KCalPrefs::instance()->allEmails() );
  if ( oldMe && newMe && ( oldMe->status() != newMe->status() ) ) {
    return true;
  }

  return false;
}

void IncidenceChanger::Private::queueChange( Change *change )
{
  // If there's already a change queued we just discard it
  // and send the newer change, which already includes
  // previous modifications
  const Item::Id id = change->newItem.id();
  if ( mQueuedChanges.contains( id ) ) {
    delete mQueuedChanges.take( id );
  }

  mQueuedChanges[id] = change;
}

void IncidenceChanger::Private::cancelChanges( Item::Id id )
{
  delete mQueuedChanges.take( id );
  delete mCurrentChanges.take( id );
}

void IncidenceChanger::Private::performNextChange( Item::Id id )
{
  delete mCurrentChanges.take( id );

  if ( mQueuedChanges.contains( id ) ) {
    performChange( mQueuedChanges.take( id ) );
  }
}

bool IncidenceChanger::Private::performChange( Change *change )
{
  Item newItem = change->newItem;
  const Incidence::Ptr oldinc =  change->oldInc;
  const Incidence::Ptr newinc = Akonadi::incidence( newItem );

  kDebug() << "id="                  << newItem.id()         <<
              "uid="                 << newinc->uid()        <<
              "version="             << newItem.revision()   <<
              "summary="             << newinc->summary()    <<
              "old summary"          << oldinc->summary()    <<
              "type="                << newinc->type()       <<
              "storageCollectionId=" << newItem.storageCollectionId();

  // There's not any job modifying this item, so mCurrentChanges[item.id] can't exist
  Q_ASSERT( !mCurrentChanges.contains( newItem.id() ) );

  // Check if the item was deleted, we already check in changeIncidence() but
  // this change could be already in the queue when the item was deleted
  if ( !mCalendar->incidence( newItem.id() ).isValid() ||
       mDeletedItemIds.contains( newItem.id() ) ) {
    kDebug() << "Incidence deleted";
    // return true, the user doesn't want to see errors because he was too fast
    return true;
  }

  if ( newinc.data() == oldinc.data() ) {
    // Don't do anything
    kDebug() << "Incidence not changed";
    return true;
  } else {

    if ( mLatestRevisionByItemId.contains( newItem.id() ) &&
         mLatestRevisionByItemId[newItem.id()] > newItem.revision() ) {
      /* When a ItemModifyJob ends, the application can still modify the old items if the user
       * is quick because the ETM wasn't updated yet, and we'll get a STORE error, because
       * we are not modifying the latest revision.
       *
       * When a job ends, we keep the new revision in m_latestVersionByItemId
       * so we can update the item's revision
       */
      newItem.setRevision( mLatestRevisionByItemId[newItem.id()] );
    }

    kDebug() << "Changing incidence";
    const bool attendeeStatusChanged = myAttendeeStatusChanged( oldinc.data(), newinc.data() );
    const int revision = newinc->revision();
    newinc->setRevision( revision + 1 );
    // FIXME: Use a generic method for this! Ideally, have an interface class
    //        for group cheduling. Each implementation could then just do what
    //        it wants with the event. If no groupware is used,use the null
    //        pattern...
    bool success = true;
    if ( KCalPrefs::instance()->mUseGroupwareCommunication ) {
      if ( !mGroupware ) {
          kError() << "Groupware communication enabled but no groupware instance set";
      } else {
        success = mGroupware->sendICalMessage( change->parent,
                                               KCalCore::iTIPRequest,
                                               newinc.data(),
                                               INCIDENCEEDITED,
                                               attendeeStatusChanged );
      }
    }

    if ( !success ) {
      kDebug() << "Changing incidence failed. Reverting changes.";
      *newinc.data() =  *oldinc.data();
      return false;
    }
  }

  // FIXME: if that's a groupware incidence, and I'm not the organizer,
  // send out a mail to the organizer with a counterproposal instead
  // of actually changing the incidence. Then no locking is needed.
  // FIXME: if that's a groupware incidence, and the incidence was
  // never locked, we can't unlock it with endChange().

  mCurrentChanges[newItem.id()] = change;

  // Don't write back remote revision since we can't make sure it is the current one
  // fixes problems with DAV resource
  newItem.setRemoteRevision( QString() );

  ItemModifyJob *job = new ItemModifyJob( newItem );
  connect( job, SIGNAL(result( KJob*)), this, SLOT(changeIncidenceFinished(KJob*)) );
  return true;
}

void IncidenceChanger::Private::changeIncidenceFinished( KJob* j )
{
  // we should probably update the revision number here,or internally in the Event
  // itself when certain things change. need to verify with ical documentation.
  const ItemModifyJob* job = qobject_cast<const ItemModifyJob*>( j );
  Q_ASSERT( job );

  const Item newItem = job->item();

  if ( !mCurrentChanges.contains( newItem.id() ) ) {
    kDebug() << "Item was deleted? Great.";
    cancelChanges( newItem.id() );
    emit incidenceChangeFinished( Item(), newItem, UNKNOWN_MODIFIED  , true );
    return;
  }

  const Private::Change *change = mCurrentChanges[newItem.id()];
  const Incidence::Ptr oldInc = change->oldInc;

  Item oldItem;
  oldItem.setPayload<Incidence::Ptr>( oldInc );
  oldItem.setMimeType( QString::fromLatin1( "application/x-vnd.akonadi.calendar.%1" )
                       .arg( QLatin1String( oldInc->type().toLower() ) ) );
  oldItem.setId( newItem.id() );

  if ( job->error() ) {
    kWarning( 5250 ) << "Item modify failed:" << job->errorString();

    const Incidence::Ptr newInc = Akonadi::incidence( newItem );
    KMessageBox::sorry( change->parent,
                        i18n( "Unable to save changes for incidence %1 \"%2\": %3",
                              i18n( newInc->type() ),
                              newInc->summary(),
                              job->errorString( )) );
    emit incidenceChangeFinished( oldItem, newItem, change->action, false );
  } else {
    emit incidenceChangeFinished( oldItem, newItem, change->action, true );
  }

  mLatestRevisionByItemId[newItem.id()] = newItem.revision();

  // execute any other modification if it exists
  qRegisterMetaType<Akonadi::Item::Id>( "Akonadi::Item::Id" );
  QMetaObject::invokeMethod( this, "performNextChange",
                             Qt::QueuedConnection,
                             Q_ARG( Akonadi::Item::Id, newItem.id() ) );
}

IncidenceChanger::IncidenceChanger( Akonadi::Calendar *cal,
                                    QObject *parent,
                                    Entity::Id defaultCollectionId )
  : QObject( parent) , d( new Private( defaultCollectionId, cal ) )
{
  connect( d, SIGNAL(incidenceChangeFinished(Akonadi::Item,Akonadi::Item,Akonadi::IncidenceChanger::WhatChanged,bool)),
           SIGNAL(incidenceChangeFinished(Akonadi::Item,Akonadi::Item,Akonadi::IncidenceChanger::WhatChanged,bool)) );
}

IncidenceChanger::~IncidenceChanger()
{
  delete d;
}

void IncidenceChanger::setGroupware( Groupware *groupware )
{
  d->mGroupware = groupware;
}

bool IncidenceChanger::sendGroupwareMessage( const Item &aitem,
                                             KCalCore::iTIPMethod method,
                                             HowChanged action,
                                             QWidget *parent )
{
  const Incidence::Ptr incidence = Akonadi::incidence( aitem );
  if ( !incidence ) {
    return false;
  }
  if ( KCalPrefs::instance()->thatIsMe( incidence->organizer().email() ) &&
       incidence->attendeeCount() > 0 &&
       !KCalPrefs::instance()->mUseGroupwareCommunication ) {
    emit schedule( method, aitem );
    return true;
  } else if ( KCalPrefs::instance()->mUseGroupwareCommunication ) {
    if ( !d->mGroupware ) {
      kError() << "Groupware communication enabled but no groupware instance set";
      return false;
    }
    return d->mGroupware->sendICalMessage( parent, method, incidence.data(), action, false );
  }
  return true;
}

void IncidenceChanger::cancelAttendees( const Item &aitem )
{
  const Incidence::Ptr incidence = Akonadi::incidence( aitem );
  Q_ASSERT( incidence );
  if ( KCalPrefs::instance()->mUseGroupwareCommunication ) {
    if ( KMessageBox::questionYesNo(
           0,
           i18n( "Some attendees were removed from the incidence. "
                 "Shall cancel messages be sent to these attendees?" ),
           i18n( "Attendees Removed" ), KGuiItem( i18n( "Send Messages" ) ),
           KGuiItem( i18n( "Do Not Send" ) ) ) == KMessageBox::Yes ) {
      // don't use Akonadi::Groupware::sendICalMessage here, because that asks just
      // a very general question "Other people are involved, send message to
      // them?", which isn't helpful at all in this situation. Afterwards, it
      // would only call the Akonadi::MailScheduler::performTransaction, so do this
      // manually.
      // FIXME: Groupware scheduling should be factored out to it's own class
      //        anyway
      Akonadi::MailScheduler scheduler( static_cast<Akonadi::Calendar*>(d->mCalendar) );
      scheduler.performTransaction( incidence.data(), iTIPCancel );
    }
  }
}

bool IncidenceChanger::deleteIncidence( const Item &aitem, QWidget *parent )
{
  const Incidence::Ptr incidence = Akonadi::incidence( aitem );
  if ( !incidence ) {
    return false;
  }

  if ( !isNotDeleted( aitem.id() ) ) {
    kDebug() << "Item already deleted, skipping and returning true";
    return true;
  }

  if ( !( aitem.parentCollection().rights() & Collection::CanDeleteItem ) ) {
    kWarning() << "insufficient rights to delete incidence";
    return false;
  }

  bool doDelete = sendGroupwareMessage( aitem, KCalCore::iTIPCancel,
                                        INCIDENCEDELETED, parent );
  if( !doDelete ) {
    return false;
  }

  d->mDeletedItemIds.append( aitem.id() );

  emit incidenceToBeDeleted( aitem );
  d->cancelChanges( aitem.id() ); //abort changes to this incidence cause we will just delete it
  ItemDeleteJob* job = new ItemDeleteJob( aitem );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(deleteIncidenceFinished(KJob*)) );
  return true;
}

void IncidenceChanger::deleteIncidenceFinished( KJob* j )
{
  // todo, cancel changes?
  kDebug();
  const ItemDeleteJob* job = qobject_cast<const ItemDeleteJob*>( j );
  Q_ASSERT( job );
  const Item::List items = job->deletedItems();
  Q_ASSERT( items.count() == 1 );
  Incidence::Ptr tmp = Akonadi::incidence( items.first() );
  Q_ASSERT( tmp );
  if ( job->error() ) {
    KMessageBox::sorry( 0, //PENDING(AKONADI_PORT) set parent
                        i18n( "Unable to delete incidence %1 \"%2\": %3",
                              i18n( tmp->type() ),
                              tmp->summary(),
                              job->errorString( )) );
    d->mDeletedItemIds.removeOne( items.first().id() );
    emit incidenceDeleteFinished( items.first(), false );
    return;
  }
  if ( !KCalPrefs::instance()->thatIsMe( tmp->organizer().email() ) ) {
    const QStringList myEmails = KCalPrefs::instance()->allEmails();
    bool notifyOrganizer = false;
    for ( QStringList::ConstIterator it = myEmails.begin(); it != myEmails.end(); ++it ) {
      QString email = *it;
      Attendee *me = tmp->attendeeByMail( email );
      if ( me ) {
        if ( me->status() == KCalCore::Attendee::Accepted ||
             me->status() == KCalCore::Attendee::Delegated ) {
          notifyOrganizer = true;
        }
        Attendee *newMe = new Attendee( *me );
        newMe->setStatus( KCalCore::Attendee::Declined );
        tmp->clearAttendees();
        tmp->addAttendee( newMe );
        break;
      }
    }

    if ( d->mGroupware ) {
      if ( !d->mGroupware->doNotNotify() && notifyOrganizer ) {
        Akonadi::MailScheduler scheduler( static_cast<Akonadi::Calendar*>(d->mCalendar) );
        scheduler.performTransaction( tmp.get(), KCalCore::iTIPReply );
      }
      //reset the doNotNotify flag
      d->mGroupware->setDoNotNotify( false );
    }
  }
  d->mLatestRevisionByItemId.remove( items.first().id() );
  emit incidenceDeleteFinished( items.first(), true );
}

bool IncidenceChanger::cutIncidences( const Item::List &list, QWidget *parent )
{
  Item::List::ConstIterator it;
  bool doDelete = true;
  Item::List itemsToCut;
  for ( it = list.constBegin(); it != list.constEnd(); ++it ) {
    if ( Akonadi::hasIncidence( ( *it ) ) ) {
      doDelete = sendGroupwareMessage( *it, KCalCore::iTIPCancel,
                                       INCIDENCEDELETED, parent );
      if ( doDelete ) {
        emit incidenceToBeDeleted( *it );
        itemsToCut.append( *it );
      }
    }
   }
  KCalCore::MemoryCalendar *cal = new KCalCore::MemoryCalendar( d->mCalendar, parent );
  Akonadi::DndFactory factory( cal, true /*delete calendarAdaptor*/ );

  if ( factory.cutIncidences( itemsToCut ) ) {
    for ( it = itemsToCut.constBegin(); it != itemsToCut.constEnd(); ++it ) {
      emit incidenceDeleteFinished( *it, true );
    }
    return !itemsToCut.isEmpty();
  } else {
    return false;
  }
}

bool IncidenceChanger::cutIncidence( const Item &item, QWidget *parent )
{
  Item::List items;
  items.append( item );
  return cutIncidences( items, parent );
}

void IncidenceChanger::setDefaultCollectionId( Entity::Id defaultCollectionId )
{
  d->mDefaultCollectionId = defaultCollectionId;
}

bool IncidenceChanger::changeIncidence( const KCalCore::Incidence::Ptr &oldinc,
                                        const Item &newItem,
                                        WhatChanged action,
                                        QWidget *parent )
{
  if ( !Akonadi::hasIncidence( newItem ) ||
       !newItem.isValid() ) {
    kDebug() << "Skipping invalid item id=" << newItem.id();
    return false;
  }

  if ( !( newItem.parentCollection().rights() & Collection::CanChangeItem ) ) {
    kWarning() << "insufficient rights to change incidence";
    return false;
  }

  if ( !isNotDeleted( newItem.id() ) ) {
    kDebug() << "Skipping change, the item got deleted";
    return false;
  }

  Private::Change *change = new Private::Change();
  change->action = action;
  change->newItem = newItem;
  change->oldInc = oldinc;
  change->parent = parent;

  if ( d->mCurrentChanges.contains( newItem.id() ) ) {
    d->queueChange( change );
  } else {
    d->performChange( change );
  }

  return true;
}


bool IncidenceChanger::addIncidence( const KCalCore::Incidence::Ptr &incidence,
                                     QWidget *parent, Akonadi::Collection &selectedCollection,
                                     int &dialogCode )
{
  const Collection defaultCollection = d->mCalendar->collection( d->mDefaultCollectionId );

  const QString incidenceMimeType = Akonadi::subMimeTypeForIncidence( incidence.data() );
  const bool defaultIsOk = defaultCollection.contentMimeTypes().contains( incidenceMimeType ) &&
                           defaultCollection.rights() & Collection::CanCreateItem;

  if ( d->mDestinationPolicy == ASK_DESTINATION ||
       !defaultCollection.isValid() ||
       !defaultIsOk ) {
    QStringList mimeTypes( incidenceMimeType );
    selectedCollection = Akonadi::selectCollection( parent,
                                                    dialogCode,
                                                    mimeTypes,
                                                    defaultCollection );
  } else {
    dialogCode = QDialog::Accepted;
    selectedCollection = defaultCollection;
  }

  if ( selectedCollection.isValid() ) {
    return addIncidence( incidence, selectedCollection, parent );
  } else {
    return false;
  }
}

bool IncidenceChanger::addIncidence( const Incidence::Ptr &incidence,
                                     const Collection &collection, QWidget *parent )
{
  if ( !incidence || !collection.isValid() ) {
    return false;
  }

  if ( !( collection.rights() & Collection::CanCreateItem ) ) {
    kWarning() << "insufficient rights to create incidence";
    return false;
  }

  Item item;
  item.setPayload<KCalCore::Incidence::Ptr>( incidence );

  item.setMimeType( Akonadi::subMimeTypeForIncidence( incidence.data() ) );
  ItemCreateJob *job = new ItemCreateJob( item, collection );

  // The connection needs to be queued to be sure addIncidenceFinished is called after the kjob finished
  // it's eventloop. That's needed cause Akonadi::Groupware uses synchron job->exec() calls.
  connect( job, SIGNAL(result(KJob*)),
           this, SLOT(addIncidenceFinished(KJob*)), Qt::QueuedConnection );
  return true;
}

void IncidenceChanger::addIncidenceFinished( KJob* j )
{
  kDebug();
  const Akonadi::ItemCreateJob* job = qobject_cast<const Akonadi::ItemCreateJob*>( j );
  Q_ASSERT( job );
  Incidence::Ptr incidence = Akonadi::incidence( job->item() );

  if  ( job->error() ) {
    KMessageBox::sorry(
      0, //PENDING(AKONADI_PORT) set parent, ideally the one passed in addIncidence...
      i18n( "Unable to save %1 \"%2\": %3",
            i18n( incidence->type() ),
            incidence->summary(),
            job->errorString() ) );
    emit incidenceAddFinished( job->item(), false );
    return;
  }

  Q_ASSERT( incidence );
  if ( KCalPrefs::instance()->mUseGroupwareCommunication ) {
    if ( !d->mGroupware ) {
      kError() << "Groupware communication enabled but no groupware instance set";
    } else if ( !d->mGroupware->sendICalMessage(
           0, //PENDING(AKONADI_PORT) set parent, ideally the one passed in addIncidence...
           KCalCore::iTIPRequest,
           incidence.data(), INCIDENCEADDED, false ) ) {
      kError() << "sendIcalMessage failed.";
    }
  }

  emit incidenceAddFinished( job->item(), true );
}

void IncidenceChanger::setDestinationPolicy( DestinationPolicy destinationPolicy )
{
  d->mDestinationPolicy = destinationPolicy;
}

IncidenceChanger::DestinationPolicy IncidenceChanger::destinationPolicy() const
{
  return d->mDestinationPolicy;
}

bool IncidenceChanger::isNotDeleted( Akonadi::Item::Id id ) const
{
  if ( d->mCalendar->incidence( id ).isValid() ) {
    // it's inside the calendar, but maybe it's being deleted by a job or was
    // deleted but the ETM doesn't know yet
    return !d->mDeletedItemIds.contains( id );
  } else {
    // not inside the calendar, i don't know it
    return false;
  }
}
