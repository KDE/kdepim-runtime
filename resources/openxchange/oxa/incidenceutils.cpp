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

#include "davutils.h"
#include "oxutils.h"

#include <kcal/event.h>
#include <kcal/todo.h>

#include <QtXml/QDomElement>

#include <QtCore/QDebug>

using namespace OXA;

static void parseMembersAttribute( const QDomElement &element, KCal::Incidence *incidence )
{
  incidence->clearAttendees();

  for ( QDomElement child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement() ) {
    if ( child.tagName() == QLatin1String( "Participant" ) ) {
      const QString member = child.text();

      KABC::Addressee account;
      /*FIXME:
      if ( mAccounts )
        account = mAccounts->lookupUser( member );
      else
        qDebug() << "KCalResourceSlox: no accounts set";
      */

      QString name;
      QString email;
      KCal::Attendee *attendee = incidence->attendeeByUid( member );
      if ( account.isEmpty() ) {
        if ( attendee )
          continue;

        name = member;
//FIXME:        email = member + '@' + KUrl( Settings::self()->baseUrl() ).host();
      } else {
        name = account.realName();
        email = account.preferredEmail();
      }

      if ( attendee ) {
        attendee->setName( name );
        attendee->setEmail( email );
      } else {
        attendee = new KCal::Attendee( name, email );
        attendee->setUid( member );
        incidence->addAttendee( attendee );
      }

      const QString status = child.attribute( "confirm" );
      if ( !status.isEmpty() ) {
        if ( status == QLatin1String( "accept" ) ) {
          attendee->setStatus( KCal::Attendee::Accepted );
        } else if ( status == QLatin1String( "decline" ) ) {
          attendee->setStatus( KCal::Attendee::Declined );
        } else {
          attendee->setStatus( KCal::Attendee::NeedsAction );
        }
      }
    } else {
      qDebug() << "Unknown tag in members attribute:" << child.tagName();
    }
  }
}

static void parseReadRightsAttribute( const QDomElement &element, KCal::Incidence *incidence )
{
  for ( QDomElement child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement() ) {
    if ( child.tagName() == QLatin1String( "group" ) ) {
      if ( child.text() == QLatin1String( "users" ) )
        incidence->setSecrecy( KCal::Incidence::SecrecyPublic );
    }
  }
}

static void parseIncidenceAttribute( const QDomElement &element, KCal::Incidence *incidence )
{
  const QString tagName = element.tagName();
  const QString text = OXUtils::readString( element.text() );

  if ( tagName == QLatin1String( "IncidenceTitle" ) ) {
    incidence->setSummary( text );
  } else if ( tagName == QLatin1String( "Description" ) ) {
    incidence->setDescription( text );
  } else if ( tagName == QLatin1String( "Reminder" ) ) {
    const int minutes = OXUtils::readNumber( element.text() );
    if ( minutes != 0 ) {
      KCal::Alarm::List alarms = incidence->alarms();
      KCal::Alarm *alarm;
      if ( alarms.isEmpty() )
        alarm = incidence->newAlarm();
      else
        alarm = alarms.first();

      if ( alarm->type() == KCal::Alarm::Invalid )
        alarm->setType( KCal::Alarm::Display );

      KCal::Duration duration( minutes * -60 );
      alarm->setStartOffset( duration );
      alarm->setEnabled( true );
    } else {
      // 0 reminder -> disable alarm
      incidence->clearAlarms();
    }
  } else if ( tagName == QLatin1String( "CreatedBy" ) ) {
    /*
    KABC::Addressee contact;
    if ( mAccounts )
      contact = mAccounts->lookupUser( text );
    else
      qDebug() << "KCalResourceSlox: no accounts set";
    incidence->setOrganizer( KCal::Person( contact.formattedName(), contact.preferredEmail() ) );
    */
  } else if ( tagName == QLatin1String( "Participants" ) ) {
    parseMembersAttribute( element, incidence );
  } else if ( tagName == QLatin1String( "readrights" ) ) {
    parseReadRightsAttribute( element, incidence );
  } else if ( tagName == QLatin1String( "categories" ) ) {
    incidence->setCategories( text.split( QRegExp( QLatin1String( ",\\s*" ) ) ) );
  }
}

static void parseEventAttribute( const QDomElement &element, KCal::Event *event )
{
  const QString tagName = element.tagName();
  const QString text = OXUtils::readString( element.text() );

  if ( tagName == QLatin1String( "EventBegin" ) ) {
    KDateTime dateTime = OXUtils::readDateTime( element.text() );
    if ( event->allDay() )
      dateTime.setDateOnly( true );

    event->setDtStart( dateTime );

  } else if ( tagName == QLatin1String( "EventEnd" ) ) {
    KDateTime dateTime = OXUtils::readDateTime( element.text() );
    if ( event->allDay() )
      dateTime = dateTime.addSecs( -1 );

    event->setDtEnd( dateTime );

  } else if ( tagName == QLatin1String( "Location" ) ) {
    event->setLocation( text );
  }
}

static void parseTodoAttribute( const QDomElement &element, KCal::Todo *todo )
{
  const QString tagName = element.tagName();
  const QString text = OXUtils::readString( element.text() );

  if ( tagName == QLatin1String( "TaskBegin" ) ) {
    const KDateTime dateTime = OXUtils::readDateTime( element.text() );
    if ( dateTime.isValid() ) {
      todo->setDtStart( dateTime );
      todo->setHasStartDate( true );
    }
  } else if ( tagName == QLatin1String( "TaskEnd" ) ) {
    const KDateTime dateTime = OXUtils::readDateTime( element.text() );
    if ( dateTime.isValid() ) {
      todo->setDtDue( dateTime );
      todo->setHasDueDate( true );
    }
  } else if ( tagName == QLatin1String( "Priority" ) ) {
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
  } else if ( tagName == QLatin1String( "PercentComplete" ) ) {
    todo->setPercentComplete( OXUtils::readNumber( element.text() ) );
  }
}

static void parseRecurrence( const QDomElement &element, KCal::Event *event )
{
  QString type;

  int dailyValue = -1;
  KDateTime end;

  int weeklyValue = -1;
  QBitArray days( 7 ); // days, starting with monday
  bool daysSet = false;

  int monthlyValueDay = -1;
  int monthlyValueMonth = -1;

  int yearlyValueDay = -1;
  int yearlyMonth = -1;

  int monthly2Recurrency = 0;
  int monthly2Day = 0;
  int monthly2ValueMonth = -1;

  int yearly2Recurrency = 0;
  int yearly2Day = 0;
  int yearly2Month = -1;

  KCal::DateList deleteExceptions;

  for ( QDomElement child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement() ) {
    const QString tagName = child.tagName();
    const QString text = OXUtils::readString( child.text() );

    if ( tagName == QLatin1String( "RecurrenceType" ) ) {
      type = text;
    } else if ( tagName == QLatin1String( "daily_value" ) ) {
      dailyValue = OXUtils::readNumber( child.text() );
    } else if ( tagName == QLatin1String( "RecurrenceEnd" ) ) {
      end = OXUtils::readDateTime( child.text() );
    } else if ( tagName == QLatin1String( "weekly_value" ) ) {
      weeklyValue = OXUtils::readNumber( child.text() );
    } else if ( tagName.startsWith( QLatin1String( "weekly_day_" ) ) ) {
      const int day = tagName.mid( 11, 1 ).toInt();
      if ( day == 1 )
        days.setBit( 0 );
      else
        days.setBit( day - 2 );
    } else if ( tagName == QLatin1String( "monthly_value_day" ) ) {
      monthlyValueDay = text.toInt();
    } else if ( tagName == QLatin1String( "monthly_value_month" ) ) {
      monthlyValueMonth = text.toInt();
    } else if ( tagName == QLatin1String( "yearly_value_day" ) ) {
      yearlyValueDay = text.toInt();
    } else if ( tagName == QLatin1String( "yearly_month" ) ) {
      yearlyMonth = text.toInt();
    } else if ( tagName == QLatin1String( "monthly2_recurrency" ) ) {
      monthly2Recurrency = text.toInt();
    } else if ( tagName == QLatin1String( "monthly2_day" ) ) {
      monthly2Day = text.toInt();
    } else if ( tagName == QLatin1String( "monthly2_value_month" ) ) {
      monthly2ValueMonth = text.toInt();
    } else if ( tagName == QLatin1String( "yearly2_reccurency" ) ) { // this is not a typo, this is what SLOX really sends!
      yearly2Recurrency = text.toInt();
    } else if ( tagName == QLatin1String( "yearly2_day" ) ) {
      yearly2Day = text.toInt();
    } else if ( tagName == QLatin1String( "yearly2_month" ) ) {
      yearly2Month = text.toInt() + 1;
    // OX recurrence fields
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
    } else if ( tagName == QLatin1String( "RecurrenceDelEx" ) ) {
      const QStringList exceptionDates = text.split( QLatin1String( "," ) );
      foreach ( const QString &date, exceptionDates )
        deleteExceptions.append( OXUtils::readDateTime( date ).date() );
    }
  }

  if ( daysSet && type == QLatin1String( "monthly" ) )
    type = QLatin1String( "monthly2" ); // HACK: OX doesn't cleanly distinguish between monthly and monthly2
  if ( daysSet && type == QLatin1String( "yearly" ) )
    type = QLatin1String( "yearly2" );

  KCal::Recurrence *recurrence = event->recurrence();

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
      _days.setBit( event->dtStart().date().dayOfWeek() );
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

  recurrence->setEndDate( end.date() );
  recurrence->setExDates( deleteExceptions );
}


static void createIncidenceAttributes( QDomDocument &document, QDomElement &parent, KCal::Incidence *incidence )
{
  DAVUtils::addOxElement( document, parent, QLatin1String( "IncidenceTitle" ), OXUtils::writeString( incidence->summary() ) );
  DAVUtils::addOxElement( document, parent, QLatin1String( "Description" ), OXUtils::writeString( incidence->description() ) );

  if ( incidence->attendeeCount() > 0 ) {
    QDomElement members = DAVUtils::addOxElement( document, parent, QLatin1String( "Participants" ) );
    const KCal::Attendee::List attendees = incidence->attendees();
    foreach ( const KCal::Attendee *attendee, attendees ) {
      /*
      if ( mAccounts ) {
        const QString userId = mAccounts->lookupId( attendee->email() );
        QString status;
        switch ( attendee->status() ) {
          case KCal::Attendee::Accepted: status = QLatin1String( "accept" ); break;
          case KCal::Attendee::Declined: status = QLatin1String( "decline" ); break;
          default: status = QLatin1String( "none" ); break;
        }
        QDomElement element = DAVUtils::addOxElement( document, members, QLatin1String( "Participant" ), userId );
        element.setAttribute( "confirm", status ); //TODO: ox attr?
      } else {
        qDebug() << "KCalResourceSlox: No accounts set.";
      }
      */
    }
  }

/*
  // set read attributes - if SecrecyPublic, set it to users
  // TODO OX support
  if ( incidence->secrecy() == Incidence::SecrecyPublic && type() != "ox" )
  {
    QDomElement rights = WebdavHandler::addSloxElement( this, doc, parent, "readrights" );
    WebdavHandler::addSloxElement( this, doc, rights, "group", "users" );
  }
*/

  // set reminder as the number of minutes to the start of the event
  const KCal::Alarm::List alarms = incidence->alarms();
  if ( !alarms.isEmpty() && alarms.first()->hasStartOffset() && alarms.first()->enabled() )
    DAVUtils::addOxElement( document, parent, QLatin1String( "Reminder" ), OXUtils::writeNumber( (-1) * alarms.first()->startOffset().asSeconds() / 60 ) );
  else
    DAVUtils::addOxElement( document, parent, QLatin1String( "Reminder" ), QLatin1String( "0" ) );

  // categories
  DAVUtils::addOxElement( document, parent, QLatin1String( "Categories" ), OXUtils::writeString( incidence->categories().join( QLatin1String( ", " ) ) ) );
}

static void createEventAttributes( QDomDocument &document, QDomElement &parent, KCal::Event *event )
{
  DAVUtils::addOxElement( document, parent, QLatin1String( "EventBegin" ), OXUtils::writeDateTime( event->dtStart() ) );
  DAVUtils::addOxElement( document, parent, QLatin1String( "EventEnd" ), OXUtils::writeDateTime( event->dtEnd() ) );
  DAVUtils::addOxElement( document, parent, QLatin1String( "Location" ), OXUtils::writeString( event->location() ) );
  DAVUtils::addOxElement( document, parent, QLatin1String( "FullTime" ), OXUtils::writeBoolean( event->allDay() ) );
}

static void createTaskAttributes( QDomDocument &document, QDomElement &parent, KCal::Todo *todo )
{
  if ( todo->hasStartDate() )
    DAVUtils::addOxElement( document, parent, QLatin1String( "TaskBegin" ), OXUtils::writeDateTime( todo->dtStart() ) );

  if ( todo->hasDueDate() )
    DAVUtils::addOxElement( document, parent, QLatin1String( "TaskEnd" ), OXUtils::writeDateTime( todo->dtDue() ) );

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
  DAVUtils::addOxElement( document, parent, QLatin1String( "Priority" ), priority );

  DAVUtils::addOxElement( document, parent, QLatin1String( "PercentComplete" ), OXUtils::writeNumber( todo->percentComplete() ) );
}

static void createRecurrenceAttributes( QDomDocument &document, QDomElement &parent, KCal::Incidence *incidence )
{
  if ( !incidence->recurs() ) {
    DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceType" ), QLatin1String( "none" ) );
    return;
  }

  const KCal::Recurrence *recurrence = incidence->recurrence();
  int monthOffset = -1;
  switch ( recurrence->recurrenceType() ) {
    case KCal::Recurrence::rDaily:
      DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceType" ), QLatin1String( "daily" ) );
      DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceDailyFreq" ), OXUtils::writeNumber( recurrence->frequency() ) );
      break;
    case KCal::Recurrence::rWeekly:
      {
        DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceType" ), QLatin1String( "weekly" ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceWeeklyFreq" ), OXUtils::writeNumber( recurrence->frequency() ) );

        int days = 0;
        for ( int i = 0; i < 7; ++i ) {
          if ( recurrence->days()[i] )
            days += 1 << ( ( i + 1 ) % 7 );
        }

        DAVUtils::addOxElement( document, parent, QLatin1String( "days" ), OXUtils::writeNumber( days ) );
      }
      break;
    case KCal::Recurrence::rMonthlyDay:
      DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceType" ), QLatin1String( "monthly" ) );
      DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceMonthlyFreq" ), OXUtils::writeNumber( recurrence->frequency() ) );
      DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceMonthlyDay" ), OXUtils::writeNumber( recurrence->monthDays().first() ) );
      break;
    case KCal::Recurrence::rMonthlyPos:
      {
        const KCal::RecurrenceRule::WDayPos wdp = recurrence->monthPositions().first();

        DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceType" ), QLatin1String( "monthly" ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceMonthly2Freq" ), OXUtils::writeNumber( recurrence->frequency() ) );

        DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceMonthly2Day" ), OXUtils::writeNumber( 1 << wdp.day() ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceMonthly2Pos" ), OXUtils::writeNumber( wdp.pos() ) );
      }
      break;
    case KCal::Recurrence::rYearlyMonth:
      DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceType" ), QLatin1String( "yearly" ) );
      DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceYearlyDay" ), OXUtils::writeNumber( recurrence->yearDates().first() ) );
      DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceYearlyMonth" ), OXUtils::writeNumber( recurrence->yearMonths().first() + monthOffset ) );
      DAVUtils::addOxElement( document, parent, QLatin1String( "interval" ), QLatin1String( "1" ) );
      break;
    case KCal::Recurrence::rYearlyPos:
      {
        const KCal::RecurrenceRule::WDayPos wdp = recurrence->monthPositions().first();

        DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceType" ), QLatin1String( "yearly" ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceYearly2Day" ), OXUtils::writeNumber( 1 << wdp.day() ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceYearly2Pos" ), OXUtils::writeNumber( wdp.pos() ) );
        DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceYearly2Month" ), OXUtils::writeNumber( recurrence->yearMonths().first() + monthOffset ) );
        DAVUtils::addOxElement( document, parent, "interval", "1" );
      }
      break;
    default:
      qDebug() << "unsupported recurrence type:" << recurrence->recurrenceType();
  }

  DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceEnd" ), OXUtils::writeDateTime( recurrence->endDateTime() ) );

  // delete exceptions
  const KCal::DateList exceptionList = recurrence->exDates();

  QStringList dates;
  foreach ( const QDate &date, exceptionList )
    dates.append( OXUtils::writeDateTime( KDateTime( date ) ) );

  DAVUtils::addOxElement( document, parent, QLatin1String( "RecurrenceDelEx" ), dates.join( QLatin1String( "," ) ) );
}

void OXA::IncidenceUtils::parseEvent( const QDomElement &propElement, Object &object )
{
  KCal::Event *event = new KCal::Event;

  bool doesRecur = false;
  QDomElement element = propElement.firstChildElement();
  while ( !element.isNull() ) {
    parseIncidenceAttribute( element, event );
    parseEventAttribute( element, event );

    if ( element.tagName() == QLatin1String( "RecurrenceType" ) ) {
      if ( element.text() != QLatin1String( "none" ) )
            doesRecur = true;
    }

    element = element.nextSiblingElement();
  }

  if ( doesRecur )
    parseRecurrence( propElement, event );
  else
    event->recurrence()->unsetRecurs();

  object.setEvent( KCal::Incidence::Ptr( event ) );
}

void OXA::IncidenceUtils::parseTask( const QDomElement &propElement, Object &object )
{
  KCal::Todo *todo = new KCal::Todo;
  todo->setSecrecy( KCal::Incidence::SecrecyPrivate );

  QDomElement element = propElement.firstChildElement();
  while ( !element.isNull() ) {
    parseIncidenceAttribute( element, todo );
    parseTodoAttribute( element, todo );

    element = element.nextSiblingElement();
  }

  object.setTask( KCal::Incidence::Ptr( todo ) );
}

void OXA::IncidenceUtils::addEventElements( QDomDocument &document, QDomElement &propElement, const Object &object )
{
  createIncidenceAttributes( document, propElement, object.event().get() );
  createEventAttributes( document, propElement, static_cast<KCal::Event*>( object.event().get() ) );
  createRecurrenceAttributes( document, propElement, object.event().get() );
}

void OXA::IncidenceUtils::addTaskElements( QDomDocument &document, QDomElement &propElement, const Object &object )
{
  createIncidenceAttributes( document, propElement, object.task().get() );
  createTaskAttributes( document, propElement, static_cast<KCal::Todo*>( object.event().get() ) );
}
