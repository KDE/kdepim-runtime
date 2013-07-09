/*
 * Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "dirresourcebase.h"
#include "settings.h"
#include "dirsettingsdialog.h"
#include "settingsadaptor.h"

#include <QtCore/QDir>
#include <QtCore/QDirIterator>

#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/ItemFetchScope>

using namespace Akonadi;

#define FILE_EVENT_PROPERTY "FileEvent_"
#define FILE_PATH_PROPERTY "FilePath_"

DirResourceBase::DirResourceBase( const QString &id )
    : ResourceBase( id )
    , mDirWatch( 0 )
{
    // setup the resource
    new SettingsAdaptor( Settings::self() );
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                                Settings::self(), QDBusConnection::ExportAdaptors );

    changeRecorder()->itemFetchScope().fetchFullPayload();

    initializeDirWatch();
}

DirResourceBase::~DirResourceBase()
{
    mIgnoredPaths.clear();
}

void DirResourceBase::aboutToQuit()
{
    Settings::self()->writeConfig();
}

void DirResourceBase::clear()
{
}

void DirResourceBase::configure( WId windowId )
{
    SettingsDialog dlg( windowId );
    dlg.setWindowIcon( KIcon( "text-directory" ) );
    if ( dlg.exec() ) {
        initializeDirWatch();
        initializeDirectory();

        loadEntities();

        synchronize();

        emit configurationDialogAccepted();
    } else {
        emit configurationDialogRejected();
    }
}

void DirResourceBase::initializeDirWatch()
{
    delete mDirWatch;
    const QString dir = directoryName();
    if ( dir.isEmpty() ) {
        mDirWatch = 0;
        return;
    }

    mDirWatch = new KDirWatch( this );
    connect( mDirWatch, SIGNAL(created(QString)), this, SLOT(slotFileCreated(QString)) );
    connect( mDirWatch, SIGNAL(deleted(QString)), this, SLOT(slotFileDeleted(QString)) );
    connect( mDirWatch, SIGNAL(dirty(QString)), this, SLOT(slotDirChanged(QString)) );

    mDirWatch->addDir( dir, KDirWatch::WatchFiles );
    mDirWatch->startScan();
}

void DirResourceBase::initializeDirectory()
{
    QDir dir( directoryName() );

    // if folder does not exists, create it
    if ( !dir.exists() ) {
        QDir::root().mkpath( dir.absolutePath() );
    }
}

bool DirResourceBase::ensureReady(const QString &path, DirResourceBase::FileEvent event)
{
    if ( mIgnoredPaths.contains( path ) ) {
        kDebug() << "Path is in ignoredPath, skipping";
        mIgnoredPaths.removeAll( path );
        return false;
    }

    QFileInfo fInfo( path );
    if ( fInfo.exists() && !fInfo.isFile() ) {
        return false;
    }

    if ( !mCollection.isValid() ) {
        Collection col;
        col.setRemoteId( directoryName() );
        col.setParentCollection( Collection::root() );
        CollectionFetchJob *fetchJob = new CollectionFetchJob( col, CollectionFetchJob::Base, this );
        fetchJob->setProperty( FILE_EVENT_PROPERTY, event );
        fetchJob->setProperty( FILE_PATH_PROPERTY, path );
        connect( fetchJob, SIGNAL(finished(KJob*)),
                 this, SLOT(slotCollectionRetrieved(KJob*)) );
        return false;
    }

    return true;
}

void DirResourceBase::slotCollectionRetrieved( KJob *job )
{
    CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>( job );
    Q_ASSERT( fetchJob );

    if ( fetchJob->collections().isEmpty() ) {
        kWarning() << "Failed to retrieve parent collection";
        return;
    }

    mCollection = fetchJob->collections().first();

    const QString filePath = fetchJob->property( FILE_PATH_PROPERTY ).toString();
    const FileEvent event = static_cast<FileEvent>( fetchJob->property( FILE_EVENT_PROPERTY ).toUInt() );

    switch (event) {
        case DirResourceBase::FileCreated:
            slotFileCreated( filePath );
            break;
        case DirResourceBase::FileModified:
            slotDirChanged( filePath );
            break;
        case DirResourceBase::FileDeleted:
            slotFileDeleted( filePath );
            break;
        default:
            Q_ASSERT( !"Invalid event" );
    }
}

bool DirResourceBase::loadEntities()
{
    clear();

    QDirIterator it( directoryName() );
    while ( it.hasNext() ) {
        it.next();
        if ( it.fileName() != "." && it.fileName() != ".." && it.fileName() != "WARNING_README.txt" ) {
            loadEntity( it.filePath() );
        }
    }

    emit status( Idle );

    return true;
}

QString DirResourceBase::directoryName() const
{
    return Settings::self()->path();
}

QString DirResourceBase::directoryFileName( const QString &file ) const
{
    return Settings::self()->path() + QDir::separator() + file;
}

Collection DirResourceBase::createCollection() const
{
    Akonadi::Collection c;
    c.setParentCollection( Akonadi::Collection::root() );
    c.setRemoteId( directoryName() );
    c.setName( name() );

    c.setContentMimeTypes( QStringList() << mimeType() );
    if ( Settings::self()->readOnly() ) {
        c.setRights( Akonadi::Collection::CanChangeCollection );
    } else {
        Akonadi::Collection::Rights rights = Akonadi::Collection::ReadOnly;
        rights |= Akonadi::Collection::CanChangeItem;
        rights |= Akonadi::Collection::CanCreateItem;
        rights |= Akonadi::Collection::CanDeleteItem;
        rights |= Akonadi::Collection::CanChangeCollection;
        c.setRights( rights );
    }

    return c;
}


#include "dirresourcebase.moc"
