/*
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

#include "nepomukcalendarfeeder.h"
#include "selectsqarqlbuilder.h"
#include "nco.h"

#include <akonadi/changerecorder.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/kcal/incidencemimetypevisitor.h>

#include <nepomuk/resource.h>
#include <nepomuk/resourcemanager.h>
#include <nepomuk/variant.h>
#include <kurl.h>

#include <Soprano/Model>
#include <Soprano/NodeIterator>
#include <Soprano/QueryResultIterator>

// ontology includes
#include "attendee.h"
#include "emailaddress.h"
#include "event.h"
#include "eventstatus.h"
#include "journal.h"
#include "participationstatus.h"
#include "personcontact.h"
#include "todo.h"
#include "unionofalarmeventfreebusyjournaltodo.h"

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>

#include <KDebug>

#include <boost/shared_ptr.hpp>

namespace AkonadiFuture {

NepomukCalendarFeeder::NepomukCalendarFeeder( const QString &id )
  : NepomukFeederAgent<NepomukFast::Calendar>( id )
{
  addSupportedMimeType( Akonadi::IncidenceMimeTypeVisitor::eventMimeType() );
  addSupportedMimeType( Akonadi::IncidenceMimeTypeVisitor::todoMimeType() );
  addSupportedMimeType( Akonadi::IncidenceMimeTypeVisitor::journalMimeType() );
  addSupportedMimeType( Akonadi::IncidenceMimeTypeVisitor::freeBusyMimeType() );

  changeRecorder()->itemFetchScope().fetchFullPayload();
}

NepomukCalendarFeeder::~NepomukCalendarFeeder()
{
}

void NepomukCalendarFeeder::updateItem( const Akonadi::Item &item, const QUrl &graphUri )
{
  if ( item.hasPayload<KCal::Event::Ptr>() ) {
    updateEventItem( item, item.payload<KCal::Event::Ptr>(), graphUri );
  } else if ( item.hasPayload<KCal::Journal::Ptr>() ) {
    updateJournalItem( item, item.payload<KCal::Journal::Ptr>(), graphUri );
  } else if ( item.hasPayload<KCal::Todo::Ptr>() ) {
    updateTodoItem( item, item.payload<KCal::Todo::Ptr>(), graphUri );
  } else {
    kDebug() << "Got item without known payload. Mimetype:" << item.mimeType()
             << "Id:" << item.id();
  }
}

void NepomukCalendarFeeder::updateEventItem( const Akonadi::Item &item, const KCal::Event::Ptr &calEvent, const QUrl &graphUri )
{
  // create event with the graph reference
  NepomukFast::Event event( item.url(), graphUri );
  event.addProperty( Soprano::Vocabulary::NAO::hasSymbol(), Soprano::LiteralValue( "view-pim-calendar" ) );
  setParent( event, item );
  updateIncidenceItem( calEvent, event, graphUri );

  QUrl uri;
  switch ( calEvent->status() ) {
    case KCal::Incidence::StatusCanceled:
      uri = Vocabulary::NCAL::cancelledEventStatus();
      break;
    case KCal::Incidence::StatusConfirmed:
      uri = Vocabulary::NCAL::confirmedStatus();
      break;
    case KCal::Incidence::StatusTentative:
      uri = Vocabulary::NCAL::tentativeStatus();
      break;
    default: // other states are not available in the ontology
      break;
  }

  if ( !uri.isEmpty() ) {
    NepomukFast::EventStatus status( uri, graphUri );
    event.addEventStatus( status );
  }

  foreach ( const KCal::Attendee *calAttendee, calEvent->attendees() ) {
    NepomukFast::Contact contact = findOrCreateContact( calAttendee->email(), calAttendee->name(), graphUri );
    NepomukFast::Attendee attendee( QUrl(), graphUri );
    attendee.addInvolvedContact( contact );

    uri.clear();
    switch( calAttendee->status() ) {
      case KCal::Attendee::NeedsAction:
        uri = Vocabulary::NCAL::needsActionParticipationStatus();
        break;
      case KCal::Attendee::Accepted:
        uri = Vocabulary::NCAL::acceptedParticipationStatus();
        break;
      case KCal::Attendee::Declined:
        uri = Vocabulary::NCAL::declinedParticipationStatus();
        break;
      case KCal::Attendee::Tentative:
        uri = Vocabulary::NCAL::tentativeParticipationStatus();
        break;
      case KCal::Attendee::Delegated:
        uri = Vocabulary::NCAL::delegatedParticipationStatus();
        break;
      default: // other states are not available in the ontology
        break;
    }

    if ( !uri.isEmpty() ) {
      NepomukFast::ParticipationStatus partStatus( uri, graphUri );
      attendee.addPartstat( partStatus );
    }

    event.toUnionOfAlarmEventFreebusyJournalTodo().addAttendee( attendee );
  }
}

void NepomukCalendarFeeder::updateJournalItem( const Akonadi::Item &item, const KCal::Journal::Ptr &calJournal, const QUrl &graphUri )
{
    // create journal entry with the graph reference
    NepomukFast::Journal journal( item.url(), graphUri );
    journal.addProperty( Soprano::Vocabulary::NAO::hasSymbol(), Soprano::LiteralValue( "view-pim-journal" ) );
    setParent( journal, item );
    updateIncidenceItem( calJournal, journal, graphUri );
}

void NepomukCalendarFeeder::updateTodoItem( const Akonadi::Item &item, const KCal::Todo::Ptr &calTodo, const QUrl &graphUri )
{
  NepomukFast::Todo todo( item.url(), graphUri );
  setParent( todo, item );
  todo.addProperty( Soprano::Vocabulary::NAO::hasSymbol(), Soprano::LiteralValue( "view-pim-task" ) );
  updateIncidenceItem( calTodo, todo, graphUri );
}

} // namespace Akonadi

AKONADI_AGENT_MAIN( AkonadiFuture::NepomukCalendarFeeder )

#include "nepomukcalendarfeeder.moc"
