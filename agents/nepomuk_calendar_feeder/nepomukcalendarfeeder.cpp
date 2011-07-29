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
#include "nco.h"
#include "ncal.h"

#include "nepomukfeederutils.h"

#include <KCalCore/Event>
#include <KCalCore/Todo>
#include <KCalCore/Journal>
#include <KCalCore/FreeBusy>

#include <akonadi/changerecorder.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>

#include <nepomuk/resource.h>
#include <nepomuk/resourcemanager.h>
#include <nepomuk/variant.h>
#include <kurl.h>
#include <KLocale>

#include <nepomuk/andterm.h>
#include <nepomuk/comparisonterm.h>
#include <nepomuk/literalterm.h>
#include <nepomuk/resourcetypeterm.h>
#include <nepomuk/simpleresource.h>
#include <nepomuk/simpleresourcegraph.h>

#include <Soprano/Model>
#include <Soprano/NodeIterator>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/NAO>

// ontology includes
#include <ncal/attendee.h>
#include <ncal/event.h>
#include <ncal/todo.h>
#include <ncal/journal.h>
#include <nco/emailaddress.h>
#include <nco/personcontact.h>

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QString>
#include <QtDBus/QDBusConnection>

#include <KDebug>

#include <boost/shared_ptr.hpp>

namespace Akonadi {

NepomukCalendarFeeder::NepomukCalendarFeeder( const QString &id )
  : NepomukFeederAgentBase( id )
{
  KGlobal::locale()->insertCatalog( "akonadi_nepomukfeeder" );
  addSupportedMimeType( KCalCore::Event::eventMimeType() );
  addSupportedMimeType( KCalCore::Todo::todoMimeType() );
  addSupportedMimeType( KCalCore::Journal::journalMimeType() );
  addSupportedMimeType( KCalCore::FreeBusy::freeBusyMimeType() );

  changeRecorder()->itemFetchScope().fetchFullPayload();

  disableIdleDetection( true );
}

NepomukCalendarFeeder::~NepomukCalendarFeeder()
{
}

void NepomukCalendarFeeder::updateItem( const Akonadi::Item &item, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph )
{
    kWarning() << item.id();
  if ( item.hasPayload<KCalCore::Event::Ptr>() ) {
    updateEventItem( item, item.payload<KCalCore::Event::Ptr>(), res, graph );
  } else if ( item.hasPayload<KCalCore::Journal::Ptr>() ) {
    updateJournalItem( item, item.payload<KCalCore::Journal::Ptr>(), res, graph );
  } else if ( item.hasPayload<KCalCore::Todo::Ptr>() ) {
    updateTodoItem( item, item.payload<KCalCore::Todo::Ptr>(), res, graph );
  } else {
    kDebug() << "Got item without known payload. Mimetype:" << item.mimeType()
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
  // create event with the graph reference
  Nepomuk::NCAL::Event event( &res );
  //workaround since the wrapper class doesn't set the type if no property is set
  res.addProperty(Soprano::Vocabulary::RDF::type(), QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Event", QUrl::StrictMode));
  NepomukFeederUtils::setIcon( "view-pim-calendar", res, graph );
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
    QUrl contact = findOrCreateContact( calAttendee->email(), calAttendee->name(), graph );
    Nepomuk::SimpleResource attendeeResource;
    Nepomuk::NCAL::Attendee attendee(&attendeeResource);
    attendee.addInvolvedContact( contact );

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
  // create journal entry with the graph reference
  Nepomuk::NCAL::Journal journal( &res );
  //workaround since the wrapper class doesn't set the type if no property is set
  res.addProperty(Soprano::Vocabulary::RDF::type(), QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Journal", QUrl::StrictMode));
  NepomukFeederUtils::setIcon( "view-pim-journal", res, graph );
  updateIncidenceItem( calJournal, res, graph );
}

void NepomukCalendarFeeder::updateTodoItem( const Akonadi::Item &item, const KCalCore::Todo::Ptr &calTodo, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph )
{
  Nepomuk::NCAL::Todo todo( &res );
  //workaround since the wrapper class doesn't set the type if no property is set
  res.addProperty(Soprano::Vocabulary::RDF::type(), QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Todo", QUrl::StrictMode));
  NepomukFeederUtils::setIcon( "view-pim-task", res, graph );
  updateIncidenceItem( calTodo, res, graph );
}

//FIXME just storing it again would do the same, so no need for the query.
//We're supposed to find the contact created by the contactfeeder, so some code sharing would probably be useful
QUrl NepomukCalendarFeeder::findOrCreateContact(const QString& emailAddress, const QString& name, Nepomuk::SimpleResourceGraph &graph, bool* found)
{
  //
  // Querying with the exact address string is not perfect since email addresses
  // are case insensitive. But for the moment we stick to it and hope Nepomuk
  // alignment fixes any duplicates
  //
  Nepomuk::Query::Query query;
  Nepomuk::Query::AndTerm andTerm;
  Nepomuk::Query::ResourceTypeTerm personTypeTerm( Vocabulary::NCO::PersonContact() );
  andTerm.addSubTerm( personTypeTerm );
  if ( emailAddress.isEmpty() ) {
    const Nepomuk::Query::ComparisonTerm nameTerm( Vocabulary::NCO::fullname(), Nepomuk::Query::LiteralTerm( name ),
                                                   Nepomuk::Query::ComparisonTerm::Equal );
    andTerm.addSubTerm( nameTerm );
  } else {
    const Nepomuk::Query::ComparisonTerm addrTerm( Vocabulary::NCO::emailAddress(), Nepomuk::Query::LiteralTerm( emailAddress ),
                                                   Nepomuk::Query::ComparisonTerm::Equal );
    const Nepomuk::Query::ComparisonTerm mailTerm( Vocabulary::NCO::hasEmailAddress(), addrTerm );
    andTerm.addSubTerm( mailTerm );
  }
  query.setTerm( andTerm );
  query.setLimit( 1 );
  
  Soprano::QueryResultIterator it = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query.toSparqlQuery(), Soprano::Query::QueryLanguageSparql );

  if ( it.next() ) {
    if ( found ) *found = true;
    const QUrl uri = it.binding( 0 ).uri();
    it.close();
    return uri;
  }
  if ( found ) *found = false;
  // create a new contact
  kDebug() << "Did not find " << name << emailAddress << ", creating a new PersonContact";

  Nepomuk::SimpleResource contactRes;
  Nepomuk::NCO::PersonContact contact(&contactRes);
  contactRes.setProperty( Soprano::Vocabulary::NAO::prefLabel(), name.isEmpty() ? emailAddress : name);
  if ( !emailAddress.isEmpty() ) {
    Nepomuk::SimpleResource emailRes;
    Nepomuk::NCO::EmailAddress email( &emailRes );
    email.setEmailAddress( emailAddress );
    graph << emailRes;
    contact.addHasEmailAddress( emailRes.uri() );
  }
  if ( !name.isEmpty() )
    contact.setFullname( name );

  graph << contactRes;
  return contactRes.uri();
}


} // namespace Akonadi

AKONADI_AGENT_MAIN( Akonadi::NepomukCalendarFeeder )

#include "nepomukcalendarfeeder.moc"
