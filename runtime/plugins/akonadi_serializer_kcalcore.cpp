/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "akonadi_serializer_kcalcore.h"

#include <akonadi/abstractdifferencesreporter.h>
#include <akonadi/item.h>

#include <KCalCore/Event>
#include <KCalCore/Todo>

#include <KCalUtils/Stringify>

#include <kdebug.h>
#include <klocale.h>

#include <QtCore/qplugin.h>

using namespace KCalCore;
using namespace KCalUtils;
using namespace Akonadi;

//// ItemSerializerPlugin interface

bool SerializerPluginKCalCore::deserialize(Item & item, const QByteArray & label, QIODevice & data, int version)
{
  Q_UNUSED( version );

  if ( label != Item::FullPayload ) {
    return false;
  }

  Incidence::Ptr i = mFormat.fromString( data.readAll() );
  if ( !i ) {
    kWarning( 5263 ) << "Failed to parse incidence!";
    data.seek( 0 );
    kWarning( 5263 ) << QString::fromUtf8( data.readAll() );
    return false;
  }
  item.setPayload( i );
  return true;
}

void SerializerPluginKCalCore::serialize(const Item & item, const QByteArray & label, QIODevice & data, int &version)
{
  Q_UNUSED( version );

  if ( label != Item::FullPayload || !item.hasPayload<Incidence::Ptr>() )
    return;
  Incidence::Ptr i = item.payload<Incidence::Ptr>();
  // ### I guess this can be done without hardcoding stuff
  data.write( "BEGIN:VCALENDAR\nPRODID:-//K Desktop Environment//NONSGML libkcal 3.2//EN\nVERSION:2.0\n" );
  data.write( mFormat.toString( i ).toUtf8() );
  data.write( "\nEND:VCALENDAR" );
}

//// DifferencesAlgorithmInterface

static bool compareString( const QString &left, const QString &right )
{
  if ( left.isEmpty() && right.isEmpty() )
    return true;
  else
    return left == right;
}

static QString toString( const Attendee::Ptr &attendee )
{
  return attendee->name() + QLatin1Char( '<' ) + attendee->email() + QLatin1Char( '>' );
}

static QString toString( const Alarm::Ptr & )
{
  return QString();
}

static QString toString( const Incidence::Ptr & )
{
  return QString();
}

static QString toString( const Attachment::Ptr & )
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

static QString toString( const QString &str )
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

template <class C>
static void compareList( AbstractDifferencesReporter *reporter,
                         const QString &id,
                         const C &left,
                         const C &right )
{
  for ( typename C::const_iterator it = left.begin(), end = left.end() ; it != end ; ++it ) {
    if ( !right.contains( *it )  )
      reporter->addProperty( AbstractDifferencesReporter::AdditionalLeftMode, id, toString( *it ), QString() );
  }

  for ( typename C::const_iterator it = right.begin(), end = right.end() ; it != end ; ++it ) {
    if ( !left.contains( *it )  )
      reporter->addProperty( AbstractDifferencesReporter::AdditionalRightMode, id, QString(), toString( *it ) );
  }
}

static void compareIncidenceBase( AbstractDifferencesReporter *reporter,
                                  const IncidenceBase::Ptr &left,
                                  const IncidenceBase::Ptr &right )
{
  compareList( reporter, i18n( "Attendees" ), left->attendees(), right->attendees() );

  if ( !compareString( left->organizer()->fullName(), right->organizer()->fullName() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Organizer" ),
                            left->organizer()->fullName(), right->organizer()->fullName() );

  if ( !compareString( left->uid(), right->uid() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "UID" ),
                            left->uid(), right->uid() );

  if ( left->allDay() != right->allDay() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Is all-day" ),
                            toString( left->allDay() ), toString( right->allDay() ) );

  if ( left->hasDuration() != right->hasDuration() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Has duration" ),
                            toString( left->hasDuration() ), toString( right->hasDuration() ) );

  if ( left->duration() != right->duration() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Duration" ),
                            QString::number( left->duration().asSeconds() ), QString::number( right->duration().asSeconds() ) );
}

static void compareIncidence( AbstractDifferencesReporter *reporter,
                              const Incidence::Ptr &left,
                              const Incidence::Ptr &right )
{
  if ( !compareString( left->description(), right->description() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Description" ),
                            left->description(), right->description() );

  if ( !compareString( left->summary(), right->summary() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Summary" ),
                            left->summary(), right->summary() );

  if ( left->status() != right->status() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Status" ),
                           Stringify::incidenceStatus( left ), Stringify::incidenceStatus( right ) );

  if ( left->secrecy() != right->secrecy() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Secrecy" ),
                            toString( left->secrecy() ), toString( right->secrecy() ) );

  if ( left->priority() != right->priority() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Priority" ),
                            toString( left->priority() ), toString( right->priority() ) );

  if ( !compareString( left->location(), right->location() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Location" ),
                            left->location(), right->location() );

  compareList( reporter, i18n( "Categories" ), left->categories(), right->categories() );
  compareList( reporter, i18n( "Alarms" ), left->alarms(), right->alarms() );
  compareList( reporter, i18n( "Resources" ), left->resources(), right->resources() );
  compareList( reporter, i18n( "Attachments" ), left->attachments(), right->attachments() );
  compareList( reporter, i18n( "Exception Dates" ), left->recurrence()->exDates(), right->recurrence()->exDates() );
  compareList( reporter, i18n( "Exception Times" ), left->recurrence()->exDateTimes(), right->recurrence()->exDateTimes() );
  // TODO: recurrence dates and date/times, exrules, rrules

  if ( left->created() != right->created() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode,
                            i18n( "Created" ), left->created().toString(), right->created().toString() );

  if ( !compareString( left->relatedTo(), right->relatedTo() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode,
                            i18n( "Related Uid" ), left->relatedTo(), right->relatedTo() );
}

static void compareEvent( AbstractDifferencesReporter *reporter,
                          const Event::Ptr &left,
                          const Event::Ptr &right )
{

  if ( left->dtStart() != right->dtStart() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Start time" ),
                            left->dtStart().toString(), right->dtStart().toString() );

  if ( left->hasEndDate() != right->hasEndDate() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Has End Date" ),
                            toString( left->hasEndDate() ), toString( right->hasEndDate() ) );

  if ( left->dtEnd() != right->dtEnd() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "End Date" ),
                            left->dtEnd().toString(), right->dtEnd().toString() );

  // TODO: check transparency
}

static void compareTodo( AbstractDifferencesReporter *reporter,
                         const Todo::Ptr &left,
                         const Todo::Ptr &right )
{
  if ( left->hasStartDate() != right->hasStartDate() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Has Start Date" ),
                            toString( left->hasStartDate() ), toString( right->hasStartDate() ) );

  if ( left->hasDueDate() != right->hasDueDate() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Has Due Date" ),
                            toString( left->hasDueDate() ), toString( right->hasDueDate() ) );

  if ( left->dtDue() != right->dtDue() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Due Date" ),
                            left->dtDue().toString(), right->dtDue().toString() );

  if ( left->hasCompletedDate() != right->hasCompletedDate() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Has Complete Date" ),
                            toString( left->hasCompletedDate() ), toString( right->hasCompletedDate() ) );

  if ( left->percentComplete() != right->percentComplete() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Complete" ),
                            QString::number( left->percentComplete() ), QString::number( right->percentComplete() ) );

  if ( left->completed() != right->completed() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Completed" ),
                            toString( left->completed() ), toString( right->completed() ) );
}

void SerializerPluginKCalCore::compare( Akonadi::AbstractDifferencesReporter *reporter,
                                        const Akonadi::Item &leftItem,
                                        const Akonadi::Item &rightItem )
{
  Q_ASSERT( reporter );
  Q_ASSERT( leftItem.hasPayload<Incidence::Ptr>() );
  Q_ASSERT( rightItem.hasPayload<Incidence::Ptr>() );

  const Incidence::Ptr leftIncidencePtr = leftItem.payload<Incidence::Ptr>();
  const Incidence::Ptr rightIncidencePtr = rightItem.payload<Incidence::Ptr>();

  if ( leftIncidencePtr->type() == Incidence::TypeEvent  ) {
    reporter->setLeftPropertyValueTitle( i18n( "Changed Event" ) );
    reporter->setRightPropertyValueTitle( i18n( "Conflicting Event" ) );
  } else if ( leftIncidencePtr->type() == Incidence::TypeTodo ) {
    reporter->setLeftPropertyValueTitle( i18n( "Changed Todo" ) );
    reporter->setRightPropertyValueTitle( i18n( "Conflicting Todo" ) );
  }

  compareIncidenceBase( reporter, leftIncidencePtr, rightIncidencePtr );
  compareIncidence( reporter, leftIncidencePtr, rightIncidencePtr );

  const Event::Ptr leftEvent = leftIncidencePtr.dynamicCast<Event>() ;
  const Event::Ptr rightEvent = rightIncidencePtr.dynamicCast<Event>() ;
  if ( leftEvent && rightEvent ) {
    compareEvent( reporter, leftEvent, rightEvent );
  } else {
    const Todo::Ptr leftTodo =  leftIncidencePtr.dynamicCast<Todo>();
    const Todo::Ptr rightTodo = rightIncidencePtr.dynamicCast<Todo>();
    if ( leftTodo && rightTodo ) {
      compareTodo( reporter, leftTodo, rightTodo );
    }
  }
}

Q_EXPORT_PLUGIN2( akonadi_serializer_kcalcore, SerializerPluginKCalCore )

#include "akonadi_serializer_kcalcore.moc"
