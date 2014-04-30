/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>
    Copyright (c) 2013 Laurent Montel <montel@kde.org>

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
#include "notelockattribute.h"
#include "notealarmattribute.h"
#include "notedisplayattribute.h"
#include "showfoldernotesattribute.h"
#include "knotesmigratorconfig.h"
#include "knoteconfig.h"

#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agenttype.h>
#include <akonadi/resourcesynchronizationjob.h>
#include <akonadi/collectionfetchjob.h>
#include <Akonadi/AttributeFactory>
#include <Akonadi/CollectionModifyJob>
#include <akonadi/item.h>
#include "entitytreecreatejob.h"
#include <KMime/Message>

#include <KDebug>
#include <KStandardDirs>
#include "maildirsettings.h"
#include <krandom.h>


using namespace Akonadi;

KNotesMigrator::KNotesMigrator() :
    KMigratorBase(), mIndexResource(-1), m_notesResource( 0 )
{
    Akonadi::AttributeFactory::registerAttribute<NoteLockAttribute>();
    Akonadi::AttributeFactory::registerAttribute<NoteAlarmAttribute>();
    Akonadi::AttributeFactory::registerAttribute<NoteDisplayAttribute>();
    Akonadi::AttributeFactory::registerAttribute<ShowFolderNotesAttribute>();
    const QString kresCfgFile = KStandardDirs::locateLocal( "config", QLatin1String( "kresources/notes/stdrc" ) );
    mConfig = new KConfig( kresCfgFile );
    const KConfigGroup generalGroup( mConfig, QLatin1String( "General" ) );
    mUnknownTypeResources = generalGroup.readEntry( QLatin1String( "ResourceKeys" ), QStringList() );
    m_notesResource = new KCal::CalendarLocal( QString() );
}

KNotesMigrator::~KNotesMigrator()
{
    delete m_notesResource;
    delete mConfig;
}

void KNotesMigrator::migrate()
{
    emit message( Info, i18n( "Beginning KNotes migration..." ) );
    migrateNext();
}

void KNotesMigrator::migrateNext()
{
    ++mIndexResource;

    if (mUnknownTypeResources.isEmpty() || mIndexResource >= mUnknownTypeResources.count()) {
        emit message( Info, i18n( "KNotes migration finished" ) );
        deleteLater();
        return;
    }

    const KConfigGroup kresCfgGroup( mConfig, QString::fromLatin1( "Resource_%1" ).arg( mUnknownTypeResources.at(mIndexResource) ) );
    const QString resourceType = kresCfgGroup.readEntry( QLatin1String( "ResourceType" ), QString() );
    if (resourceType == QLatin1String("file")) {
        createAgentInstance( QLatin1String("akonadi_akonotes_resource"), this, SLOT(notesResourceCreated(KJob*)) );
    } else {
        migrateNext();
    }
}

void KNotesMigrator::notesResourceCreated(KJob * job)
{
    if ( job->error() ) {
        migrationFailed( i18n( "Failed to create resource: %1", job->errorText() ) );
        migrateNext();
        return;
    }

    const KConfigGroup kresCfgGroup( mConfig, QString::fromLatin1( "Resource_%1" ).arg( mUnknownTypeResources.at(mIndexResource) ) );

    m_agentInstance = static_cast<AgentInstanceCreateJob*>( job )->instance();
    m_agentInstance.setName( kresCfgGroup.readEntry( "ResourceName", "Migrated Notes" ) );

    const QString resourcePath = kresCfgGroup.readEntry( "NotesURL" );
    KUrl url( resourcePath );

    if ( !QFile::exists( url.toLocalFile() ) ) {
        migrateNext();
        return;
    }


    const bool success = m_notesResource->load( url.toLocalFile() );
    if ( !success ) {
        migrationFailed( i18n( "Failed to open file for reading: %1" , resourcePath ) );
        migrateNext();
        return;
    }

    OrgKdeAkonadiMaildirSettingsInterface *iface = new OrgKdeAkonadiMaildirSettingsInterface(
                QLatin1String("org.freedesktop.Akonadi.Resource.") + m_agentInstance.identifier(),
                QLatin1String("/Settings"), QDBusConnection::sessionBus(), this );

    if ( !iface->isValid() ) {
        migrationFailed( i18n( "Failed to obtain D-Bus interface for remote configuration." ), m_agentInstance );
        delete iface;
        migrateNext();
        return;
    }
    const bool isReadOnly = kresCfgGroup.readEntry("ResourceIsReadOnly", false);
    iface->setReadOnly( isReadOnly );

    QDBusPendingReply<void> response = iface->setPath( KGlobal::dirs()->localxdgdatadir() + QLatin1String("/notes/") + KRandom::randomString( 10 ) );

    // make sure the config is saved
    iface->writeConfig();

    m_agentInstance.reconfigure();

    ResourceSynchronizationJob *syncJob = new ResourceSynchronizationJob( m_agentInstance, this );
    connect( syncJob, SIGNAL(result(KJob*)), SLOT(syncDone(KJob*)));
    syncJob->start();
}

void KNotesMigrator::syncDone(KJob *job)
{
    Q_UNUSED( job );
    emit message( Info, i18n( "Instance \"%1\" synchronized" , m_agentInstance.identifier() ) );

    CollectionFetchJob *collectionFetchJob = new CollectionFetchJob( Collection::root(), CollectionFetchJob::FirstLevel, this );
    connect( collectionFetchJob, SIGNAL(collectionsReceived(Akonadi::Collection::List)), SLOT(rootCollectionsRecieved(Akonadi::Collection::List)) );
    connect( collectionFetchJob, SIGNAL(result(KJob*)), SLOT(rootFetchFinished(KJob*)) );
}

void KNotesMigrator::rootFetchFinished( KJob *job )
{
    emit message( Info, i18n( "Root fetch finished" ) );
    if ( job->error() ) {
        emit message( Error, i18nc( "A job to fetch akonadi resources failed. %1 is the error string.", "Fetching resources failed: %1" , job->errorString() ) );
    }
}

void KNotesMigrator::rootCollectionsRecieved( const Akonadi::Collection::List &list )
{
    emit message( Info, i18n( "Received root collections" ) );
    foreach ( const Collection &collection, list ) {
        if ( collection.resource() == m_agentInstance.identifier() ) {
            m_resourceCollection = collection;
            startMigration();
            return;
        }
    }
    emit message( Error, i18n( "Could not find root collection for resource \"%1\"" ,m_agentInstance.identifier() ) );
    migrateNext();
}

void KNotesMigrator::startMigration()
{
    KCal::Journal::List oldNotesList = m_notesResource->rawJournals();
    Akonadi::Item::List newItemsList;
    KConfig config(QLatin1String("globalnotesettings"));
    KConfigGroup grp = config.group(QLatin1String("SelectNoteFolder"));
    grp.writeEntry("DefaultFolder", m_resourceCollection.id());
    config.sync();

    emit message( Info, i18np( "Starting migration of %1 note", "Starting migration of %1 notes", oldNotesList.size() ) );

    foreach ( KCal::Journal *journal, oldNotesList ) {
        Item newItem;
        newItem.setMimeType( QLatin1String("text/x-vnd.akonadi.note") );
        newItem.setParentCollection( m_resourceCollection );
        KMime::Message::Ptr note( new KMime::Message() );

        QByteArray encoding( "utf-8" );
        note->subject( true )->fromUnicodeString( journal->summary(), encoding );
        note->mainBodyPart()->fromUnicodeString( journal->description() );
        note->contentType( true )->setMimeType( journal->descriptionIsRich() ? "text/html" : "text/plain" );

        note->assemble();
        KNotesMigratorConfig *config = new KNotesMigratorConfig(journal);
        if (config) {

            if (config->readOnly()) {
                newItem.addAttribute( new NoteLockAttribute() );
            }

            //Position/Editor/Color etc.
            NoteDisplayAttribute *displayAttribute = new NoteDisplayAttribute();
            displayAttribute->setBackgroundColor(config->noteConfig()->bgColor());
            displayAttribute->setForegroundColor(config->noteConfig()->fgColor());
            displayAttribute->setSize(QSize(config->noteConfig()->width(), config->noteConfig()->height()));
            displayAttribute->setRememberDesktop(config->noteConfig()->rememberDesktop());
            displayAttribute->setTabSize(config->noteConfig()->tabSize());
            displayAttribute->setFont(config->noteConfig()->font());
            displayAttribute->setTitleFont(config->noteConfig()->titleFont());
            displayAttribute->setDesktop(config->noteConfig()->desktop());
            displayAttribute->setIsHidden(config->noteConfig()->hideNote());
            displayAttribute->setPosition(config->noteConfig()->position());
            displayAttribute->setShowInTaskbar(config->noteConfig()->showInTaskbar());
            displayAttribute->setKeepAbove(config->noteConfig()->keepAbove());
            displayAttribute->setKeepBelow(config->noteConfig()->keepBelow());
            displayAttribute->setAutoIndent(config->noteConfig()->autoIndent());
            newItem.addAttribute( displayAttribute );
            delete config;
        }

        //Alarm.
        //In note we have an unique alarm.
        if (!journal->alarms().isEmpty()) {
            KCal::Alarm *alarm = journal->alarms().first();
            if (alarm->hasTime()) {
                NoteAlarmAttribute *alarmAttribute = new NoteAlarmAttribute;
                alarmAttribute->setDateTime(alarm->time());
                newItem.addAttribute( alarmAttribute );
            }
        }
        newItem.setPayload( note );
        newItemsList.append( newItem );
    }

    EntityTreeCreateJob *createJob = new EntityTreeCreateJob( QList<Akonadi::Collection::List>(), newItemsList,this );
    connect(createJob, SIGNAL(result(KJob*)), SLOT(newResourceFilled(KJob*)));
}

void KNotesMigrator::newResourceFilled(KJob* job)
{
    Q_UNUSED( job );
    showDefaultCollection();
}

void KNotesMigrator::showDefaultCollection()
{
    ShowFolderNotesAttribute *attribute = m_resourceCollection.attribute<ShowFolderNotesAttribute>( Akonadi::Collection::AddIfMissing );
    Q_UNUSED(attribute);
    Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( m_resourceCollection );
    connect(job, SIGNAL(result(KJob*)), SLOT(slotCollectionModify(KJob*)));
}

void KNotesMigrator::slotCollectionModify(KJob* job)
{
    Q_UNUSED( job );
    migrateNext();
}

void KNotesMigrator::migrationFailed( const QString& errorMsg, const Akonadi::AgentInstance& instance )
{
    Q_UNUSED( instance )
    emit message( Error, i18n( "Migration failed: %1" ,errorMsg ) );
}

