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

#include <akonadi/changerecorder.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>
#include <kcal/kcalmimetypevisitor.h>

#include <nepomuk/resource.h>
#include <nepomuk/resourcemanager.h>
#include <nepomuk/variant.h>
#include <kurl.h>

#include <Soprano/Model>
#include <Soprano/NodeIterator>
#include <Soprano/QueryResultIterator>
#include <soprano/nrl.h>

#define USING_SOPRANO_NRLMODEL_UNSTABLE_API 1
#include <soprano/nrlmodel.h>

// ontology includes
#include "attendee.h"
#include "emailaddress.h"
#include "event.h"
#include "eventstatus.h"
#include "journal.h"
#include "participationstatus.h"
#include "personcontact.h"
#include "todo.h"

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>

#include <KDebug>

#include <boost/shared_ptr.hpp>

namespace Akonadi {

static NepomukFast::Contact findNepomukContact( const QString &name, const QString &email )
{
  // find person using name and email
  QList<Soprano::Node> list = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(
                QString( "prefix nco:<http://www.semanticdesktop.org/ontologies/2007/03/22/nco#>"
                          "select ?person where {"
                          "  ?person nco:fullname \"%1\"^^<http://www.w3.org/2001/XMLSchema#string> ."
                          "  ?person nco:hasEmailAddress ?email ."
                          "  ?email nco:emailAddress \"%2\"^^<http://www.w3.org/2001/XMLSchema#string> ."
                          "}" )
                        .arg( name, email ),
                Soprano::Query::QueryLanguageSparql ).iterateBindings( 0 ).allNodes();
  foreach ( const Soprano::Node &node, list )
    if ( node.isResource () )
      return NepomukFast::Contact( node.uri() );

  return NepomukFast::Contact();
}

NepomukCalendarFeeder::NepomukCalendarFeeder( const QString &id )
  : NepomukFeederAgent( id )
{
  addSupportedMimeType( KCalMimeTypeVisitor::eventMimeType() );
  addSupportedMimeType( KCalMimeTypeVisitor::todoMimeType() );
  addSupportedMimeType( KCalMimeTypeVisitor::journalMimeType() );
  addSupportedMimeType( KCalMimeTypeVisitor::freeBusyMimeType() );

  changeRecorder()->itemFetchScope().fetchFullPayload();

  mNrlModel = new Soprano::NRLModel( Nepomuk::ResourceManager::instance()->mainModel() );
}

NepomukCalendarFeeder::~NepomukCalendarFeeder()
{
  delete mNrlModel;
}

void NepomukCalendarFeeder::updateItem( const Akonadi::Item &item )
{
  if ( !item.hasPayload<KCal::Incidence::Ptr>() ) {
    kDebug() << "Got item without payload. Mimetype:" << item.mimeType()
             << "Id:" << item.id();
    return;
  }

  // first remove the item: since we use a graph that has a reference to all parts
  // of the item's semantic representation this is a really fast operation
  removeItemFromNepomuk( item );

  // create a new graph for the item
  QUrl metaDataGraphUri;
  QUrl graphUri = mNrlModel->createGraph( Soprano::Vocabulary::NRL::InstanceBase(), &metaDataGraphUri );

  // remember to which graph the item belongs to (used in search query in removeItemFromNepomuk())
  mNrlModel->addStatement( graphUri,
                           QUrl::fromEncoded( "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#dataGraphFor", QUrl::StrictMode ),
                           item.url(), metaDataGraphUri );

  if ( item.hasPayload<KCal::Event::Ptr>() ) {
    updateEventItem( item, item.payload<KCal::Event::Ptr>(), graphUri );
  } else if ( item.hasPayload<KCal::Journal::Ptr>() ) {
    updateJournalItem( item, item.payload<KCal::Journal::Ptr>(), graphUri );
  } else if ( item.hasPayload<KCal::Todo::Ptr>() ) {
    updateTodoItem( item, item.payload<KCal::Todo::Ptr>(), graphUri );
  }
}

void NepomukCalendarFeeder::updateEventItem( const Akonadi::Item &item, const KCal::Event::Ptr &calEvent, const QUrl &graphUri )
{
  // create event with the graph reference
  NepomukFast::Event event( item.url(), graphUri );

  event.setLabel( calEvent->summary() );

  QString uri;
  switch ( calEvent->status() ) {
    case KCal::Incidence::StatusCanceled:
      uri = "http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#cancelledEventStatus";
      break;
    case KCal::Incidence::StatusConfirmed:
      uri = "http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#confirmedEventStatus";
      break;
    case KCal::Incidence::StatusTentative:
      uri = "http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tentativeEventStatus";
      break;
    default: // other states are not available in the ontology
      break;
  }

  if ( !uri.isEmpty() ) {
    NepomukFast::EventStatus status( QUrl( uri ), graphUri );
    event.addEventStatus( status );
  }

  foreach ( const KCal::Attendee *calAttendee, calEvent->attendees() ) {
    NepomukFast::Contact contact = findNepomukContact( calAttendee->name(), calAttendee->email() );
    if ( !contact.uri().isValid() ) {
      contact.setLabel( calAttendee->name() );
      NepomukFast::EmailAddress email( QUrl( "mailto:" + calAttendee->email() ) );
      email.setEmailAddress( calAttendee->email() );
      contact.addEmailAddress( email );
    }

    NepomukFast::Attendee attendee( QUrl(), graphUri );
    attendee.addInvolvedContact( contact );

    uri.clear();
    switch( calAttendee->status() ) {
      case KCal::Attendee::NeedsAction:
        uri = "http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#needsActionParticipationStatus";
        break;
      case KCal::Attendee::Accepted:
        uri = "http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#acceptedParticipationStatus";
        break;
      case KCal::Attendee::Declined:
        uri = "http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#declinedParticipationStatus";
        break;
      case KCal::Attendee::Tentative:
        uri = "http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tentativeParticipationStatus";
        break;
      case KCal::Attendee::Delegated:
        uri = "http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#delegatedParticipationStatus";
        break;
      default: // other states are not available in the ontology
        break;
    }

    if ( !uri.isEmpty() ) {
      NepomukFast::ParticipationStatus partStatus( QUrl( uri ), graphUri );
      attendee.addPartstat( partStatus );
    }

    event.addAttendee( attendee );
  }

  tagsFromCategories( event, calEvent->categories() );
}

void NepomukCalendarFeeder::updateJournalItem( const Akonadi::Item &item, const KCal::Journal::Ptr &calJournal, const QUrl &graphUri )
{
    // create journal entry with the graph reference
    NepomukFast::Journal journal( item.url(), graphUri );

    journal.setLabel( calJournal->summary() );

    tagsFromCategories( journal, calJournal->categories() );
}

void NepomukCalendarFeeder::updateTodoItem( const Akonadi::Item &item, const KCal::Todo::Ptr &calTodo, const QUrl &graphUri )
{
  NepomukFast::Todo todo( item.url(), graphUri );
  todo.setLabel( calTodo->summary() );
  tagsFromCategories( todo, calTodo->categories() );
}

} // namespace Akonadi

AKONADI_AGENT_MAIN( Akonadi::NepomukCalendarFeeder )

#include "nepomukcalendarfeeder.moc"
