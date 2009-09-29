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

#include "nepomukfeederagentbase.h"
#include <nie.h>
#include <nco.h>
#include <personcontact.h>
#include <emailaddress.h>

#include <akonadi/item.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/mimetypechecker.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/entitydisplayattribute.h>

#include <nepomuk/resource.h>
#include <nepomuk/tag.h>

#include <KLocale>
#include <KUrl>
#include <KProcess>
#include <KMessageBox>

#include <Soprano/Vocabulary/NAO>

#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>

using namespace Akonadi;

NepomukFeederAgentBase::NepomukFeederAgentBase(const QString& id) :
  AgentBase(id),
  mTotalAmount( 0 ),
  mProcessedAmount( 0 ),
  mPendingJobs( 0 ),
  mNrlModel( 0 ),
  mNepomukStartupAttempted( false ),
  mInitialUpdateDone( false )
{
  // initialize Nepomuk
  Nepomuk::ResourceManager::instance()->init();
  mNrlModel = new Soprano::NRLModel( Nepomuk::ResourceManager::instance()->mainModel() );

  changeRecorder()->setChangeRecordingEnabled( false );
  changeRecorder()->fetchCollection( true );

  mNepomukStartupTimeout.setInterval( 60 * 1000 );
  mNepomukStartupTimeout.setSingleShot( true );
  connect( &mNepomukStartupTimeout, SIGNAL(timeout()), SLOT(selfTest()) );
  connect( QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)), SLOT(serviceOwnerChanged(QString,QString,QString)) );

  setOnline( false );
  selfTest();
}

NepomukFeederAgentBase::~NepomukFeederAgentBase()
{
  delete mNrlModel;
}

void NepomukFeederAgentBase::itemAdded(const Akonadi::Item& item, const Akonadi::Collection& collection)
{
  Q_UNUSED( collection );
  if ( item.hasPayload() )
    updateItem( item, createGraphForEntity( item ) );
}

void NepomukFeederAgentBase::itemChanged(const Akonadi::Item& item, const QSet< QByteArray >& partIdentifiers)
{
  // TODO: check part identfiers if anything interesting changed at all
  if ( item.hasPayload() ) {
    removeEntityFromNepomuk( item );
    updateItem( item, createGraphForEntity( item ) );
  }
}

void NepomukFeederAgentBase::itemRemoved(const Akonadi::Item& item)
{
  removeEntityFromNepomuk( item );
}

void NepomukFeederAgentBase::collectionAdded(const Akonadi::Collection& collection, const Akonadi::Collection& parent)
{
  Q_UNUSED( parent );
  updateCollection( collection, createGraphForEntity( collection ) );
}

void NepomukFeederAgentBase::collectionChanged(const Akonadi::Collection& collection, const QSet< QByteArray >& partIdentifiers)
{
  Q_UNUSED( partIdentifiers );
  removeEntityFromNepomuk( collection );
  updateCollection( collection, createGraphForEntity( collection ) );
}

void NepomukFeederAgentBase::collectionRemoved(const Akonadi::Collection& collection)
{
  removeEntityFromNepomuk( collection );
}

void NepomukFeederAgentBase::addSupportedMimeType( const QString &mimeType )
{
  mSupportedMimeTypes.append( mimeType );
  changeRecorder()->setMimeTypeMonitored( mimeType );
  mMimeTypeChecker.setWantedMimeTypes( mSupportedMimeTypes );
}

void NepomukFeederAgentBase::updateAll()
{
  CollectionFetchJob *collectionFetch = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, this );
  collectionFetch->fetchScope().setContentMimeTypes( mSupportedMimeTypes );
  connect( collectionFetch, SIGNAL(collectionsReceived(Akonadi::Collection::List)), SLOT(collectionsReceived(Akonadi::Collection::List)) );
}

void NepomukFeederAgentBase::collectionsReceived(const Akonadi::Collection::List& collections)
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

void NepomukFeederAgentBase::processNextCollection()
{
  if ( mCurrentCollection.isValid() || mCollectionQueue.isEmpty() )
    return;
  mCurrentCollection = mCollectionQueue.takeFirst();
  emit status( AgentBase::Running, i18n( "Indexing collection '%1'...", mCurrentCollection.name() ) );
  kDebug() << "Indexing collection" << mCurrentCollection.name();
  if ( !Nepomuk::ResourceManager::instance()->mainModel()->containsAnyStatement( mCurrentCollection.url(), Soprano::Node(), Soprano::Node() ) )
    updateCollection( mCurrentCollection, createGraphForEntity( mCurrentCollection ) );
  ItemFetchJob *itemFetch = new ItemFetchJob( mCurrentCollection, this );
  itemFetch->fetchScope().setCacheOnly( true );
  connect( itemFetch, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(itemHeadersReceived(Akonadi::Item::List)) );
  connect( itemFetch, SIGNAL(result(KJob*)), SLOT(itemFetchResult(KJob*)) );
  ++mPendingJobs;
  mTotalAmount = 0;
}

void NepomukFeederAgentBase::itemHeadersReceived(const Akonadi::Item::List& items)
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

void NepomukFeederAgentBase::itemFetchResult(KJob* job)
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

void NepomukFeederAgentBase::itemsReceived(const Akonadi::Item::List& items)
{
  kDebug() << items.size();
  foreach ( Item item, items ) {
    // we only get here if the item is not anywhere in Nepomuk yet, so no need to delete it
    item.setParentCollection( mCurrentCollection );
    updateItem( item, createGraphForEntity( item ) );
  }
  mProcessedAmount += items.count();
  emit percent( (mProcessedAmount * 100) / (mTotalAmount * 100) );
}


void NepomukFeederAgentBase::tagsFromCategories(NepomukFast::Resource& resource, const QStringList& categories)
{
  foreach ( const QString &category, categories ) {
    const Nepomuk::Tag tag( category );
    resource.addProperty( Soprano::Vocabulary::NAO::hasTag(), tag.resourceUri() );
  }
}


void NepomukFeederAgentBase::selfTest()
{
  QStringList errorMessages;

  // if Nepomuk is not running, try to start it
  if ( !mNepomukStartupAttempted && !QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.NepomukStorage" ) ) {
    KProcess process;
    if ( process.startDetached( QLatin1String( "nepomukserver" ) ) == 0 ) {
      errorMessages.append( i18n( "Unable to start the Nepomuk server." ) );
    } else {
      mNepomukStartupAttempted = true;
      mNepomukStartupTimeout.start();
      // wait for Nepomuk to start
      setOnline( false );
      emit status( Broken, i18n( "Waiting for the Nepomuk server to start..." ) );
      return;
    }
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
    mNepomukStartupAttempted = false; // everything worked, we can try again if the server goes down later
    if ( !mInitialUpdateDone ) {
      mInitialUpdateDone = true;
      QTimer::singleShot( 0, this, SLOT(updateAll()) );
    } else {
      emit status( Idle, i18n( "Ready to index data." ) );
    }
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

void NepomukFeederAgentBase::serviceOwnerChanged(const QString& name, const QString& oldOwner, const QString& newOwner)
{
  Q_UNUSED( oldOwner );
  Q_UNUSED( newOwner );

  if ( name == QLatin1String("org.kde.NepomukStorage") )
    selfTest();
}

NepomukFast::PersonContact NepomukFeederAgentBase::findOrCreateContact(const QString& emailAddress, const QString& name, const QUrl& graphUri, bool* found)
{
  //
  // Querying with the exact address string is not perfect since email addresses
  // are case insensitive. But for the moment we stick to it and hope Nepomuk
  // alignment fixes any duplicates
  //
  SelectSparqlBuilder::BasicGraphPattern graph;
  if ( emailAddress.isEmpty() ) {
    graph.addTriple( "?person", Vocabulary::NCO::fullname(), name );
  } else {
    graph.addTriple( "?person", Vocabulary::NCO::hasEmailAddress(), SparqlBuilder::QueryVariable( "?email" ) );
    graph.addTriple( "?email", Vocabulary::NCO::emailAddress(), emailAddress );
  }
  SelectSparqlBuilder qb;
  qb.setGraphPattern( graph );
  qb.addQueryVariable( "?person" );
  Soprano::QueryResultIterator it = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( qb.query(), Soprano::Query::QueryLanguageSparql );

  if ( it.next() ) {
    if ( found ) *found = true;
    const QUrl uri = it.binding( 0 ).uri();
    it.close();
    return NepomukFast::PersonContact( uri, graphUri );
  }
  if ( found ) *found = false;
  // create a new contact
  kDebug() << "Did not find " << name << emailAddress << ", creating a new PersonContact";
  NepomukFast::PersonContact contact( QUrl(), graphUri );
  contact.setLabel( name.isEmpty() ? emailAddress : name );
  if ( !emailAddress.isEmpty() ) {
    NepomukFast::EmailAddress emailRes( QUrl( "mailto:" + emailAddress ), graphUri );
    emailRes.setEmailAddress( emailAddress );
    contact.addEmailAddress( emailRes );
  }
  if ( !name.isEmpty() )
    contact.addFullname( name );
  return contact;
}

#include "nepomukfeederagentbase.moc"
