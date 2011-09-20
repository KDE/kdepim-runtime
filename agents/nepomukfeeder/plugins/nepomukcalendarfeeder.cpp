/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>
                  2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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

#include <KMime/Message>

// ontology includes
#include <nepomuk/nfo.h>
#include <Soprano/Vocabulary/NAO>
#include <Nepomuk/Vocabulary/NIE>

#include <KDebug>

#include <akonadi/notes/noteutils.h>
#include <nepomukfeederutils.h>

#include <kexportplugin.h>
#include <kpluginfactory.h>
#include <nepomuk/ncal.h>
#include <Soprano/Vocabulary/RDF>
#include <ncal/event.h>
#include <ncal/journal.h>
#include <ncal/todo.h>
#include <ncal/attendee.h>

using namespace Nepomuk;


namespace Akonadi {

void NepomukCalendarFeeder::updateItem(const Akonadi::Item& item, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph)
{
  //kDebug() << item.id();
  if ( item.hasPayload<KCalCore::Event::Ptr>() ) {
    updateEventItem( item, item.payload<KCalCore::Event::Ptr>(), res, graph );
  } else if ( item.hasPayload<KCalCore::Journal::Ptr>() ) {
    updateJournalItem( item, item.payload<KCalCore::Journal::Ptr>(), res, graph );
  } else if ( item.hasPayload<KCalCore::Todo::Ptr>() ) {
    updateTodoItem( item, item.payload<KCalCore::Todo::Ptr>(), res, graph );
  } else {
    kWarning() << "Got item without known payload. Mimetype:" << item.mimeType()
             << "Id:" << item.id();
  }
}

void NepomukCalendarFeeder::updateIncidenceItem( const KCalCore::Incidence::Ptr &calInc, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph )
{
  res.setProperty( Soprano::Vocabulary::NAO::prefLabel(), calInc->summary() );
  res.setProperty( Vocabulary::NCAL::summary(), calInc->summary() );
  res.setProperty( Vocabulary::NIE::title(), calInc->summary() );
  if ( !calInc->location().isEmpty() )
    res.setProperty( Vocabulary::NCAL::location(), calInc->location() );
  if ( !calInc->description().isEmpty() ) {
    res.setProperty( Vocabulary::NCAL::description(), calInc->description() );
    res.setProperty( Vocabulary::NIE::plainTextContent(), calInc->description() );
  }

  res.setProperty( Vocabulary::NCAL::uid(), calInc->uid() );

  NepomukFeederUtils::tagsFromCategories( calInc->categories(), res, graph );
}

void NepomukCalendarFeeder::updateEventItem( const Akonadi::Item &item, const KCalCore::Event::Ptr &calEvent, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph )
{
  Q_UNUSED(item);
  // create event with the graph reference
  Nepomuk::NCAL::Event event( &res );
  //the wrapper class doesn't set the type if no memberfunction is called
  res.addType( Vocabulary::NCAL::Event());
  //NepomukFeederUtils::setIcon( "view-pim-calendar", res, graph ); //Disable Icon until we know how to properly set them
  updateIncidenceItem( calEvent, res, graph );

  QUrl uri;
  switch ( calEvent->status() ) {
    case KCalCore::Incidence::StatusCanceled:
      uri = Vocabulary::NCAL::cancelledEventStatus();
      break;
    case KCalCore::Incidence::StatusConfirmed:
      uri = Vocabulary::NCAL::confirmedStatus();
      break;
    case KCalCore::Incidence::StatusTentative:
      uri = Vocabulary::NCAL::tentativeStatus();
      break;
    default: // other states are not available in the ontology
      break;
  }

  if ( !uri.isEmpty() ) {
    event.setEventStatus( uri );
  }

  foreach ( const KCalCore::Attendee::Ptr &calAttendee, calEvent->attendees() ) {
    QUrl contactUri = NepomukFeederUtils::addContact( calAttendee->email(), calAttendee->name(), graph ).uri();
    Nepomuk::SimpleResource attendeeResource;
    Nepomuk::NCAL::Attendee attendee(&attendeeResource);
    attendee.addInvolvedContact( contactUri );

    uri.clear();
    switch( calAttendee->status() ) {
      case KCalCore::Attendee::NeedsAction:
        uri = Vocabulary::NCAL::needsActionParticipationStatus();
        break;
      case KCalCore::Attendee::Accepted:
        uri = Vocabulary::NCAL::acceptedParticipationStatus();
        break;
      case KCalCore::Attendee::Declined:
        uri = Vocabulary::NCAL::declinedParticipationStatus();
        break;
      case KCalCore::Attendee::Tentative:
        uri = Vocabulary::NCAL::tentativeParticipationStatus();
        break;
      case KCalCore::Attendee::Delegated:
        uri = Vocabulary::NCAL::delegatedParticipationStatus();
        break;
      default: // other states are not available in the ontology
        break;
    }

    if ( !uri.isEmpty() ) {
      attendee.addPartstat( uri );
    }
    graph << attendeeResource;

    event.addAttendee( attendeeResource.uri() ); //FIXME is this correct?
  }
}

void NepomukCalendarFeeder::updateJournalItem( const Akonadi::Item &item, const KCalCore::Journal::Ptr &calJournal, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph )
{
  Q_UNUSED(item);
  // create journal entry with the graph reference
  Nepomuk::NCAL::Journal journal( &res );
  //the wrapper class doesn't set the type if no memberfunction is called
  res.addType( Vocabulary::NCAL::Journal());
  //NepomukFeederUtils::setIcon( "view-pim-journal", res, graph );
  updateIncidenceItem( calJournal, res, graph );
}

void NepomukCalendarFeeder::updateTodoItem( const Akonadi::Item &item, const KCalCore::Todo::Ptr &calTodo, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph )
{
  Q_UNUSED(item);
  Nepomuk::NCAL::Todo todo( &res );
  //the wrapper class doesn't set the type if no memberfunction is called
  res.addType( Vocabulary::NCAL::Todo());
  //NepomukFeederUtils::setIcon( "view-pim-task", res, graph );
  updateIncidenceItem( calTodo, res, graph );
}

K_PLUGIN_FACTORY(factory, registerPlugin<NepomukCalendarFeeder>();)      
K_EXPORT_PLUGIN(factory("akonadi_nepomuk_calendar_feeder"))

}
