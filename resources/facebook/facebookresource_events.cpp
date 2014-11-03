/*
  Copyright 2010, 2011 Thomas McGuire <mcguire@kde.org>
  Copyright 2011 Roeland Jago Douma <unix@rullzer.com>

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published
  by the Free Software Foundation; either version 2 of the License or
  ( at your option ) version 3 or, at the discretion of KDE e.V.
  ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "facebookresource.h"
#include "settings.h"
#include "settingsdialog.h"
#include "timestampattribute.h"

#include <KFbAPI/alleventslistjob.h>
#include <KFbAPI/eventjob.h>
#include <KFbAPI/eventinfo.h>

#include <KPIMUtils/LinkLocator>
#include <KCalCore/Attendee>

#include <KLocalizedString>

#include <AkonadiCore/AttributeFactory>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/ChangeRecorder>

using namespace Akonadi;

void FacebookResource::eventListFetched( KJob *job )
{
  Q_ASSERT( !mIdle );
  Q_ASSERT( mCurrentJobs.indexOf( job ) != -1 );
  KFbAPI::AllEventsListJob * const listJob = qobject_cast<KFbAPI::AllEventsListJob*>( job );
  Q_ASSERT( listJob );
  mCurrentJobs.removeAll( job );

  if ( listJob->error() ) {
    abortWithError( i18n( "Unable to get events from server: %1", listJob->errorString() ),
                    listJob->error() == KFbAPI::FacebookJob::AuthenticationProblem );
  } else {
    QStringList eventIds;
    Q_FOREACH ( const KFbAPI::EventInfo &event, listJob->allEvents() ) {
      eventIds.append( event.id() );
    }
    if ( eventIds.isEmpty() ) {
      itemsRetrievalDone();
      finishEventsFetching();
      return;
    }
    KFbAPI::EventJob * const eventJob =
      new KFbAPI::EventJob( eventIds, Settings::self()->accessToken(), this );
    mCurrentJobs << eventJob;
    connect( eventJob, SIGNAL(result(KJob*)), this, SLOT(detailedEventListJobFinished(KJob*)) );
    eventJob->start();
  }

  listJob->deleteLater();
}

void FacebookResource::detailedEventListJobFinished( KJob *job )
{
  Q_ASSERT( !mIdle );
  Q_ASSERT( mCurrentJobs.indexOf( job ) != -1 );
  KFbAPI::EventJob * const eventJob = qobject_cast<KFbAPI::EventJob*>( job );
  Q_ASSERT( eventJob );
  mCurrentJobs.removeAll( job );

  if ( job->error() ) {
    abortWithError( i18n( "Unable to get list of events from server: %1", eventJob->errorText() ) );
  } else {
    setItemStreamingEnabled( true );

    Item::List eventItems;
    Q_FOREACH ( const KFbAPI::EventInfo &eventInfo, eventJob->eventInfo() ) {
      if (eventInfo.id().isEmpty()) {
        //skip invalid events
        continue;
      }
      Item event;
      event.setRemoteId( eventInfo.id() );
      event.setPayload<KCalCore::Incidence::Ptr>(convertEventInfoToEventPtr(eventInfo));
      event.setMimeType( eventMimeType );
      eventItems.append( event );
    }
    itemsRetrieved( eventItems );
    itemsRetrievalDone();
    finishEventsFetching();
  }

  eventJob->deleteLater();
}

void FacebookResource::finishEventsFetching()
{
  emit percent( 100 );
  // TODO: Use an actual number here
  emit status( Idle, i18n( "All events fetched from server." ) );
  resetState();
}

KCalCore::Event::Ptr FacebookResource::convertEventInfoToEventPtr(const KFbAPI::EventInfo &eventInfo)
{
    KCalCore::Event::Ptr event(new KCalCore::Event);
    QString desc = eventInfo.description();
    desc = KPIMUtils::LinkLocator::convertToHtml(desc, KPIMUtils::LinkLocator::ReplaceSmileys);
    if (!desc.isEmpty()) {
        desc += "<br><br>";
    }
    desc += "<a href=\"" + QString("http://www.facebook.com/event.php?eid=%1").arg(id()) +
            "\">" + i18n("View Event on Facebook") + "</a>";

    event->setSummary(eventInfo.name());
    event->setLastModified(eventInfo.updatedTime());
    event->setCreated(eventInfo.updatedTime()); // That's a lie, but Facebook doesn't give us the created time
    event->setDescription(desc, true);
    event->setLocation(eventInfo.location());
    event->setHasEndDate(eventInfo.endTime().isValid());
    event->setOrganizer(eventInfo.organizer());
    event->setUid(eventInfo.id());
    if (eventInfo.startTime().isValid()) {
        event->setDtStart(eventInfo.startTime());
    } else {
        qWarning() << "Event has no start date";
    }
    if (eventInfo.endTime().isValid()) {
        event->setDtEnd(eventInfo.endTime());
    } else if (eventInfo.startTime().isValid() && !eventInfo.endTime().isValid()) {
        // Urgh...
        QDateTime endDate;
        endDate.setDate(eventInfo.startTime().date());
        endDate.setTime(QTime::fromString("23:59:59"));
        qWarning() << "Event without end time: " << event->summary() << event->dtStart();
        qWarning() << "Making it an event until the end of the day.";
        event->setDtEnd(endDate);
        //kWarning() << "Using a duration of 2 hours";
        //event->setDuration(KCalCore::Duration(2 * 60 * 60, KCalCore::Duration::Seconds));
    }

    auto attendeeRsvp = [](const QString &status) {
        if (status == QLatin1String("noreply")) {
            return KCalCore::Attendee::NeedsAction;
        } else if (status == QLatin1String("maybe")) {
            return KCalCore::Attendee::Tentative;
        } else if (status == QLatin1String("attending")) {
            return KCalCore::Attendee::Accepted;
        } else if (status == QLatin1String("declined")) {
            return KCalCore::Attendee::Declined;
        }
    };

    // TODO: Organizer
    //       Public/Private -> freebusy!
    //       venue: add to location?
    //       picture?
    Q_FOREACH (const KFbAPI::AttendeeInfoPtr &attendeeInfo, eventInfo.attendees()) {
        KCalCore::Attendee::Ptr attendee(new KCalCore::Attendee(attendeeInfo->name(),
                                                                QStringLiteral("facebook@unkown.invalid"),
                                                                false,
                                                                attendeeRsvp(attendeeInfo->status()),
                                                                KCalCore::Attendee::OptParticipant,
                                                                attendeeInfo->id()));
        event->addAttendee(attendee);
    }

    return event;
}

