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

#include <kcal/event.h>
#include <kcal/journal.h>
#include <kcal/todo.h>

#include <akonadi/changerecorder.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/mimetypechecker.h>

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

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>

#include <KDebug>

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

namespace Akonadi {

static void removeItemFromNepomuk( const Akonadi::Item &item )
{
  // find the graph that contains our item and delete the complete graph
  QList<Soprano::Node> list = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(
                QString( "select ?g where { ?g <http://www.semanticdesktop.org/ontologies/2007/01/19/nie#dataGraphFor> %1. }")
                       .arg( Soprano::Node::resourceToN3( item.url() ) ),
                Soprano::Query::QueryLanguageSparql ).iterateBindings( 0 ).allNodes();

  foreach ( const Soprano::Node &node, list )
    Nepomuk::ResourceManager::instance()->mainModel()->removeContext( node );
}

NepomukCalendarFeeder::NepomukCalendarFeeder( const QString &id )
  : AgentBase( id ),
    mForceUpdate( false )
{
  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->setMimeTypeMonitored( "text/calendar" );
  changeRecorder()->setChangeRecordingEnabled( false );

  // do the initial scan to make sure all items have been fed to nepomuk
  QTimer::singleShot( 0, this, SLOT( slotInitialItemScan() ) );

  // The line below is not necessary anymore once AgentBase also exports scriptable slots
  QDBusConnection::sessionBus().registerObject( "/nepomukcalendarfeeder", this, QDBusConnection::ExportScriptableSlots );

  // initialize Nepomuk
  Nepomuk::ResourceManager::instance()->init();

  mNrlModel = new Soprano::NRLModel( Nepomuk::ResourceManager::instance()->mainModel() );
}

NepomukCalendarFeeder::~NepomukCalendarFeeder()
{
  delete mNrlModel;
}

void NepomukCalendarFeeder::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
{
  itemChanged( item, QSet<QByteArray>() );
}

void NepomukCalendarFeeder::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  if ( item.hasPayload<IncidencePtr>() )
    updateItem( item );
}

void NepomukCalendarFeeder::itemRemoved( const Akonadi::Item &item )
{
  removeItemFromNepomuk( item );
}

void NepomukCalendarFeeder::slotInitialItemScan()
{
  kDebug();
  updateAll( false );
}

void NepomukCalendarFeeder::updateAll( bool force )
{
  mForceUpdate = force;

  CollectionFetchJob *collectionFetch = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  if ( collectionFetch->exec() ) {
    Collection::List collections = collectionFetch->collections();

    MimeTypeChecker calendarFilter;
    calendarFilter.addWantedMimeType( "text/calendar" );
    foreach ( const Collection &collection, collections ) {
      if ( calendarFilter.isWantedCollection( collection ) ) {
        ItemFetchJob *itemFetch = new ItemFetchJob( collection );
        itemFetch->fetchScope().fetchFullPayload();
        connect( itemFetch, SIGNAL( itemsReceived( Akonadi::Item::List ) ),
                 this, SLOT( slotItemsReceivedForInitialScan( Akonadi::Item::List ) ) );
      }
    }
  }
}

void NepomukCalendarFeeder::slotItemsReceivedForInitialScan( const Akonadi::Item::List& items )
{
  kDebug() << items.count();
  foreach( const Item &item, items ) {
    // only update the item if it does not exist
    if ( mForceUpdate ||
         !Nepomuk::ResourceManager::instance()->mainModel()->containsAnyStatement( item.url(), Soprano::Node(), Soprano::Node() ) ) {
      updateItem( item );
    }
  }
}

void NepomukCalendarFeeder::updateItem( const Akonadi::Item &item )
{
  if ( !item.hasPayload<IncidencePtr>() ) {
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

  IncidencePtr incidencePtr = item.payload<IncidencePtr>();

  KCal::Incidence *incidence = incidencePtr.get();

  if ( incidence->type() == "Event" )
    updateEventItem( item, graphUri );
  else if ( incidence->type() == "Journal" )
    updateJournalItem( item, graphUri );
  else if ( incidence->type() == "Todo" )
    updateTodoItem( item, graphUri );
}

void NepomukCalendarFeeder::updateEventItem( const Akonadi::Item &item, const QUrl &graphUri )
{
}

void NepomukCalendarFeeder::updateJournalItem( const Akonadi::Item &item, const QUrl &graphUri )
{
}

void NepomukCalendarFeeder::updateTodoItem( const Akonadi::Item &item, const QUrl &graphUri )
{
}

} // namespace Akonadi

AKONADI_AGENT_MAIN( Akonadi::NepomukCalendarFeeder )

#include "nepomukcalendarfeeder.moc"
