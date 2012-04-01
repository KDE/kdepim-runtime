/*
    Copyright (c) 2011 Frank Osterfeld <osterfeld@kde.org> (*)

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

//Heavily copy/pasted from kjotsmigrator.cpp, (C) 2010 Stephen Kelly <steveire@gmail.com>
#include "akregatormigrator.h"
#include "importitemsjob.h"
#include "settingsinterface.h"

#include <KConfigGroup>
#include <KSaveFile>
#include <KGlobal>
#include <KLocale>
#include <KRandom>
#include <KStandardDirs>

#include <Akonadi/AgentInstance>
#include <Akonadi/AgentInstanceCreateJob>
#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/EntityDisplayAttribute>
#include <akonadi/resourcesynchronizationjob.h>

#include <krss/feedcollection.h>
#include <krss/item.h>

#include <cerrno>
#include <functional>

using namespace Akonadi;
using namespace KRss;

static const char* defaultOpml =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<opml version=\"1.0\">"
"<head>"
"<text>Local RSS Feeds</text>"
"</head>"
"<body>"
"<outline text=\"KDE\">"
"   <outline title=\"KDE Dot News\" version=\"RSS\" description=\"\" htmlUrl=\"http://dot.kde.org\" xmlUrl=\"http://www.kde.org/dotkdeorg.rdf\" type=\"rss\" text=\"KDE Dot News\"/>"
"   <outline title=\"Planet KDE\" version=\"RSS\" description=\"Planet KDE - http://planetKDE.org/\" htmlUrl=\"http://planetKDE.org/\" xmlUrl=\"http://planetkde.org/rss20.xml\" type=\"rss\" text=\"Planet KDE\"/>"
"   <outline title=\"Planet KDE PIM\" version=\"RSS\" description=\"Planet KDE - http://planetKDE.org/\" htmlUrl=\"http://planetKDE.org/\" xmlUrl=\"http://pim.planetkde.org/rss20.xml\" type=\"rss\" text=\"Planet KDE PIM\"/>"
"   <outline title=\"KDE Apps\" version=\"RSS\" description=\"KDE-Look.org: Applications for your KDE-Desktop\" htmlUrl=\"http://kde-apps.org\" xmlUrl=\"http://www.kde.org/dot/kde-apps-content.rdf\" type=\"rss\" text=\"KDE Apps\"/>"
"   <outline title=\"KDE Look\"version=\"RSS\" description=\"KDE-Look.org: Eyecandy for your KDE-Desktop\" htmlUrl=\"http://kde-look.org\" xmlUrl=\"http://www.kde.org/kde-look-content.rdf\" text=\"KDE Look\"/>"
"  </outline>"
" </body>"
"</opml>";


AkregatorMigrator::AkregatorMigrator()
  : KMigratorBase()
{
}

AkregatorMigrator::~AkregatorMigrator()
{
}

void AkregatorMigrator::migrate()
{
  emit message( Info, i18n( "Beginning Akregator migration..." ) );

  createAgentInstance( "akonadi_krsslocal_resource", this, SLOT(resourceCreated(KJob*)) );
}

static bool ensureOpmlCreated( const QString& target, QString* err ) {
  const QString source = KGlobal::dirs()->saveLocation("data", "akregator/data") + "feeds.opml";

  //if there is a feeds.opml from Akregator, copy it over
  if ( QFileInfo( source ).exists() ) {
    QFile sf( source );
    if ( !sf.copy( target ) ) {
      *err = sf.errorString();
      return false;
    } else
      return true;
  }

  //otherwise, create a new one from the default feed list
  KSaveFile out( target );
  if ( !out.open( QIODevice::WriteOnly ) ) {
    *err = out.errorString();
    return false;
  }
  const qint64 len = strlen( defaultOpml );
  qint64 written = 0;
  while ( written < len ) {
    const qint64 n = out.write( defaultOpml + written, len - written );
    if ( n < 0 ) {
      *err = out.errorString();
      return false;
    }
    written += n;
  }
  if ( !out.finalize() ) {
    *err = out.errorString();
    return false;
  }

  return true;
}

void AkregatorMigrator::resourceCreated( KJob *job )
{
  if ( job->error() ) {
    emit message( Error, i18n( "Failed to create resource for RSS feeds: %1", job->errorText() ) );
    deleteLater();
    return;
  }
  emit message( Info, i18n( "Created local RSS resource." ) );

  AgentInstance instance = static_cast< AgentInstanceCreateJob* >( job )->instance();

  instance.setName( i18nc( "Default name for resource holding RSS feeds", "Local RSS Feeds" ) );

  OrgKdeAkonadiRssLocalSettingsInterface *iface = new OrgKdeAkonadiRssLocalSettingsInterface(
    "org.freedesktop.Akonadi.Resource." + instance.identifier(),
    "/Settings", QDBusConnection::sessionBus(), this );

  if (!iface->isValid() ) {
    migrationFailed( i18n("Failed to obtain D-Bus interface for remote configuration."), instance );
    delete iface;
    return;
  }

  const QString targetPath = KGlobal::dirs()->localxdgdatadir() + "/feeds/" + KRandom::randomString( 10 );

  errno = 0;
  if ( !QDir().mkpath( targetPath ) ) {
    migrationFailed( i18n("Failed to create folder for resource: %1", QString::fromLocal8Bit( strerror( errno ) ) ), instance );
    return;
  }

  const QString targetFile = targetPath + QLatin1String("/feeds.opml");
  QString err;
  if ( !ensureOpmlCreated( targetFile, &err ) ) {
    migrationFailed( i18n("Failed to create OPML file in %1: %2", targetPath, err ), instance );
    return;
  }

  QDBusPendingReply<void> response = iface->setPath( targetFile );

  // make sure the config is saved
  iface->writeConfig();

  instance.reconfigure();
  m_resourceIdentifier = instance.identifier();

  ResourceSynchronizationJob *syncJob = new ResourceSynchronizationJob(instance, this);
  syncJob->setCollectionTreeOnly( true );
  connect( syncJob, SIGNAL(result(KJob*)), SLOT(syncDone(KJob*)));
  syncJob->start();
}

void AkregatorMigrator::syncDone(KJob *job)
{
  if ( job->error() ) {
    emit message( Error, i18n( "Synchronizing the collection tree failed: %1" , job->errorString() ) );
    return;
  }
  emit message( Info, i18n( "Instance \"%1\" synchronized" , m_resourceIdentifier ) );

  CollectionFetchJob *collectionFetchJob = new CollectionFetchJob( Collection::root(), CollectionFetchJob::FirstLevel, this );
  connect( collectionFetchJob, SIGNAL(result(KJob*)), SLOT(rootCollectionsReceived(KJob*)) );
  collectionFetchJob->start();
}

void AkregatorMigrator::rootCollectionsReceived( KJob* j )
{
  CollectionFetchJob * job = qobject_cast<CollectionFetchJob*>( j );
  if ( job->error() ) {
    emit message( Error, i18nc( "A job to fetch akonadi collection failed. %1 is the error string.", "Fetching root collections failed: %1" , job->errorString() ) );
    return;
  }

  const Collection::List list = job->collections();
  foreach( const Collection& i, list ) {
    if ( i.resource() != m_resourceIdentifier )
      continue;

    FeedCollection fc( i );
    fc.setTitle( i18n("Local Feeds") );
    fc.setIsFolder( true );
    emit message( Info, i18n( "New resource is rooted at Collection (%1)", fc.id() ) );
    CollectionModifyJob* mjob = new CollectionModifyJob( fc, this );
    connect( mjob, SIGNAL(result(KJob*)), this, SLOT(rootCollectionRenamed(KJob*)) );
    mjob->start();
    return;
  }
  emit message( Error, i18n( "Could not find root collection for resource \"%1\"" ,m_resourceIdentifier ) );
}

void AkregatorMigrator::rootCollectionRenamed( KJob* j ) {
  if ( j->error() ) {
    emit message( Error, i18nc( "A job to rename the root collection failed. %1 is the error string.", "Could not rename root collection: %1" , j->errorString() ) );
    return;
  }

  CollectionModifyJob* job = qobject_cast<CollectionModifyJob*>( j );
  m_resourceCollection = job->collection();
  startMigration();
}

void AkregatorMigrator::startMigration()
{
  CollectionFetchJob* job = new CollectionFetchJob( m_resourceCollection, CollectionFetchJob::Recursive, this );
  job->fetchScope().setContentMimeTypes( QStringList( KRss::Item::mimeType() ) );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(feedCollectionsReceived(KJob*)) );
  connect( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)), this, SLOT(feedCollectionsReceived(Akonadi::Collection::List)) );
  job->start();
}

namespace {
  struct HasNoXmlUrl : public std::unary_function<Collection,bool> {
    bool operator()( const Collection& c ) const {
      return FeedCollection( c ).xmlUrl().isEmpty();
    }
  };
}
void AkregatorMigrator::feedCollectionsReceived( const Collection::List& l_ ) {
    Collection::List l( l_ );
    l.erase( std::remove_if( l.begin(), l.end(), HasNoXmlUrl() ), l.end() );
    m_unmigrated << l;
}

void AkregatorMigrator::feedCollectionsReceived( KJob* j ) {
  if ( j->error() ) {
    emit message( Error, i18n("Failed to retrieve feed collections: %1", j->errorText() ) );
    return;
  }
  migrateNext();
}

void AkregatorMigrator::migrateNext()
{
  if ( m_unmigrated.isEmpty() ) {
    migrationFinished();
    return;
  }

  const KRss::FeedCollection fc( m_unmigrated.front() );
  m_unmigrated.pop_front();

  emit message( Info, i18nc( "A migration tool is migrating the RSS feed with title %1", "Migrating \"%1\"..." , fc.title() ) );

  ImportItemsJob* job = new ImportItemsJob( fc, this );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(itemImportDone(KJob*)) );
  job->start();
}

void AkregatorMigrator::itemImportDone( KJob* j ) {
  ImportItemsJob* job = qobject_cast<ImportItemsJob*>( j );
  Q_ASSERT( job );
  if ( job->error() )
    emit message( Error, i18n( "Could not import items for feed \"%1\": %2", FeedCollection( job->collection() ).title(), job->errorText() ) );
  else
    emit message( Success, i18n( "Import done: \"%1\"", FeedCollection( job->collection() ).title() ) );
  migrateNext();
}

void AkregatorMigrator::migrationFinished()
{
  emit message( Info, i18n( "Akregator feeds migration finished" ) );
  deleteLater();
}

void AkregatorMigrator::migrationFailed( const QString& errorMsg, const Akonadi::AgentInstance& instance )
{
  Q_UNUSED( instance )
  emit message( Error, i18n( "Migration failed: %1" ,errorMsg ) );
}

#include "akregatormigrator.moc"
