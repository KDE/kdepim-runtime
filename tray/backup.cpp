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

#include <KDebug>
#include <KProcess>
#include <KStandardDirs>
#include <KTempDir>
#include <KUrl>

#include <kio/netaccess.h>

#include <akonadi/private/xdgbasedirs_p.h>
#include <akonadi/agentmanager.h>


using namespace Akonadi;

/**
 * Use this class to create a backup. possible() will tell you if all
 * apps needed for the backup are available. Don't proceed without them.
 * After that call create() to get it running. Please make sure the parameter
 * has the tgz extension.
 */
Backup::Backup( QWidget *parent ) : QWidget( parent )
{
}

bool Backup::possible()
{
    const QString mysqldump = KStandardDirs::findExe( "mysqldump" );
    const QString bzip2 = KStandardDirs::findExe( "bzip2" );
    const QString tar = KStandardDirs::findExe( "tar" );
    kDebug() << "mysqldump:" << mysqldump << "bzip2:" << bzip2 << "tar:" << tar;
    return !mysqldump.isEmpty() && !bzip2.isEmpty() && !tar.isEmpty();
}

void Backup::create( const KUrl& filename )
{
    if ( filename.isEmpty() ) {
        emit completed( false );
        return;
    }

    /* first create the temp folder. */
    KTempDir *tempDir = new KTempDir( KStandardDirs::locateLocal( "tmp", "akonadi" ) );
    tempDir->setAutoRemove( false );
    KIO::NetAccess::mkdir( tempDir->name() + "config", this );
    KIO::NetAccess::mkdir( tempDir->name() + "db", this );
    kDebug() << "Temp dir: "<< tempDir->name();

    QStringList filesToBackup;

    /* Copy over the configuration files. */
    AgentManager *manager = AgentManager::self();
    AgentInstance::List list = manager->instances();
    foreach( AgentInstance agent, list ) {
        const QString agentFileName = agent.identifier() + "rc";
        const QString configFileName = KStandardDirs::locateLocal( "config", agentFileName );
        bool exists = KIO::NetAccess::exists( configFileName, KIO::NetAccess::DestinationSide, this );
        if ( exists ) {
            KIO::NetAccess::file_copy( configFileName, tempDir->name() + "config/" + agentFileName, this );
            filesToBackup << "config/" + agentFileName;
        }
    }

    /* Dump the database */
    const QString socket = XdgBaseDirs::findResourceDir( "data",
                           "akonadi/db_misc/" ) + "mysql.socket";
    if ( socket.isEmpty() )
        kFatal() << "No socket found";

    KProcess *proc = new KProcess( this );
    QStringList params;
    params << "--single-transaction" << "--master-data=2";
    params << "--flush-logs" << "--triggers";
    params << "--result-file=" + tempDir->name() + "db/database.sql";
    params << "--socket=" + socket << "akonadi";
    proc->setProgram( KStandardDirs::findExe( "mysqldump" ), params );
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
    filesToBackup << "db/database.sql";

    /* Make a nice tar file. */
    proc = new KProcess( this );
    params.clear();
    params << "-C" << tempDir->name();
    params << "-cjvf";
    params << filename.path() << filesToBackup;
    proc->setWorkingDirectory( tempDir->name() );
    proc->setProgram( KStandardDirs::findExe( "tar" ), params );
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

#include "backup.moc"
