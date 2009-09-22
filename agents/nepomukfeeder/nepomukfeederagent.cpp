/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
                  2008 Sebastian Trueg <trueg@kde.org>
                  2009 Volker Krause <vkrause@kde.org>

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

#include "nepomukfeederagent.h"
#include "selectsqarqlbuilder.h"
#include "nie.h"

#include <akonadi/item.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/mimetypechecker.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

#include <nepomuk/resource.h>
#include <nepomuk/resourcemanager.h>
#include <nepomuk/tag.h>

#include <KLocale>
#include <KUrl>

#include <Soprano/Model>
#include <Soprano/NodeIterator>
#include <Soprano/QueryResultIterator>

#include <QtCore/QTimer>

#include <boost/bind.hpp>

using namespace Akonadi;

NepomukFeederAgent::NepomukFeederAgent(const QString& id) :
  AgentBase(id),
  mTotalAmount( 0 ),
  mProcessedAmount( 0 ),
  mPendingJobs( 0 )
{
  // initialize Nepomuk
  Nepomuk::ResourceManager::instance()->init();

  changeRecorder()->setChangeRecordingEnabled( false );

  QTimer::singleShot( 0, this, SLOT(updateAll()) );
}

NepomukFeederAgent::~NepomukFeederAgent()
{
}

void NepomukFeederAgent::removeItemFromNepomuk( const Akonadi::Item &item )
{
  // find the graph that contains our item and delete the complete graph
  SparqlBuilder::BasicGraphPattern graph;
  // FIXME: why isn't that in the ontology?
//   graph.addTriple( "?g", Vocabulary::Nie::dataGraphFor(), item.url() );
  graph.addTriple( "?g", QUrl( "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#dataGraphFor" ), item.url() );
  SelectSparqlBuilder qb;
  qb.addQueryVariable( "?g" );
  qb.setGraphPattern( graph );
  const QList<Soprano::Node> list = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( qb.query(),
      Soprano::Query::QueryLanguageSparql ).iterateBindings( 0 ).allNodes();

  foreach ( const Soprano::Node &node, list )
    Nepomuk::ResourceManager::instance()->mainModel()->removeContext( node );
}


void NepomukFeederAgent::itemAdded(const Akonadi::Item& item, const Akonadi::Collection& collection)
{
  Q_UNUSED( collection );
  if ( item.hasPayload() )
    updateItem( item );
}

void NepomukFeederAgent::itemChanged(const Akonadi::Item& item, const QSet< QByteArray >& partIdentifiers)
{
  // TODO: check part identfiers if anything interesting changed at all
  if ( item.hasPayload() )
    updateItem( item );
}

void NepomukFeederAgent::itemRemoved(const Akonadi::Item& item)
{
  removeItemFromNepomuk( item );
}

void NepomukFeederAgent::addSupportedMimeType( const QString &mimeType )
{
  mSupportedMimeTypes.append( mimeType );
  changeRecorder()->setMimeTypeMonitored( mimeType );
  mMimeTypeChecker.setWantedMimeTypes( mSupportedMimeTypes );
}

void NepomukFeederAgent::updateAll()
{
  CollectionFetchJob *collectionFetch = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, this );
  collectionFetch->fetchScope().setContentMimeTypes( mSupportedMimeTypes );
  connect( collectionFetch, SIGNAL(collectionsReceived(Akonadi::Collection::List)), SLOT(collectionsReceived(Akonadi::Collection::List)) );
}

void NepomukFeederAgent::collectionsReceived(const Akonadi::Collection::List& collections)
{
  mMimeTypeChecker.setWantedMimeTypes( mSupportedMimeTypes );
  foreach( const Collection &collection, collections ) {
    if ( mMimeTypeChecker.isWantedCollection( collection ) )
      mCollectionQueue.append( collection );
  }
  processNextCollection();
}

void NepomukFeederAgent::processNextCollection()
{
  if ( mCurrentCollection.isValid() || mCollectionQueue.isEmpty() )
    return;
  mCurrentCollection = mCollectionQueue.takeFirst();
  emit status( AgentBase::Running, i18n( "Indexing collection '%1'...", mCurrentCollection.name() ) );
  kDebug() << "Indexing collection" << mCurrentCollection.name();
  ItemFetchJob *itemFetch = new ItemFetchJob( mCurrentCollection, this );
  itemFetch->fetchScope().setCacheOnly( true );
  connect( itemFetch, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(itemHeadersReceived(Akonadi::Item::List)) );
  connect( itemFetch, SIGNAL(result(KJob*)), SLOT(itemFetchResult(KJob*)) );
  ++mPendingJobs;
  mTotalAmount = 0;
}

void NepomukFeederAgent::itemHeadersReceived(const Akonadi::Item::List& items)
{
  kDebug() << items.count();
  Akonadi::Item::List itemsToUpdate;
  foreach( const Item &item, items ) {
    if ( !mMimeTypeChecker.isWantedItem( item ) )
      continue;
    // update item if it does not exist
    if ( !Nepomuk::ResourceManager::instance()->mainModel()->containsAnyStatement( item.url(), Soprano::Node(), Soprano::Node() ) )
      itemsToUpdate.append( item );
  }

  if ( !itemsToUpdate.isEmpty() ) {
    ItemFetchJob *itemFetch = new ItemFetchJob( itemsToUpdate, this );
    itemFetch->setFetchScope( changeRecorder()->itemFetchScope() );
    connect( itemFetch, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(itemsReceived(Akonadi::Item::List)) );
    connect( itemFetch, SIGNAL(result(KJob*)), SLOT(itemFetchResult(KJob*)) );
    ++mPendingJobs;
    mTotalAmount += itemsToUpdate.size();
  }
}

void NepomukFeederAgent::itemFetchResult(KJob* job)
{
  if ( job->error() )
    kDebug() << job->errorString();

  --mPendingJobs;
  if ( mPendingJobs == 0 ) {
    mCurrentCollection = Collection();
    emit status( Idle, i18n( "Indexing completed." ) );
    processNextCollection();
    return;
  }
}

void NepomukFeederAgent::itemsReceived(const Akonadi::Item::List& items)
{
  kDebug() << items.size();
  std::for_each( items.constBegin(), items.constEnd(), boost::bind( &NepomukFeederAgent::updateItem, this, _1 ) );
  mProcessedAmount += items.count();
  emit percent( (mProcessedAmount * 100) / (mTotalAmount * 100) );
}

void NepomukFeederAgent::tagsFromCategories(NepomukFast::Resource& resource, const QStringList& categories)
{
  // FIXME: this voids the usage of the XFast classes...
  Nepomuk::Resource res( resource.uri() );
  foreach ( const QString &category, categories ) {
    const Nepomuk::Tag tag( category );
    res.addTag( tag );
  }
}

#include "nepomukfeederagent.moc"
