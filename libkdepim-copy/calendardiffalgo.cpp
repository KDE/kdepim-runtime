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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include <klocale.h>

#include "calendardiffalgo.h"

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

static QString toString( KCal::Attendee *attendee )
{
  return attendee->name() + "<" + attendee->email() + ">";
}

static QString toString( KCal::Alarm *alarm )
{
  return QString::null;
}

static QString toString( KCal::Incidence *incidence )
{
  return QString::null;
}

static QString toString( KCal::Attachment *attachment )
{
  return QString::null;
}

static QString toString( const QDate &date )
{
  return date.toString();
}

static QString toString( const QDateTime &dateTime )
{
  return dateTime.toString();
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

CalendarDiffAlgo::CalendarDiffAlgo( KCal::Incidence *leftIncidence,
                                    KCal::Incidence *rightIncidence )
  : mLeftIncidence( leftIncidence ), mRightIncidence( rightIncidence )
{
}

void CalendarDiffAlgo::run()
{
  begin();

  diffIncidenceBase( mLeftIncidence, mRightIncidence );
  diffIncidence( mLeftIncidence, mRightIncidence );

  KCal::Event *leftEvent = dynamic_cast<KCal::Event*>( mLeftIncidence );
  KCal::Event *rightEvent = dynamic_cast<KCal::Event*>( mRightIncidence );
  if ( leftEvent && rightEvent ) {
    diffEvent( leftEvent, rightEvent );
  } else {
    KCal::Todo *leftTodo = dynamic_cast<KCal::Todo*>( mLeftIncidence );
    KCal::Todo *rightTodo = dynamic_cast<KCal::Todo*>( mRightIncidence );
    if ( leftTodo && rightTodo ) {
      diffTodo( leftTodo, rightTodo );
    }
  }

  end();
}

void CalendarDiffAlgo::diffIncidenceBase( KCal::IncidenceBase *left, KCal::IncidenceBase *right )
{
  diffList( i18n( "Attendees" ), left->attendees(), right->attendees() );

  if ( left->dtStart() != right->dtStart() )
    conflictField( i18n( "Start time" ), left->dtStartStr(), right->dtStartStr() );

  if ( !compareString( left->organizer().fullName(), right->organizer().fullName() ) )
    conflictField( i18n( "Organizer" ), left->organizer().fullName(), right->organizer().fullName() );

  if ( !compareString( left->uid(), right->uid() ) )
    conflictField( i18n( "UID" ), left->uid(), right->uid() );

  if ( left->doesFloat() != right->doesFloat() )
    conflictField( i18n( "Is floating" ), toString( left->doesFloat() ), toString( right->doesFloat() ) );

  if ( left->hasDuration() != right->hasDuration() )
    conflictField( i18n( "Has duration" ), toString( left->hasDuration() ), toString( right->hasDuration() ) );

  if ( left->duration() != right->duration() )
    conflictField( i18n( "Duration" ), QString::number( left->duration() ), QString::number( right->duration() ) );
}

void CalendarDiffAlgo::diffIncidence( KCal::Incidence *left, KCal::Incidence *right )
{
  if ( !compareString( left->description(), right->description() ) )
    conflictField( i18n( "Description" ), left->description(), right->description() );

  if ( !compareString( left->summary(), right->summary() ) )
    conflictField( i18n( "Summary" ), left->summary(), right->summary() );

  if ( left->status() != right->status() )
    conflictField( i18n( "Status" ), left->statusStr(), right->statusStr() );

  if ( left->secrecy() != right->secrecy() )
    conflictField( i18n( "Secrecy" ), toString( left->secrecy() ), toString( right->secrecy() ) );

  if ( left->priority() != right->priority() )
    conflictField( i18n( "Priority" ), toString( left->priority() ), toString( right->priority() ) );

  if ( !compareString( left->location(), right->location() ) )
    conflictField( i18n( "Location" ), left->location(), right->location() );
  
  diffList( i18n( "Categories" ), left->categories(), right->categories() );
  diffList( i18n( "Alarms" ), left->alarms(), right->alarms() );
  diffList( i18n( "Resources" ), left->resources(), right->resources() );
  diffList( i18n( "Relations" ), left->relations(), right->relations() );
  diffList( i18n( "Attachments" ), left->attachments(), right->attachments() );
  diffList( i18n( "Exception Dates" ), left->exDates(), right->exDates() );
  diffList( i18n( "Exception Times" ), left->exDateTimes(), right->exDateTimes() );

  if ( left->created() != right->created() )
    conflictField( i18n( "Created" ), left->created().toString(), right->created().toString() );

  if ( !compareString( left->relatedToUid(), right->relatedToUid() ) )
    conflictField( i18n( "Related Uid" ), left->relatedToUid(), right->relatedToUid() );
}

void CalendarDiffAlgo::diffEvent( KCal::Event *left, KCal::Event *right )
{
  if ( left->hasEndDate() != right->hasEndDate() )
    conflictField( i18n( "Has End Date" ), toString( left->hasEndDate() ), toString( right->hasEndDate() ) );

  if ( left->dtEnd() != right->dtEnd() )
    conflictField( i18n( "End Date" ), left->dtEndStr(), right->dtEndStr() );

  // TODO: check transparency
}

void CalendarDiffAlgo::diffTodo( KCal::Todo *left, KCal::Todo *right )
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
                                 const QValueList<L> &left, const QValueList<L> &right )
{
  for ( uint i = 0; i < left.count(); ++i ) {
    if ( right.find( left[ i ] ) == right.end() )
      additionalLeftField( id, toString( left[ i ] ) );
  }

  for ( uint i = 0; i < right.count(); ++i ) {
    if ( left.find( right[ i ] ) == left.end() )
      additionalRightField( id, toString( right[ i ] ) );
  }
}
