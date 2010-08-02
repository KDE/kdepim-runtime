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

#include "akonadi_serializer_kcal.h"

#include <boost/shared_ptr.hpp>

#include <akonadi/abstractdifferencesreporter.h>
#include <akonadi/item.h>
#include <kcal/event.h>
#include <kcal/todo.h>
#include <kdebug.h>
#include <klocale.h>

#include <QtCore/qplugin.h>

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

using namespace Akonadi;

//// ItemSerializerPlugin interface

bool SerializerPluginKCal::deserialize(Item & item, const QByteArray & label, QIODevice & data, int version)
{
  Q_UNUSED( version );

  if ( label != Item::FullPayload )
    return false;

  KCal::Incidence* i = mFormat.fromString( QString::fromUtf8( data.readAll() ) );
  if ( !i ) {
    kWarning( 5263 ) << "Failed to parse incidence!";
    data.seek( 0 );
    kWarning( 5263 ) << QString::fromUtf8( data.readAll() );
    return false;
  }
  item.setPayload<IncidencePtr>( IncidencePtr( i ) );
  return true;
}

void SerializerPluginKCal::serialize(const Item & item, const QByteArray & label, QIODevice & data, int &version)
{
  Q_UNUSED( version );

  if ( label != Item::FullPayload || !item.hasPayload<IncidencePtr>() )
    return;
  IncidencePtr i = item.payload<IncidencePtr>();
  // ### I guess this can be done without hardcoding stuff
  data.write( "BEGIN:VCALENDAR\nPRODID:-//K Desktop Environment//NONSGML libkcal 3.2//EN\nVERSION:2.0\n" );
  data.write( mFormat.toString( i.get() ).toUtf8() );
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

static QString toString( KCal::Attendee *attendee )
{
  return attendee->name() + QLatin1Char( '<' ) + attendee->email() + QLatin1Char( '>' );
}

static QString toString( KCal::Alarm * )
{
  return QString();
}

static QString toString( KCal::Incidence * )
{
  return QString();
}

static QString toString( KCal::Attachment * )
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

template <class T>
static void compareList( AbstractDifferencesReporter *reporter, const QString &id, const QList<T> &left, const QList<T> &right )
{
  for ( int i = 0; i < left.count(); ++i ) {
    if ( !right.contains( left[ i ] )  )
      reporter->addProperty( AbstractDifferencesReporter::AdditionalLeftMode, id, toString( left[ i ] ), QString() );
  }

  for ( int i = 0; i < right.count(); ++i ) {
    if ( !left.contains( right[ i ] )  )
      reporter->addProperty( AbstractDifferencesReporter::AdditionalRightMode, id, QString(), toString( right[ i ] ) );
  }
}

static void compareIncidenceBase( AbstractDifferencesReporter *reporter, const KCal::IncidenceBase *left, const KCal::IncidenceBase *right )
{
  compareList( reporter, i18n( "Attendees" ), left->attendees(), right->attendees() );

  if ( left->dtStart() != right->dtStart() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Start time" ),
                            left->dtStartStr(), right->dtStartStr() );

  if ( !compareString( left->organizer().fullName(), right->organizer().fullName() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Organizer" ),
                            left->organizer().fullName(), right->organizer().fullName() );

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

static void compareIncidence( AbstractDifferencesReporter *reporter, const KCal::Incidence *left, const KCal::Incidence *right )
{
  if ( !compareString( left->description(), right->description() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Description" ),
                            left->description(), right->description() );

  if ( !compareString( left->summary(), right->summary() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Summary" ),
                            left->summary(), right->summary() );

  if ( left->status() != right->status() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Status" ),
                            left->statusStr(), right->statusStr() );

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
  compareList( reporter, i18n( "Relations" ), left->relations(), right->relations() );
  compareList( reporter, i18n( "Attachments" ), left->attachments(), right->attachments() );
  compareList( reporter, i18n( "Exception Dates" ), left->recurrence()->exDates(), right->recurrence()->exDates() );
  compareList( reporter, i18n( "Exception Times" ), left->recurrence()->exDateTimes(), right->recurrence()->exDateTimes() );
  // TODO: recurrence dates and date/times, exrules, rrules

  if ( left->created() != right->created() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode,
                            i18n( "Created" ), left->created().toString(), right->created().toString() );

  if ( !compareString( left->relatedToUid(), right->relatedToUid() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode,
                            i18n( "Related Uid" ), left->relatedToUid(), right->relatedToUid() );
}

static void compareEvent( AbstractDifferencesReporter *reporter, const KCal::Event *left, const KCal::Event *right )
{
  if ( left->hasEndDate() != right->hasEndDate() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Has End Date" ),
                            toString( left->hasEndDate() ), toString( right->hasEndDate() ) );

  if ( left->dtEnd() != right->dtEnd() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "End Date" ),
                            left->dtEndStr(), right->dtEndStr() );

  // TODO: check transparency
}

static void compareTodo( AbstractDifferencesReporter *reporter, const KCal::Todo *left, const KCal::Todo *right )
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

void SerializerPluginKCal::compare( Akonadi::AbstractDifferencesReporter *reporter,
                                    const Akonadi::Item &leftItem,
                                    const Akonadi::Item &rightItem )
{
  Q_ASSERT( reporter );
  Q_ASSERT( leftItem.hasPayload<IncidencePtr>() );
  Q_ASSERT( rightItem.hasPayload<IncidencePtr>() );

  const IncidencePtr leftIncidencePtr = leftItem.payload<IncidencePtr>();
  const IncidencePtr rightIncidencePtr = rightItem.payload<IncidencePtr>();
  const KCal::Incidence *leftIncidence = leftIncidencePtr.get();
  const KCal::Incidence *rightIncidence = rightIncidencePtr.get();

  if ( dynamic_cast<const KCal::Event*>( leftIncidence ) ) {
    reporter->setLeftPropertyValueTitle( i18n( "Changed Event" ) );
    reporter->setRightPropertyValueTitle( i18n( "Conflicting Event" ) );
  } else if ( dynamic_cast<const KCal::Todo*>( leftIncidence ) ) {
    reporter->setLeftPropertyValueTitle( i18n( "Changed Todo" ) );
    reporter->setRightPropertyValueTitle( i18n( "Conflicting Todo" ) );
  }

  compareIncidenceBase( reporter, leftIncidence, rightIncidence );
  compareIncidence( reporter, leftIncidence, rightIncidence );

  const KCal::Event *leftEvent = dynamic_cast<const KCal::Event*>( leftIncidence );
  const KCal::Event *rightEvent = dynamic_cast<const KCal::Event*>( rightIncidence );
  if ( leftEvent && rightEvent ) {
    compareEvent( reporter, leftEvent, rightEvent );
  } else {
    const KCal::Todo *leftTodo = dynamic_cast<const KCal::Todo*>( leftIncidence );
    const KCal::Todo *rightTodo = dynamic_cast<const KCal::Todo*>( rightIncidence );
    if ( leftTodo && rightTodo ) {
      compareTodo( reporter, leftTodo, rightTodo );
    }
  }
}

Q_EXPORT_PLUGIN2( akonadi_serializer_kcal, SerializerPluginKCal )

#include "akonadi_serializer_kcal.moc"
