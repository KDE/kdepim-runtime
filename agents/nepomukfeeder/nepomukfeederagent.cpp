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
#include <KProcess>
#include <KMessageBox>

#include <Soprano/Model>
#include <Soprano/NodeIterator>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/NAO>

#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>

#include <boost/bind.hpp>
#include <akonadi/entitydisplayattribute.h>

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

  selfTest();
  if ( isOnline() )
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
    if ( !mMimeTypeChecker.isWantedCollection( collection ) )
      continue;
    EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>();
    if ( attr && attr->isHidden() )
      continue;
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
    if ( item.storageCollectionId() != mCurrentCollection.id() )
      continue; // stay away from links
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
  foreach ( const QString &category, categories ) {
    const Nepomuk::Tag tag( category );
    resource.addProperty( Soprano::Vocabulary::NAO::hasTag(), tag.resourceUri() );
  }
}


void NepomukFeederAgent::selfTest()
{
  QStringList errorMessages;

  // if Nepomuk is not running, try to start it
  if ( !QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.NepomukStorage" ) ) {
    KProcess process;
    if ( process.startDetached( QLatin1String( "nepomukserver" ) ) == 0 )
      errorMessages.append( i18n( "Unable to start the Nepomuk server." ) );
  }

  // if it is already running, check if the backend is correct
  if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.NepomukStorage" ) ) {
    static const QStringList backendBlacklist = QStringList() << QLatin1String( "redland" );
    // check which backend is used
    QDBusInterface interface( "org.kde.NepomukStorage", "/nepomukstorage" );
    QDBusReply<QString> reply = interface.call( "usedSopranoBackend" );
    if ( reply.isValid() ) {
      const QString backend = reply.value().toLower();
      if ( backendBlacklist.contains( backend ) )
        errorMessages.append( i18n( "A blacklisted backend is used: '%1'.", backend ) );
    } else {
      errorMessages.append( i18n( "Calling the Nepomuk storage service failed: '%1'.", reply.error().message() ) );
    }
  } else {
    errorMessages.append( i18n( "Nepomuk is not running." ) );
  }

  if ( errorMessages.isEmpty() ) {
    setOnline( true );
    return;
  }

  setOnline( false );

  QString message = i18n( "<b>Nepomuk indexing agents have been disabled</b><br/>"
                          "The Nepomuk service is not available or fully operational and attempts to rectify this have failed. "
                          "Therefore indexing of all data stored in the Akonadi PIM service has been disabled, which will "
                          "severely limit the capabilities of any application using this data.<br/><br/>"
                          "The following problems were detected:<ul><li>%1</li></ul>"
                          "Additional help can be found here: <a href=\"http://userbase.kde.org/Akonadi\">userbase.kde.org/Akonadi</a>",
                          errorMessages.join( "</li><li>" ) );

  // prevent a message storm from all agents
  emit status( Broken, i18n( "Nepomuk not operational" ) );
  if ( !QDBusConnection::sessionBus().registerService( "org.kde.pim.nepomukfeeder.selftestreport" ) )
    return;
  KMessageBox::error( 0, message, i18n( "Nepomuk indexing disabled" ), KMessageBox::Notify | KMessageBox::AllowLink );
  QDBusConnection::sessionBus().unregisterService( "org.kde.pim.nepomukfeeder.selftestreport" );
}


#include "nepomukfeederagent.moc"
