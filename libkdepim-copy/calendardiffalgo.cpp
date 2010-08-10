/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

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

#include "calendardiffalgo.h"

#include <kcalutils/stringify.h>

#include <KDateTime>
#include <KLocale>

using namespace KPIM;

#ifndef KDE_USE_FINAL
static bool compareString( const QString &left, const QString &right )
{
  if ( left.isEmpty() && right.isEmpty() )
    return true;
  else
    return left == right;
}
#endif

static QString toString( KCalCore::Attendee *attendee )
{
  return attendee->name() + '<' + attendee->email() + '>';
}

static QString toString( KCalCore::Alarm * )
{
  return QString();
}

static QString toString( KCalCore::Incidence * )
{
  return QString();
}

static QString toString( KCalCore::Attachment * )
{
  return QString();
}

static QString toString( const QDate &date )
{
  return date.toString();
}

static QString toString( const KDateTime &dateTime )
{
  return dateTime.dateTime().toString();
}

static QString toString( const QString str )
{
  return str;
}

static QString toString( bool value )
{
  if ( value )
    return i18n( "Yes" );
  else
    return i18n( "No" );
}

CalendarDiffAlgo::CalendarDiffAlgo( const KCalCore::Incidence::Ptr &leftIncidence,
                                    const KCalCore::Incidence::Ptr &rightIncidence )
  : mLeftIncidence( leftIncidence ), mRightIncidence( rightIncidence )
{
}

void CalendarDiffAlgo::run()
{
  begin();

  diffIncidenceBase( mLeftIncidence, mRightIncidence );
  diffIncidence( mLeftIncidence, mRightIncidence );

  KCalCore::Event::Ptr leftEvent = mLeftIncidence.dynamicCast<KCalCore::Event>();
  KCalCore::Event::Ptr rightEvent = mRightIncidence.dynamicCast<KCalCore::Event>();
  if ( leftEvent && rightEvent ) {
    diffEvent( leftEvent, rightEvent );
  } else {
    KCalCore::Todo::Ptr leftTodo = mLeftIncidence.dynamicCast<KCalCore::Todo>();
    KCalCore::Todo::Ptr rightTodo = mRightIncidence.dynamicCast<KCalCore::Todo>();
    if ( leftTodo && rightTodo ) {
      diffTodo( leftTodo, rightTodo );
    }
  }

  end();
}

void CalendarDiffAlgo::diffIncidenceBase( const KCalCore::IncidenceBase::Ptr &left, const KCalCore::IncidenceBase::Ptr &right )
{
  diffList( i18n( "Attendees" ), left->attendees(), right->attendees() );

  if ( left->dtStart() != right->dtStart() )
    conflictField( i18n( "Start time" ), left->dtStart().toString(), right->dtStart().toString() );

  if ( !compareString( left->organizer()->fullName(), right->organizer()->fullName() ) )
    conflictField( i18n( "Organizer" ), left->organizer()->fullName(), right->organizer()->fullName() );

  if ( !compareString( left->uid(), right->uid() ) )
    conflictField( i18n( "UID" ), left->uid(), right->uid() );

  if ( left->allDay() != right->allDay() )
    conflictField( i18n( "Is all-day" ), toString( left->allDay() ), toString( right->allDay() ) );

  if ( left->hasDuration() != right->hasDuration() )
    conflictField( i18n( "Has duration" ), toString( left->hasDuration() ), toString( right->hasDuration() ) );

  if ( left->duration() != right->duration() )
    conflictField( i18n( "Duration" ), QString::number( left->duration().asSeconds() ), QString::number( right->duration().asSeconds() ) );
}

void CalendarDiffAlgo::diffIncidence( const KCalCore::Incidence::Ptr &left,
                                      const KCalCore::Incidence::Ptr &right )
{
  if ( !compareString( left->description(), right->description() ) )
    conflictField( i18n( "Description" ), left->description(), right->description() );

  if ( !compareString( left->summary(), right->summary() ) )
    conflictField( i18n( "Summary" ), left->summary(), right->summary() );

  if ( left->status() != right->status() )
    conflictField( i18n( "Status" ),
                   KCalUtils::Stringify::incidenceStatus( left->status() ),
                   KCalUtils::Stringify::incidenceStatus( right->status() ) );


  if ( left->secrecy() != right->secrecy() )
    conflictField( i18n( "Secrecy" ), toString( left->secrecy() ), toString( right->secrecy() ) );

  if ( left->priority() != right->priority() )
    conflictField( i18n( "Priority" ), toString( left->priority() ), toString( right->priority() ) );

  if ( !compareString( left->location(), right->location() ) )
    conflictField( i18n( "Location" ), left->location(), right->location() );

  diffList( i18n( "Categories" ), left->categories(), right->categories() );
  diffList( i18n( "Alarms" ), left->alarms(), right->alarms() );
  diffList( i18n( "Resources" ), left->resources(), right->resources() );
  diffList( i18n( "Attachments" ), left->attachments(), right->attachments() );
  diffList( i18n( "Exception Dates" ), left->recurrence()->exDates(), right->recurrence()->exDates() );
  diffList( i18n( "Exception Times" ), left->recurrence()->exDateTimes(), right->recurrence()->exDateTimes() );
	// TODO: recurrence dates and date/times, exrules, rrules

  if ( left->created() != right->created() )
    conflictField( i18n( "Created" ), left->created().toString(), right->created().toString() );

  if ( !compareString( left->relatedTo(), right->relatedTo() ) )
    conflictField( i18n( "Related Uid" ), left->relatedTo(), right->relatedTo() );
}

void CalendarDiffAlgo::diffEvent( const KCalCore::Event::Ptr &left, const KCalCore::Event::Ptr &right )
{
  if ( left->hasEndDate() != right->hasEndDate() )
    conflictField( i18n( "Has End Date" ), toString( left->hasEndDate() ), toString( right->hasEndDate() ) );

  if ( left->dtEnd() != right->dtEnd() )
    conflictField( i18n( "End Date" ), left->dtEnd().toString(), right->dtEnd().toString() );

  // TODO: check transparency
}

void CalendarDiffAlgo::diffTodo( const KCalCore::Todo::Ptr &left, const KCalCore::Todo::Ptr &right )
{
  if ( left->hasStartDate() != right->hasStartDate() )
    conflictField( i18n( "Has Start Date" ), toString( left->hasStartDate() ), toString( right->hasStartDate() ) );

  if ( left->hasDueDate() != right->hasDueDate() )
    conflictField( i18n( "Has Due Date" ), toString( left->hasDueDate() ), toString( right->hasDueDate() ) );

  if ( left->dtDue() != right->dtDue() )
    conflictField( i18n( "Due Date" ), left->dtDue().toString(), right->dtDue().toString() );

  if ( left->hasCompletedDate() != right->hasCompletedDate() )
    conflictField( i18n( "Has Complete Date" ), toString( left->hasCompletedDate() ), toString( right->hasCompletedDate() ) );

  if ( left->percentComplete() != right->percentComplete() )
    conflictField( i18n( "Complete" ), QString::number( left->percentComplete() ), QString::number( right->percentComplete() ) );

  if ( left->completed() != right->completed() )
    conflictField( i18n( "Completed" ), toString( left->completed() ), toString( right->completed() ) );
}

template <class L>
void CalendarDiffAlgo::diffList( const QString &id,
                                 const QList<L> &left, const QList<L> &right )
{
  for ( int i = 0; i < left.count(); ++i ) {
    if ( !right.contains( left[ i ] )  )
      additionalLeftField( id, toString( left[ i ] ) );
  }

  for ( int i = 0; i < right.count(); ++i ) {
    if ( !left.contains( right[ i ] )  )
      additionalRightField( id, toString( right[ i ] ) );
  }
}
