/*
    Copyright (C) 2009    Frank Osterfeld <osterfeld@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "importitemsjob.h"
#include <krss/rssitem.h>
#include <krss/feedcollection.h>
#include "itemsync.h"

#include <Akonadi/Item>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemFetchJob>

#include <KLocalizedString>
#include <KRandom>
#include <KTemporaryFile>

#include <QFile>
#include <QString>
#include <QUrl>

#include <algorithm>

static QString exporterBinaryName() {
    return QString::fromLatin1( "akregatorstorageexporter" );
}

static void deleteTempFile( const QString& path ) {
    if ( !path.isEmpty() && QFile::exists( path ) ) {
        QFile f( path );
        if ( !f.remove( path ) )
            kWarning() << "Could not delete temporary file" << path << f.errorString();
    }
}

static QString generateTmpFileName() {
    KTemporaryFile file;
    file.setSuffix( KRandom::randomString( 10 ) );
    if ( !file.open() )
        return QString();
    return file.fileName();
}

using KRss::RssItem;

ImportItemsJob::ImportItemsJob( const Akonadi::Collection& collection, QObject *parent )
    : KJob( parent )
    , m_exporter( 0 )
    , m_collection( collection )
    , m_reader( 0 )
    , m_pendingCreateJobs( 0 ) {
}

ImportItemsJob::~ImportItemsJob() {
    deleteTempFile( m_exportFileName );
    delete m_reader;
}

Akonadi::Collection ImportItemsJob::collection() const {
    return m_collection;
}

void ImportItemsJob::cleanupAndEmitResult() {
    delete m_reader;
    m_reader = 0;
    m_file.close();
    emitResult();
}

void ImportItemsJob::start() {
    QMetaObject::invokeMethod( this, "doStart", Qt::QueuedConnection );
}

void ImportItemsJob::doStart() {
    assert( !m_exporter );
    assert( m_exportFileName.isEmpty() );
    m_exporter = new QProcess( this );
    m_exportFileName = generateTmpFileName();
    if ( m_exportFileName.isEmpty() || QFile::exists( m_exportFileName ) ) {
        setError( ImportItemsJob::ExporterError );
        setErrorText( i18n("Could not generate a temporary file for export." ) );
        cleanupAndEmitResult();
        return;
    }

    m_exporter->setStandardOutputFile( m_exportFileName );
    connect( m_exporter, SIGNAL(finished(int,QProcess::ExitStatus)),
             this, SLOT(exporterFinished(int,QProcess::ExitStatus)) );
    connect( m_exporter, SIGNAL(error(QProcess::ProcessError)),
             this, SLOT(exporterError(QProcess::ProcessError)) );

    m_exporter->start( exporterBinaryName(), QStringList() << QString::fromLatin1( "--base64" ) << QString::fromAscii( QUrl( KRss::FeedCollection( m_collection ).xmlUrl() ).toEncoded().toBase64() ) );
    if ( !m_exporter->waitForStarted() ) {
        setError( ImportItemsJob::ExporterError );
        setErrorText( i18n("Could not start storage exporter %1. Make sure it is properly installed.", exporterBinaryName() ) );
        cleanupAndEmitResult();
        return;
    }
}

void ImportItemsJob::exporterError( QProcess::ProcessError error ) {
    setError( ImportItemsJob::ExporterError );
    switch ( error ) {
        case QProcess::FailedToStart:
            setErrorText( i18n( "Storage exporter failed to start." ) );
            break;
        case QProcess::Crashed:
            setErrorText( i18n( "Storage exporter crashed." ) );
            break;
        case QProcess::Timedout:
            setErrorText( i18n( "Storage exporter communication timed out." ) );
            break;
        case QProcess::WriteError:
            setErrorText( i18n( "Storage exporter write error." ) );
            break;
        case QProcess::ReadError:
            setErrorText( i18n( "Storage exporter read error." ) );
            break;
        case QProcess::UnknownError:
            setErrorText( i18n( "Storage exporter: unknown error" ) );
            break;
    }
    cleanupAndEmitResult();
}

void ImportItemsJob::exporterFinished( int exitCode, QProcess::ExitStatus status ) {
    if ( status == QProcess::CrashExit ) {
        setError( ImportItemsJob::ExporterError );
        setErrorText( i18n("Item export failed. Storage exporter %1 crashed.", exporterBinaryName() ) );
        cleanupAndEmitResult();
        return;
    }

    if ( exitCode != 0 ) {
        setError( ImportItemsJob::ExporterError );
        setErrorText( i18n("Item export failed. Storage exporter %1 returned exit code %2.", exporterBinaryName(), QString::number( exitCode ) ) );
        cleanupAndEmitResult();
        return;
    }

    Akonadi::ItemFetchJob* job = new Akonadi::ItemFetchJob( m_collection, this );
    //TODO set fetch scope
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotItemsFetched(KJob*)) );
    job->start();
}

static bool lessByRemoteId( const Akonadi::Item& lhs, const Akonadi::Item& rhs ) {
    return lhs.remoteId() < rhs.remoteId();
}

static bool equalByRemoteId( const Akonadi::Item& lhs, const Akonadi::Item& rhs ) {
    return lhs.remoteId() == rhs.remoteId();
}

void ImportItemsJob::slotItemsFetched( KJob* j ) {
    const Akonadi::ItemFetchJob* const job = qobject_cast<const Akonadi::ItemFetchJob*>( j );
    assert( job );
    if ( job->error() ) {
        setError( ItemSyncFailed );
        setErrorText( i18n( "Could not retrieve existing items for syncing: %1", job->errorString() ) );
        cleanupAndEmitResult();
        return;
    }

    m_existingItems = job->items();
    std::sort( m_existingItems.begin(), m_existingItems.end(), lessByRemoteId );
    startImport();
}

void ImportItemsJob::startImport() {
    m_file.setFileName( m_exportFileName );
    if ( !m_file.open( QIODevice::ReadOnly ) ) {
        setError( CouldNotOpenFile );
        setErrorText( i18n("Could not open file %1 for reading: %2", m_exportFileName, m_file.errorString() ) );
        cleanupAndEmitResult();
        return;
    }
    m_reader = new ItemImportReader( &m_file );
    readBatch();
}

void ImportItemsJob::readBatch() {
    int num = 500;

    Akonadi::Item::List items;

    while ( num > 0 && m_reader->hasNext() ) {
        const Akonadi::Item item = m_reader->nextItem();
        if ( !item.remoteId().isEmpty() )
            items.append( item );
        --num;
    }

    if ( items.isEmpty() ) {
        cleanupAndEmitResult();
        return;
    }

    syncItems( items );
}

void ImportItemsJob::syncItems( const Akonadi::Item::List& items_ ) {
    Q_ASSERT( !items_.isEmpty() );
#if 0
    std::sort( items.begin(), items.end(), lessByRemoteId );

    //split into existing items (must be merged) and new items (will be just added)
    //Akonadi::Item::List toMerge;
    Akonadi::Item::List toAdd;
    Akonadi::Item::List::const_iterator itRead = items.constBegin();

    if ( m_existingItems.isEmpty() )
        toAdd = items;
#endif

#if 0
    else {
        Akonadi::Item::List::const_iterator itExisting = m_existingItems.constBegin();
        bool foundLast = false;
        for ( ; itRead != items.constEnd(); ++itRead ) {
            if ( foundLast ) {
                toAdd.push_back( *itRead );
                continue;
            }
            itExisting = std::lower_bound( itExisting, m_existingItems.constEnd(), *itRead, lessByRemoteId );
            foundLast = itExisting == items.constEnd();
            if ( foundLast ) {
                toAdd.push_back( *itRead );
                continue;
            }

            if ( equalByRemoteId( *itExisting, *itRead ) )
                toMerge.push_back( *itRead );
            else
                toAdd.push_back( *itRead );
        }
    }
    //TODO: merge flags etc. for existing items

    if ( toAdd.isEmpty() ) {
        QMetaObject::invokeMethod( this, "readBatch", Qt::QueuedConnection );
        return;
    }

    Q_FOREACH( const Akonadi::Item& item, items_/*toAdd*/ ) {
        Akonadi::ItemCreateJob* const job = new Akonadi::ItemCreateJob( item, m_collection );
        ++m_pendingCreateJobs;
        QObject::connect(job, SIGNAL(result(KJob*)), this, SLOT(syncDone(KJob*)) );
        job->start();
    }
#endif
    RssItemSync * const syncer = new RssItemSync( m_collection );
    syncer->setIncrementalSyncItems( items_, Akonadi::Item::List() );
    syncer->setSynchronizeFlags( false );
    connect( syncer, SIGNAL( result( KJob* ) ), this, SLOT( syncDone( KJob* ) ) );
    ++m_pendingCreateJobs;
    syncer->start();
}

void ImportItemsJob::syncDone( KJob* job ) {
    if ( job->error() ) {
        setError( ItemSyncFailed );
        setErrorText( job->errorText() );
        cleanupAndEmitResult();
        return;
    }
    assert( m_pendingCreateJobs > 0 );
    --m_pendingCreateJobs;
    if ( m_pendingCreateJobs == 0 )
       readBatch();
}
