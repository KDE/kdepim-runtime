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
#include "global.h"

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
 * has the tar.bz2 extension.
 */
Restore::Restore( QWidget *parent ) : QWidget( parent )
{
}

bool Restore::possible()
{
    Tray::Global global;
    QString dbRestoreAppName;
    if( global.dbdriver() == QLatin1String("QPSQL") )
      dbRestoreAppName = QLatin1String("pg_restore");
    else if( global.dbdriver() == QLatin1String("QMYSQL") )
      dbRestoreAppName = QLatin1String("mysql");
    else {
      kError() << "Could not find an application to restore the database.";
    }

    m_dbRestoreApp = KStandardDirs::findExe( dbRestoreAppName );
    const QString bzip2 = KStandardDirs::findExe( QLatin1String("bzip2") );
    const QString tar = KStandardDirs::findExe( QLatin1String("tar") );
    kDebug() << "m_dbRestoreApp:" << m_dbRestoreApp << "bzip2:" << bzip2 << "tar:" << tar;
    return !m_dbRestoreApp.isEmpty() && !bzip2.isEmpty() && !tar.isEmpty();
}

void Restore::restore( const KUrl& filename )
{
    if ( filename.isEmpty() ) {
        emit completed( false );
        return;
    }

    const QString sep = QDir::separator();

    /* first create the temp folder. */
    KTempDir *tempDir = new KTempDir( KStandardDirs::locateLocal( "tmp", QLatin1String("akonadi") ) );
    tempDir->setAutoRemove( false );
    kDebug() << "Temp dir: "<< tempDir->name();

    /* Extract the nice tar file. */
    KProcess *proc = new KProcess( this );
    QStringList params;
    params << QLatin1String("-C") << tempDir->name();
    params << QLatin1String("-xjf");
    params << filename.toLocalFile();
    proc->setWorkingDirectory( tempDir->name() );
    proc->setProgram( KStandardDirs::findExe( QLatin1String("tar") ), params );
    int result = proc->execute();
    delete proc;
    if ( result != 0 ) {
        kWarning() << "Executed:" << KStandardDirs::findExe( QLatin1String("tar") ) << params << " Result: " << result;
        tempDir->unlink();
        delete tempDir;
        emit completed( false );
        return;
    }

    /* Copy over the KDE configuration files. */
    QDir dir( tempDir->name() + QLatin1String("kdeconfig") + sep );
    dir.setFilter( QDir::Files | QDir::Hidden | QDir::NoSymLinks );
    QFileInfoList list = dir.entryInfoList();
    const int numberOfElement = list.size();
    for ( int i = 0; i < numberOfElement; ++i ) {
        QFileInfo fileInfo = list.at( i );
        const QString source = fileInfo.absoluteFilePath();
        const QString dest = KStandardDirs::locateLocal( "config", fileInfo.fileName() );

        kDebug() << "Restoring: " << source << "to:" << dest;
        KIO::NetAccess::file_copy( source, dest, this );
    }

    /* Copy over the Akonadi configuration files. */
    const QString akonadiconfigfolder = XdgBaseDirs::findResourceDir( "config", QLatin1String("akonadi") );
    dir.setPath( tempDir->name() + QLatin1String("akonadiconfig") + sep );
    dir.setFilter( QDir::Files | QDir::Hidden | QDir::NoSymLinks );
    list = dir.entryInfoList();
    const int sizeOfList = list.size();
    for ( int i = 0; i < sizeOfList; ++i ) {
        QFileInfo fileInfo = list.at( i );
        const QString source = fileInfo.absoluteFilePath();
        const QString dest = akonadiconfigfolder + sep + fileInfo.fileName();

        kDebug() << "Restoring: " << source << "to:" << dest;
        KIO::NetAccess::file_copy( source, dest, this );
    }

    /* Restore the database */
    Tray::Global global;
    proc = new KProcess( this );
    params.clear();

    if( global.dbdriver() == QLatin1String("QPSQL") ) {
      params << global.dboptions()
             << QLatin1String("--dbname=") + global.dbname()
             << QLatin1String("--format=custom")
             << QLatin1String("--clean")
             << QLatin1String("--no-owner")
             << QLatin1String("--no-privileges")
             << tempDir->name() + QLatin1String("db") + sep + QLatin1String("database.sql");
    }
    else if (global.dbdriver() == QLatin1String("QMYSQL") ) {
      params << global.dboptions()
             << QLatin1String("--database=") + global.dbname();
    }

    kDebug() << "Executing:" << m_dbRestoreApp << params;
    ;
    proc->setProgram( KStandardDirs::findExe( m_dbRestoreApp ), params );
    proc->setStandardInputFile(tempDir->name() + QLatin1String("db") + sep + QLatin1String("database.sql"));
    result = proc->execute();
    delete proc;
    if ( result != 0 ) {
        kWarning() << "Executed:" << m_dbRestoreApp << params << " Result: " << result;
        tempDir->unlink();
        delete tempDir;
        emit completed( false );
        return;
    }

    tempDir->unlink();
    delete tempDir;
    emit completed( true );
}

