/* This file is part of the KDE project
   Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "restore.h"

#include <KDebug>
#include <KProcess>
#include <KStandardDirs>
#include <KTempDir>
#include <KUrl>

#include <QDir>

#include <kio/netaccess.h>

#include <akonadi/private/xdgbasedirs_p.h>


using namespace Akonadi;

/**
 * Use this class to restore a backup. possible() will tell you if all
 * apps needed for the restore are available. Don't proceed without them.
 * After that call restore() to get it running. Please make sure the parameter
 * has the tgz extention.
 */
Restore::Restore( QWidget *parent ) : QWidget( parent )
{
}

bool Restore::possible()
{
    const QString mysql = KStandardDirs::findExe( "mysql" );
    const QString bzip2 = KStandardDirs::findExe( "bzip2" );
    const QString tar = KStandardDirs::findExe( "tar" );
    kDebug() << "mysql:" << mysql << "bzip2:" << bzip2 << "tar:" << tar;
    return !mysql.isEmpty() && !bzip2.isEmpty() && !tar.isEmpty();
}

void Restore::restore( const KUrl& filename )
{
    if ( filename.isEmpty() ) {
        emit completed( false );
        return;
    }

    /* first create the temp folder. */
    KTempDir *tempDir = new KTempDir( KStandardDirs::locateLocal( "tmp", "akonadi" ) );
    tempDir->setAutoRemove( false );
    kDebug() << "Temp dir: "<< tempDir->name();

    /* Extract the nice tar file. */
    KProcess *proc = new KProcess( 0 );
    QStringList params;
    params << "-C" << tempDir->name();
    params << "-xjvf";
    params << filename.path();
    proc->setWorkingDirectory( tempDir->name() );
    proc->setProgram( KStandardDirs::findExe( "tar" ), params );
    kDebug() << "Executing: " << proc->program();
    int result = proc->execute();
    delete proc;
    kDebug() << result;
    if ( result != 0 ) {
        tempDir->unlink();
        delete tempDir;
        emit completed( false );
        return;
    }

    /* Copy over the configuration files. */
    QDir dir( tempDir->name() + "config/" );
    dir.setFilter( QDir::Files | QDir::Hidden | QDir::NoSymLinks );
    QFileInfoList list = dir.entryInfoList();
    for ( int i = 0; i < list.size(); ++i ) {
        QFileInfo fileInfo = list.at( i );
        const QString source = fileInfo.absoluteFilePath();
        const QString dest = KStandardDirs::locateLocal( "config", fileInfo.fileName() );

        kDebug() << "Restoring: " << source << "to:" << dest;
        KIO::NetAccess::file_copy( source, dest, this );
    }

    /* Restore the database */
    const QString socket = XdgBaseDirs::findResourceDir( "data",
                           "akonadi/db_misc/" ) + "mysql.socket";
    if ( socket.isEmpty() )
        kFatal() << "No socket found";

    proc = new KProcess( 0 );
    params.clear();
    params << "--socket=" + socket << "akonadi";
    proc->setStandardInputFile( tempDir->name() + "db/database.sql" );
    proc->setProgram( KStandardDirs::findExe( "mysql" ), params );
    kDebug() << "Executing: " << proc->program();
    result = proc->execute();
    delete proc;
    kDebug() << result;
    if ( result != 0 ) {
        tempDir->unlink();
        delete tempDir;
        emit completed( false );
        return;
    }

    tempDir->unlink();
    delete tempDir;
    emit completed( true );
}

#include "restore.moc"
