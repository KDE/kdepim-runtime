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

#include "backup.h"
#include "global.h"

#include <KDebug>
#include <KIO/NetAccess>
#include <KProcess>
#include <KStandardDirs>
#include <KTempDir>
#include <KUrl>

#include <QDir>

#include <akonadi/private/xdgbasedirs_p.h>
#include <akonadi/agentmanager.h>


using namespace Akonadi;

/**
 * Use this class to create a backup. possible() will tell you if all
 * apps needed for the backup are available. Don't proceed without them.
 * After that call create() to get it running. Please make sure the parameter
 * has the tar.bz2 extension.
 */
Backup::Backup( QWidget *parent ) : QWidget( parent )
{
}

bool Backup::possible()
{
    Tray::Global global;
    QString dbDumpAppName;
    if( global.dbdriver() == QLatin1String("QPSQL") )
      dbDumpAppName = QLatin1String("pg_dump");
    else if( global.dbdriver() == QLatin1String("QMYSQL") )
      dbDumpAppName = QLatin1String("mysqldump");
    else {
      kError() << "Could not find an application to dump the database.";
    }

    m_dbDumpApp = KStandardDirs::findExe( dbDumpAppName );
    const QString bzip2 = KStandardDirs::findExe( QLatin1String("bzip2") );
    const QString tar = KStandardDirs::findExe( QLatin1String("tar") );
    kDebug() << "m_dbDumpApp:" << m_dbDumpApp << "bzip2:" << bzip2 << "tar:" << tar;
    return !m_dbDumpApp.isEmpty() && !bzip2.isEmpty() && !tar.isEmpty();
}

void Backup::create( const KUrl& filename )
{
    if ( filename.isEmpty() ) {
        emit completed( false );
        return;
    }

    const QString sep( QDir::separator() );
    /* first create the temp folder. */
    KTempDir *tempDir = new KTempDir( KStandardDirs::locateLocal( "tmp", QLatin1String("akonadi") ) );
    tempDir->setAutoRemove( false );
    KIO::NetAccess::mkdir( QString(tempDir->name() + QLatin1String("kdeconfig")), this );
    KIO::NetAccess::mkdir( QString(tempDir->name() + QLatin1String("akonadiconfig")), this );
    KIO::NetAccess::mkdir( QString(tempDir->name() + QLatin1String("db")), this );

    QStringList filesToBackup;

    /* Copy over the KDE config files. */
    AgentManager *manager = AgentManager::self();
    const AgentInstance::List list = manager->instances();
    foreach( const AgentInstance &agent, list ) {
        const QString agentFileName = agent.identifier() + QLatin1String("rc");
        const QString configFileName = KStandardDirs::locateLocal( "config", agentFileName );
        bool exists = KIO::NetAccess::exists( configFileName, KIO::NetAccess::DestinationSide, this );
        if ( exists ) {
            KIO::NetAccess::file_copy( configFileName,
                                       QString(tempDir->name() + QLatin1String("kdeconfig") + sep + agentFileName), this );
            filesToBackup << QLatin1String("kdeconfig") + sep + agentFileName;
        }
    }

    /* Copy over the Akonadi config files */
    const QString config = XdgBaseDirs::findResourceDir( "config", QLatin1String("akonadi") );
    QDir dir( config );
    const QStringList configlist = dir.entryList( QDir::Files );
    foreach( const QString& item, configlist ) {
        KIO::NetAccess::file_copy( QString(config + sep + item),
                                   QString(tempDir->name() + QLatin1String("akonadiconfig") + sep + item), this );
        filesToBackup << QLatin1String("akonadiconfig/") + item;
    }

    /* Dump the database */
    Tray::Global global;
    KProcess *proc = new KProcess( this );
    QStringList params;

    if( global.dbdriver() == QLatin1String("QMYSQL") ) {
        params << QLatin1String("--single-transaction")
               << QLatin1String("--flush-logs")
               << QLatin1String("--triggers")
               << QLatin1String("--result-file=") + tempDir->name() + QLatin1String("db") + sep + QLatin1String("database.sql")
               << global.dboptions()
               << global.dbname();
    }
    else if ( global.dbdriver() == QLatin1String("QPSQL") ) {
        params << QLatin1String("--format=custom")
               << QLatin1String("--blobs")
               << QLatin1String("--file=") + tempDir->name() + QLatin1String("db") + sep + QLatin1String("database.sql")
               << global.dboptions()
               << global.dbname();
    }

    kDebug() << "Executing: " << m_dbDumpApp << params;
    proc->setProgram( m_dbDumpApp, params );
    int result = proc->execute();
    delete proc;
    if ( result != 0 ) {
        kWarning() << "Executed: " << m_dbDumpApp << params << "Result: " << result;
        tempDir->unlink();
        delete tempDir;
        emit completed( false );
        return;
    }
    filesToBackup << QLatin1String("db") + sep + QLatin1String("database.sql");

    /* Make a nice tar file. */
    proc = new KProcess( this );
    params.clear();
    params << QLatin1String("-C") << tempDir->name();
    params << QLatin1String("-cjf");
    params << filename.toLocalFile() << filesToBackup;
    proc->setWorkingDirectory( tempDir->name() );
    proc->setProgram( KStandardDirs::findExe( QLatin1String("tar") ), params );
    result = proc->execute();
    delete proc;
    if ( result != 0 ) {
        kWarning() << "Executed: " << KStandardDirs::findExe( QLatin1String("tar") ) << params << QLatin1String("Result: ") << result;
        tempDir->unlink();
        delete tempDir;
        emit completed( false );
        return;
    }

    tempDir->unlink();
    delete tempDir;
    emit completed( true );
}

