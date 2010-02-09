/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "knotesmigrator.h"

#include "icalsettings.h"
#include "birthdayssettings.h"

#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agenttype.h>
#include <akonadi/resourcesynchronizationjob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/item.h>
#include "entitytreecreatejob.h"
#include <KMime/Message>

#include <KDebug>
#include <kjots/maildirsettings.h>
#include <KCal/CalendarLocal>


using namespace Akonadi;

KNotesMigrator::KNotesMigrator() :
    KResMigrator<KRES::Resource>( "notes", QString() ), m_notesResource( 0 )
{
}

KNotesMigrator::~KNotesMigrator()
{
  delete m_notesResource;
}

bool KNotesMigrator::migrateResource( KRES::Resource* res)
{
  kDebug() << res->identifier() << res->type();
  if ( res->type() == "file" )
    createAgentInstance( "akonadi_akonotes_resource", this, SLOT(notesResourceCreated(KJob*)) );
  else if ( res->type() == "kolab" )
  {
    kDebug() << "TODO: Handle kolab notes resource";
  }
  else
    return false;
  return true;
}

void KNotesMigrator::notesResourceCreated(KJob * job)
{
  if ( job->error() ) {
    migrationFailed( i18n("Failed to create resource: %1", job->errorText()) );
    return;
  }

  KRES::Resource *res = currentResource();
  m_agentInstance = static_cast<AgentInstanceCreateJob*>( job )->instance();
  const KConfigGroup kresCfg = kresConfig( res );
  m_agentInstance.setName( kresCfg.readEntry( "ResourceName", "Migrated Notes" ) );

  QString resourcePath = kresCfg.readEntry( "NotesURL" );
  KUrl url(resourcePath);

  m_notesResource = new KCal::CalendarLocal(QString());

  bool success = m_notesResource->load(url.toLocalFile());
  if (!success)
  {
    migrationFailed( i18n( "Failed to open file for reading: %1" , resourcePath ) );
    return;
  }

  OrgKdeAkonadiMaildirSettingsInterface *iface = new OrgKdeAkonadiMaildirSettingsInterface(
    "org.freedesktop.Akonadi.Resource." + m_agentInstance.identifier(),
    "/Settings", QDBusConnection::sessionBus(), this );

  if (!iface->isValid() ) {
    migrationFailed( i18n("Failed to obtain D-Bus interface for remote configuration."), m_agentInstance );
    return;
  }
  iface->setReadOnly( res->readOnly() );

  // Use a path like local/share/notes/<randomstring>/...maildir...

  QDBusPendingReply<void> response = iface->setPath( "~/.local/share/notes" );
  kDebug() << response.error().message();

  m_agentInstance.reconfigure();

  ResourceSynchronizationJob *syncJob = new ResourceSynchronizationJob(m_agentInstance, this);
  connect( syncJob, SIGNAL(result(KJob*)), SLOT(syncDone(KJob*)));
  syncJob->start();
}

void KNotesMigrator::syncDone(KJob *job)
{
  kDebug();
  emit message( Info, i18n( "Instance \"%1\" syncronized" , m_agentInstance.identifier() ) );

  CollectionFetchJob *collectionFetchJob = new CollectionFetchJob( Collection::root(), CollectionFetchJob::FirstLevel, this );
  connect( collectionFetchJob, SIGNAL(collectionsReceived(Akonadi::Collection::List)), SLOT(rootCollectionsRecieved(Akonadi::Collection::List)) );
  connect( collectionFetchJob, SIGNAL(result(KJob*)), SLOT( rootFetchFinished(KJob*)) );
}

void KNotesMigrator::rootFetchFinished( KJob *job )
{
  kDebug() << job->error();
  if ( job->error() ) {
    emit message( Error, i18nc( "A job to fetch akonadi resources failed. %1 is the error string.", "Fetching resources failed: %1" , job->errorString() ) );
  }
}

void KNotesMigrator::rootCollectionsRecieved( const Akonadi::Collection::List &list )
{
  kDebug();
  foreach( const Collection &collection, list ) {
    if ( collection.resource() == m_agentInstance.identifier() ) {
      m_resourceCollection = collection;
      startMigration();
      return;
    }
  }
  emit message( Error, i18n( "Couldn't find root collection for resource \"%1\"" ,m_agentInstance.identifier() ) );
}

void KNotesMigrator::startMigration()
{
  KCal::Journal::List oldNotesList = m_notesResource->rawJournals();
  Akonadi::Item::List newItemsList;

  kDebug() << oldNotesList.size();
  foreach ( KCal::Journal *journal, oldNotesList )
  {
    Item newItem;
    newItem.setMimeType( "text/x-vnd.akonadi.note" );
    newItem.setParentCollection( m_resourceCollection );
    KMime::Message::Ptr note( new KMime::Message() );

    QByteArray encoding("utf-8");
    // TODO: get from journal.
    note->subject( true )->fromUnicodeString( journal->summary(), encoding );
    note->mainBodyPart()->fromUnicodeString( journal->description() );
//     note->contentType( true )-> rich text?

    note->assemble();

    // TODO: Consider an attribute for existing alarms.

    newItem.setPayload( note );
    newItemsList.append( newItem );
  }

  EntityTreeCreateJob *createJob = new EntityTreeCreateJob( QList<Akonadi::Collection::List>(), newItemsList,this );
  connect(createJob, SIGNAL(result(KJob*)), SLOT(newResourceFilled(KJob*)));
}

void KNotesMigrator::newResourceFilled(KJob* job)
{
  migrationCompleted( m_agentInstance );
}




#include "knotesmigrator.moc"
