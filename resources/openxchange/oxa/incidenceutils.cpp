/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "incidenceutils.h"

#include "davmanager.h"
#include "davutils.h"
#include "oxutils.h"
#include "users.h"

#include <kcalcore/event.h>
#include <kcalcore/todo.h>

#include <QtXml/QDomElement>

#include <QtCore/QBitArray>
#include <QtCore/QDebug>

using namespace OXA;

static void parseMembersAttribute( const QDomElement &element,
                                   const KCalCore::Incidence::Ptr &incidence )
{
  incidence->clearAttendees();

  for ( QDomElement child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement() ) {
    if ( child.tagName() == QLatin1String( "user" ) ) {
      const QString uid = child.text();

      const User user = Users::self()->lookupUid( uid.toLongLong() );

      QString name;
      QString email;
      KCalCore::Attendee::Ptr attendee = incidence->attendeeByUid( uid );
      if ( !user.isValid() ) {
        if ( attendee )
          continue;

        name = uid;
        email = uid + '@' + KUrl( DavManager::self()->baseUrl() ).host();
      } else {
        name = user.name();
        email = user.email();
      }

      if ( attendee ) {
        attendee->setName( name );
        attendee->setEmail( email );
      } else {
        attendee = KCalCore::Attendee::Ptr( new KCalCore::Attendee( name, email ) );
        attendee->setUid( uid );
        incidence->addAttendee( attendee );
      }

      const QString status = child.attribute( "confirm" );
      if ( !status.isEmpty() ) {
        if ( status == QLatin1String( "accept" ) ) {
          attendee->setStatus( KCalCore::Attendee::Accepted );
        } else if ( status == QLatin1String( "decline" ) ) {
          attendee->setStatus( KCalCore::Attendee::Declined );
        } else {
          attendee->setStatus( KCalCore::Attendee::NeedsAction );
        }
      }
    }
  }
}

static void parseIncidenceAttribute( const QDomElement &element,
                                     const KCalCore::Incidence::Ptr &incidence )
{
  const QString tagName = element.tagName();
  const QString text = OXUtils::readString( element.text() );

  if ( tagName == QLatin1String( "title" ) ) {
    incidence->setSummary( text );
  } else if ( tagName == QLatin1String( "note" ) ) {
    incidence->setDescription( text );
  } else if ( tagName == QLatin1String( "alarm" ) ) {
    const int minutes = OXUtils::readNumber( element.text() );
    if ( minutes != 0 ) {
      KCalCore::Alarm::List alarms = incidence->alarms();
      KCalCore::Alarm::Ptr alarm;
      if ( alarms.isEmpty() )
        alarm = incidence->newAlarm();
      else
        alarm = alarms.first();

      if ( alarm->type() == KCalCore::Alarm::Invalid )
        alarm->setType( KCalCore::Alarm::Display );

      KCalCore::Duration duration( minutes * -60 );
      alarm->setStartOffset( duration );
      alarm->setEnabled( true );
    } else {
      // 0 reminder -> disable alarm
      incidence->clearAlarms();
    }
  } else if ( tagName == QLatin1String( "created_by" ) ) {
    const User user = Users::self()->lookupUid( OXUtils::readNumber( element.text() ) );
    incidence->setOrganizer( KCalCore::Person::Ptr( new KCalCore::Person( user.name(), user.email() ) ) );
  } else if ( tagName == QLatin1String( "participants" ) ) {
    parseMembersAttribute( element, incidence );
  } else if ( tagName == QLatin1String( "private_flag" ) ) {
    if ( OXUtils::readBoolean( element.text() ) == true )
      incidence->setSecrecy( KCalCore::Incidence::SecrecyPrivate );
    else
      incidence->setSecrecy( KCalCore::Incidence::SecrecyPublic );
  } else if ( tagName == QLatin1String( "categories" ) ) {
    incidence->setCategories( text.split( QRegExp( QLatin1String( ",\\s*" ) ) ) );
  }
}

static void parseEventAttribute( const QDomElement &element,
                                 const KCalCore::Event::Ptr &event )
{
  const QString tagName = element.tagName();
  const QString text = OXUtils::readString( element.text() );

  if ( tagName == QLatin1String( "start_date" ) ) {
    KDateTime dateTime = OXUtils::readDateTime( element.text() );
    if ( event->allDay() )
      dateTime.setDateOnly( true );

    event->setDtStart( dateTime );

  } else if ( tagName == QLatin1String( "end_date" ) ) {
    KDateTime dateTime = OXUtils::readDateTime( element.text() );
    if ( event->allDay() )
      dateTime = dateTime.addSecs( -1 );

    event->setDtEnd( dateTime );

  } else if ( tagName == QLatin1String( "location" ) ) {
    event->setLocation( text );
  }
}

static void parseTodoAttribute( const QDomElement &element,
                                const KCalCore::Todo::Ptr &todo )
{
  const QString tagName = element.tagName();
  const QString text = OXUtils::readString( element.text() );

  if ( tagName == QLatin1String( "start_date" ) ) {
    const KDateTime dateTime = OXUtils::readDateTime( element.text() );
    if ( dateTime.isValid() ) {
      todo->setDtStart( dateTime );
      todo->setHasStartDate( true );
    }
  } else if ( tagName == QLatin1String( "end_date" ) ) {
    const KDateTime dateTime = OXUtils::readDateTime( element.text() );
    if ( dateTime.isValid() ) {
      todo->setDtDue( dateTime );
      todo->setHasDueDate( true );
    }
  } else if ( tagName == QLatin1String( "priority" ) ) {
    const int priorityNumber = OXUtils::readNumber( element.text() );
    if ( priorityNumber < 1 || priorityNumber > 3 ) {
      qDebug() << "Unknown priority:" << text;
    } else {
      int priority;
      switch ( priorityNumber ) {
        case 1:
          priority = 9;
          break;
        default:
        case 2:
          priority = 5;
          break;
        case 3:
          priority = 1;
          break;
      }
      todo->setPriority( priority );
    }
  } else if ( tagName == QLatin1String( "percent_completed" ) ) {
    todo->setPercentComplete( OXUtils::readNumber( element.text() ) );
  }
}

static void parseRecurrence( const QDomElement &element,
                             const KCalCore::Incidence::Ptr &incidence )
{
  QString type;

  int dailyValue = -1;
  KDateTime endDate;

  int weeklyValue = -1;
  QBitArray days( 7 ); // days, starting with monday
  bool daysSet = false;

  int monthlyValueDay = -1;
  int monthlyValueMonth = -1;

  int yearlyValueDay = -1;
  int yearlyMonth = -1;

  int monthly2Recurrency = 0;
  int monthly2ValueMonth = -1;

  int yearly2Recurrency = 0;
  int yearly2Day = 0;
  int yearly2Month = -1;

  KCalCore::DateList deleteExceptions;

  for ( QDomElement child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement() ) {
    const QString tagName = child.tagName();
    const QString text = OXUtils::readString( child.text() );

    if ( tagName == QLatin1String( "recurrence_type" ) ) {
      type = text;
    } else if ( tagName == QLatin1String( "interval" ) ) {
      dailyValue = text.toInt();
      weeklyValue = text.toInt();
      monthlyValueMonth = text.toInt();
      monthly2ValueMonth = text.toInt();
    } else if ( tagName == QLatin1String( "days" ) ) {
      int tmp = text.toInt();  // OX encodes days binary: 1=Su, 2=Mo, 4=Tu, ...
      for ( int i = 0; i < 7; ++i ) {
        if ( tmp & (1 << i) )
          days.setBit( (i + 6) % 7 );
      }
      daysSet = true;
    } else if ( tagName == QLatin1String( "day_in_month" ) ) {
      monthlyValueDay = text.toInt();
      monthly2Recurrency = text.toInt();
      yearlyValueDay = text.toInt();
      yearly2Recurrency = text.toInt();
    } else if ( tagName == QLatin1String( "month" ) ) {
      yearlyMonth = text.toInt() + 1; // starts at 0
      yearly2Month = text.toInt() + 1;
    } else if ( tagName == QLatin1String( "deleteexceptions" ) ) {
      const QStringList exceptionDates = text.split( QLatin1String( "," ) );
      foreach ( const QString &date, exceptionDates )
        deleteExceptions.append( OXUtils::readDateTime( date ).date() );
    } else if ( tagName == QLatin1String( "until" ) ) {
      endDate = OXUtils::readDateTime( child.text() );
    }
    //TODO: notification
  }

  if ( daysSet && type == QLatin1String( "monthly" ) )
    type = QLatin1String( "monthly2" ); // HACK: OX doesn't cleanly distinguish between monthly and monthly2
  if ( daysSet && type == QLatin1String( "yearly" ) )
    type = QLatin1String( "yearly2" );

  KCalCore::Recurrence *recurrence = incidence->recurrence();

  if ( type == QLatin1String( "daily" ) ) {
    recurrence->setDaily( dailyValue );
  } else if ( type == QLatin1String( "weekly" ) ) {
    recurrence->setWeekly( weeklyValue, days );
  } else if ( type == QLatin1String( "monthly" ) ) {
    recurrence->setMonthly( monthlyValueMonth );
    recurrence->addMonthlyDate( monthlyValueDay );
  } else if ( type == QLatin1String( "yearly" ) ) {
    recurrence->setYearly( 1 );
    recurrence->addYearlyDate( yearlyValueDay );
    recurrence->addYearlyMonth( yearlyMonth );
  } else if ( type == QLatin1String( "monthly2" ) ) {
    recurrence->setMonthly( monthly2ValueMonth );
    QBitArray _days( 7 );
    if ( daysSet )
      _days = days;
    else
      _days.setBit( incidence->dtStart().date().dayOfWeek() );
    recurrence->addMonthlyPos( monthly2Recurrency, _days );
  } else if ( type == QLatin1String( "yearly2" ) ) {
    recurrence->setYearly( 1 );
    recurrence->addYearlyMonth( yearly2Month );
    QBitArray _days( 7 );
    if ( daysSet )
      _days = days;
    else
      _days.setBit( ( yearly2Day + 5 ) % 7 );
    recurrence->addYearlyPos( yearly2Recurrency, _days );
  }

  if ( endDate.isValid() )
    recurrence->setEndDate( endDate.date() );

  recurrence->setExDates( deleteExceptions );
}


static void createIncidenceAttributes( QDomDocument &document, QDomElement &parent,
                                       const KCalCore::Incidence::Ptr &incidence )
{
  DAVUtils::addOxElement( document, parent, QLatin1String( "title" ), OXUtils::writeString( incidence->summary() ) );
  DAVUtils::addOxElement( document, parent, QLatin1String( "note" ), OXUtils::writeString( incidence->description() ) );

  if ( incidence->attendeeCount() > 0 ) {
    QDomElement members = DAVUtils::addOxElement( document, parent, QLatin1String( "participants" ) );
    const KCalCore::Attendee::List attendees = incidence->attendees();
    foreach ( const KCalCore::Attendee::Ptr attendee, attendees ) {
      const User user = Users::self()->lookupEmail( attendee->email() );

      if ( !user.isValid() )
        continue;

      QString status;
      switch ( attendee->status() ) {
        case KCalCore::Attendee::Accepted: status = QLatin1String( "accept" ); break;
        case KCalCore::Attendee::Declined: status = QLatin1String( "decline" ); break;
        default: status = QLatin1String( "none" ); break;
      }

      QDomElement element = DAVUtils::addOxElement( document, members, QLatin1String( "user" ), OXUtils::writeNumber( user.uid() ) );
      DAVUtils::setOxAttribute( element, "confirm", status );
    }
  }

  if ( incidence->secrecy() == KCalCore::Incidence::SecrecyPublic )
    DAVUtils::addOxElement( document, parent, QLatin1String( "private_flag" ), OXUtils::writeBoolean( false ) );
  else
    DAVUtils::addOxElement( document, parent, QLatin1String( "private_flag" ), OXUtils::writeBoolean( true ) );

  // set reminder as the number of minutes to the start of the event
  const KCalCore::Alarm::List alarms = incidence->alarms();
  if ( !alarms.isEmpty() && alarms.first()->hasStartOffset() && alarms.first()->enabled() ) {
    DAVUtils::addOxElement( document, parent, QLatin1String( "alarm_flag" ), OXUtils::writeBoolean( true ) );
    DAVUtils::addOxElement( document, parent, QLatin1String( "alarm" ), OXUtils::writeNumber( (-1) * alarms.first()->startOffset().asSeconds() / 60 ) );
  } else {
    DAVUtils::addOxElement( document, parent, QLatin1String( "alarm_flag" ), OXUtils::writeBoolean( false ) );
    DAVUtils::addOxElement( document, parent, QLatin1String( "alarm" ), QLatin1String( "0" ) );
  }

  // categories
  DAVUtils::addOxElement( document, parent, QLatin1String( "categories" ), OXUtils::writeString( incidence->categories().join( QLatin1String( ", " ) ) ) );
}

static void createEventAttributes( QDomDocument &document, QDomElement &parent,
                                   const KCalCore::Event::Ptr &event )
{
  DAVUtils::addOxElement( document, parent, QLatin1String( "start_date" ), OXUtils::writeDateTime( event->dtStart() ) );

  if ( event->hasEndDate() )
    DAVUtils::addOxElement( document, parent, QLatin1String( "end_date" ), OXUtils::writeDateTime( event->dtEnd() ) );
  else
    DAVUtils::addOxElement( document, parent, QLatin1String( "end_date" ) );

  DAVUtils::addOxElement( document, parent, QLatin1String( "location" ), OXUtils::writeString( event->location() ) );
  DAVUtils::addOxElement( document, parent, QLatin1String( "full_time" ), OXUtils::writeBoolean( event->allDay() ) );
}

static void createTaskAttributes( QDomDocument &document, QDomElement &parent,
                                  const KCalCore::Todo::Ptr &todo )
{
  if ( todo->hasStartDate() )
    DAVUtils::addOxElement( document, parent, QLatin1String( "start_date" ), OXUtils::writeDateTime( todo->dtStart() ) );
  else
    DAVUtils::addOxElement( document, parent, QLatin1String( "start_date" ) );

  if ( todo->hasDueDate() )
    DAVUtils::addOxElement( document, parent, QLatin1String( "end_date" ), OXUtils::writeDateTime( todo->dtDue() ) );
  else
    DAVUtils::addOxElement( document, parent, QLatin1String( "end_date" ) );

  QString priority;
  switch ( todo->priority() ) {
    case 9:
    case 8:
      priority = QLatin1String( "1" );
      break;
    case 2:
    case 1:
      priority = QLatin1String( "3" );
      break;
    default:
      priority = QLatin1String( "2" );
      break;
  }
  DAVUtils::addOxElement( document, parent, QLatin1String( "priority" ), priority );

  DAVUtils::addOxElement( document, parent, QLatin1String( "percent_completed" ), OXUtils::writeNumber( todo->percentComplete() ) );
}

static void createRecurrenceAttributes( QDomDocument &document, QDomElement &parent,
                                        const KCalCore::Incidence::Ptr &incidence )
{
  if ( !incidence->recurs() ) {
    DAVUtils::addOxElement( document, parent, QLatin1String( "recurrence_type" ), QLatin1String( "none" ) );
    return;
  }

  const KCalCore::Recurrence *recurrence = incidence->recurrence();
  int monthOffset = -1;
  switch ( recurrence->recurrenceType() ) {
    case KCalCore::Recurrence::rDaily:
      DAVUtils::addOxElement( document, parent, QLatin1String( "recurrence_type" ), QLatin1String( "daily" ) );
      DAVUtils::addOxElement( document, parent, QLatin1String( "interval" ), OXUtils::writeNumber( recurrence->frequency() ) );
      break;
    case KCalCore::Recurrence::rWeekly:
      {
        DAVUtils::addOxElement( document, parent, QLatin1String( "recurrence_type" ), QLatin1String( "weekly" ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "interval" ), OXUtils::writeNumber( recurrence->frequency() ) );

        int days = 0;
        for ( int i = 0; i < 7; ++i ) {
          if ( recurrence->days()[i] )
            days += 1 << ( ( i + 1 ) % 7 );
        }

        DAVUtils::addOxElement( document, parent, QLatin1String( "days" ), OXUtils::writeNumber( days ) );
      }
      break;
    case KCalCore::Recurrence::rMonthlyDay:
      DAVUtils::addOxElement( document, parent, QLatin1String( "recurrence_type" ), QLatin1String( "monthly" ) );
      DAVUtils::addOxElement( document, parent, QLatin1String( "interval" ), OXUtils::writeNumber( recurrence->frequency() ) );
      DAVUtils::addOxElement( document, parent, QLatin1String( "day_in_month" ), OXUtils::writeNumber( recurrence->monthDays().first() ) );
      break;
    case KCalCore::Recurrence::rMonthlyPos:
      {
        const KCalCore::RecurrenceRule::WDayPos wdp = recurrence->monthPositions().first();

        DAVUtils::addOxElement( document, parent, QLatin1String( "recurrence_type" ), QLatin1String( "monthly" ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "interval" ), OXUtils::writeNumber( recurrence->frequency() ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "days" ), OXUtils::writeNumber( 1 << wdp.day() ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "day_in_month" ), OXUtils::writeNumber( wdp.pos() ) );
      }
      break;
    case KCalCore::Recurrence::rYearlyMonth:
      DAVUtils::addOxElement( document, parent, QLatin1String( "recurrence_type" ), QLatin1String( "yearly" ) );
      DAVUtils::addOxElement( document, parent, QLatin1String( "interval" ), QLatin1String( "1" ) );
      DAVUtils::addOxElement( document, parent, QLatin1String( "day_in_month" ), OXUtils::writeNumber( recurrence->yearDates().first() ) );
      DAVUtils::addOxElement( document, parent, QLatin1String( "month" ), OXUtils::writeNumber( recurrence->yearMonths().first() + monthOffset ) );
      break;
    case KCalCore::Recurrence::rYearlyPos:
      {
        const KCalCore::RecurrenceRule::WDayPos wdp = recurrence->monthPositions().first();

        DAVUtils::addOxElement( document, parent, QLatin1String( "recurrence_type" ), QLatin1String( "yearly" ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "interval" ), QLatin1String( "1" ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "days" ), OXUtils::writeNumber( 1 << wdp.day() ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "day_in_month" ), OXUtils::writeNumber( wdp.pos() ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "month" ), OXUtils::writeNumber( recurrence->yearMonths().first() + monthOffset ) );
      }
      break;
    default:
      qDebug() << "unsupported recurrence type:" << recurrence->recurrenceType();
  }

  if ( recurrence->endDateTime().isValid() )
    DAVUtils::addOxElement( document, parent, QLatin1String( "until" ), OXUtils::writeDateTime( recurrence->endDateTime() ) );
  else
    DAVUtils::addOxElement( document, parent, QLatin1String( "until" ) );

  // delete exceptions
  const KCalCore::DateList exceptionList = recurrence->exDates();

  QStringList dates;
  foreach ( const QDate &date, exceptionList )
    dates.append( OXUtils::writeDateTime( KDateTime( date ) ) );

  DAVUtils::addOxElement( document, parent, QLatin1String( "deleteexceptions" ), dates.join( QLatin1String( "," ) ) );
}

void OXA::IncidenceUtils::parseEvent( const QDomElement &propElement, Object &object )
{
  KCalCore::Event::Ptr event( new KCalCore::Event );

  const QDomElement fullTimeElement = propElement.firstChildElement( "full_time" );
  if ( !fullTimeElement.isNull() )
    event->setAllDay( OXUtils::readBoolean( fullTimeElement.text() ) );

  bool doesRecur = false;
  const QDomElement recurrenceTypeElement = propElement.firstChildElement( "recurrence_type" );
  if ( !recurrenceTypeElement.isNull() && recurrenceTypeElement.text() != QLatin1String( "none" ) )
    doesRecur = true;

  QDomElement element = propElement.firstChildElement();
  while ( !element.isNull() ) {
    parseIncidenceAttribute( element, event );
    parseEventAttribute( element, event );

    element = element.nextSiblingElement();
  }

  if ( doesRecur )
    parseRecurrence( propElement, event );
  else
    event->recurrence()->unsetRecurs();

  object.setEvent( KCalCore::Incidence::Ptr( event ) );
}

void OXA::IncidenceUtils::parseTask( const QDomElement &propElement, Object &object )
{
  KCalCore::Todo::Ptr todo( new KCalCore::Todo );
  todo->setSecrecy( KCalCore::Incidence::SecrecyPrivate );

  bool doesRecur = false;
  const QDomElement recurrenceTypeElement = propElement.firstChildElement( "recurrence_type" );
  if ( !recurrenceTypeElement.isNull() && recurrenceTypeElement.text() != QLatin1String( "none" ) )
    doesRecur = true;

  QDomElement element = propElement.firstChildElement();
  while ( !element.isNull() ) {
    parseIncidenceAttribute( element, todo );
    parseTodoAttribute( element, todo );

    element = element.nextSiblingElement();
  }

  if ( doesRecur )
    parseRecurrence( propElement, todo );
  else
    todo->recurrence()->unsetRecurs();

  object.setTask( KCalCore::Incidence::Ptr( todo ) );
}

void OXA::IncidenceUtils::addEventElements( QDomDocument &document, QDomElement &propElement, const Object &object )
{
  createIncidenceAttributes( document, propElement, object.event() );
  createEventAttributes( document, propElement, object.event().staticCast<KCalCore::Event>() );
  createRecurrenceAttributes( document, propElement, object.event() );
}

void OXA::IncidenceUtils::addTaskElements( QDomDocument &document, QDomElement &propElement, const Object &object )
{
  createIncidenceAttributes( document, propElement, object.task() );
  createTaskAttributes( document, propElement, object.task().staticCast<KCalCore::Todo>() );
  createRecurrenceAttributes( document, propElement, object.task() );
}
