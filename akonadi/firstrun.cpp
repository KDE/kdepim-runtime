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

#include "firstrun.h"

#include <akonadi/agenttype.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KProcess>
#include <KStandardDirs>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QMetaMethod>
#include <QMetaObject>

using namespace Akonadi;

Firstrun::Firstrun( QObject *parent ) :
  QObject( parent ),
  mConfig( new KConfig( QLatin1String("akonadi-firstrunrc") ) ),
  mCurrentDefault( 0 ),
  mProcess( 0 )
{
  findPendingDefaults();
  kDebug() << mPendingDefaults;
  setupNext();
}

Firstrun::~Firstrun()
{
  delete mConfig;
  kDebug() << "done";
}

void Firstrun::findPendingDefaults()
{
  const KConfigGroup cfg( mConfig, "ProcessedDefaults" );
  foreach ( const QString &dirName, KGlobal::dirs()->findDirs( "data", QLatin1String("akonadi/firstrun") ) ) {
    const QStringList files = QDir( dirName ).entryList( QDir::Files | QDir::Readable );
    foreach ( const QString &fileName, files ) {
      const QString fullName = dirName + fileName;
      KConfig c( fullName );
      const QString id = KConfigGroup( &c, "Agent" ).readEntry( "Id", QString() );
      if ( id.isEmpty() ) {
        kWarning() << "Found invalid default configuration in " << fullName;
        continue;
      }
      if ( cfg.hasKey( id ) )
        continue;
      mPendingDefaults << dirName + fileName;
    }
  }
}

static QString resourceTypeForMimetype( const QStringList &mimeTypes )
{
  if ( mimeTypes.contains( QLatin1String("text/directory") ) )
    return QString::fromLatin1( "contact" );
  if ( mimeTypes.contains( QLatin1String("text/calendar") ) )
    return QString::fromLatin1( "calendar" );
  // TODO notes
  return QString();
}

void Firstrun::migrateKresType( const QString& resourceFamily )
{
  mResourceFamily = resourceFamily;
  KConfig config( QLatin1String("kres-migratorrc") );
  KConfigGroup migrationCfg( &config, "Migration" );
  const bool enabled = migrationCfg.readEntry( "Enabled", false );
  const int currentVersion = migrationCfg.readEntry( QString::fromLatin1("Version-%1").arg( resourceFamily ), 0 );
  const int targetVersion = migrationCfg.readEntry( "TargetVersion", 0 );
  if ( enabled && currentVersion < targetVersion ) {
    kDebug() << "Performing migration of legacy KResource settings. Good luck!";
    mProcess = new KProcess( this );
    connect( mProcess, SIGNAL(finished(int)), SLOT(migrationFinished(int)) );
    mProcess->setProgram( QLatin1String("kres-migrator"),
                          QStringList() << QLatin1String("--interactive-on-change")
                                        << QLatin1String("--type") << resourceFamily );
    mProcess->start();
    if ( !mProcess->waitForStarted() )
      migrationFinished( -1 );
  } else {
    // nothing to do
    setupNext();
  }
}

void Firstrun::migrationFinished(int exitCode)
{
  Q_ASSERT( mProcess );
  if ( exitCode == 0 ) {
    kDebug() << "KResource -> Akonadi migration has been successful";
    KConfig config( QLatin1String("kres-migratorrc") );
    KConfigGroup migrationCfg( &config, "Migration" );
    const int targetVersion = migrationCfg.readEntry( "TargetVersion", 0 );
    migrationCfg.writeEntry( QString::fromLatin1("Version-%1").arg( mResourceFamily ), targetVersion );
    migrationCfg.sync();
  } else if ( exitCode != 1 ) {
    // exit code 1 means it is already running, so we are probably called by a migrator instance
    kError() << "KResource -> Akonadi migration failed!";
    kError() << "command was: " << mProcess->program();
    kError() << "exit code: " << mProcess->exitCode();
    kError() << "stdout: " << mProcess->readAllStandardOutput();
    kError() << "stderr: " << mProcess->readAllStandardError();
  }

  setupNext();
}


void Firstrun::setupNext()
{
  delete mCurrentDefault;
  mCurrentDefault = 0;

  if ( mPendingDefaults.isEmpty() ) {
    deleteLater();
    return;
  }

  mCurrentDefault = new KConfig( mPendingDefaults.takeFirst() );
  const KConfigGroup agentCfg = KConfigGroup( mCurrentDefault, "Agent" );

  AgentType type = AgentManager::self()->type( agentCfg.readEntry( "Type", QString() ) );
  if ( !type.isValid() ) {
    kError() << "Unable to obtain agent type for default resource agent configuration " << mCurrentDefault->name();
    setupNext();
    return;
  }

  // KDE5: remove me
  // check if there is a kresource setup for this type already
  const QString kresType = resourceTypeForMimetype( type.mimeTypes() );
  if ( !kresType.isEmpty() ) {
    const QString kresCfgFile = KStandardDirs::locateLocal( "config", QString::fromLatin1( "kresources/%1/stdrc" ).arg( kresType ) );
    KConfig resCfg( kresCfgFile );
    const KConfigGroup resGroup( &resCfg, "General" );
    if ( !resGroup.readEntry( "ResourceKeys", QStringList() ).isEmpty()
      || !resGroup.readEntry( "PassiveResourceKeys", QStringList() ).isEmpty() )
    {
      kDebug() << "ignoring " << mCurrentDefault->name() << " as there is a KResource setup for its type already.";
      KConfigGroup cfg( mConfig, "ProcessedDefaults" );
      cfg.writeEntry( agentCfg.readEntry( "Id", QString() ), QString::fromLatin1( "kres" ) );
      cfg.sync();
      migrateKresType( kresType );
      return;
    }
  }

   AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type );
   connect( job, SIGNAL(result(KJob*)), SLOT(instanceCreated(KJob*)) );
   job->start();
}

void Firstrun::instanceCreated( KJob *job )
{
  Q_ASSERT( mCurrentDefault );

  if ( job->error() ) {
    kError() << "Creating agent instance failed for " << mCurrentDefault->name();
    setupNext();
    return;
  }

  AgentInstance instance = static_cast<AgentInstanceCreateJob*>( job )->instance();
  const KConfigGroup agentCfg = KConfigGroup( mCurrentDefault, "Agent" );
  const QString agentName = agentCfg.readEntry( "Name", QString() );
  if ( !agentName.isEmpty() )
    instance.setName( agentName );

  // agent specific settings, using the D-Bus <-> KConfigXT bridge
  const KConfigGroup settings = KConfigGroup( mCurrentDefault, "Settings" );

  QDBusInterface *iface = new QDBusInterface( QString::fromLatin1("org.freedesktop.Akonadi.Agent.%1").arg( instance.identifier() ),
                                              QLatin1String("/Settings"), QString(), QDBusConnection::sessionBus(), this );
  if ( !iface->isValid() ) {
    kError() << "Unable to obtain the KConfigXT D-Bus interface of " << instance.identifier();
    setupNext();
    return;
  }

  foreach ( const QString &setting, settings.keyList() ) {
    kDebug() << "Setting up " << setting << " for agent " << instance.identifier();
    const QString methodName = QString::fromLatin1("set%1").arg( setting );
    const QVariant::Type argType = argumentType( iface->metaObject(), methodName );
    if ( argType == QVariant::Invalid ) {
      kError() << "Setting " << setting << " not found in agent configuration interface of " << instance.identifier();
      continue;
    }
    const QVariant arg = settings.readEntry( setting, QVariant( argType ) );
    const QDBusReply<void> reply = iface->call( methodName, arg );
    if ( !reply.isValid() )
      kError() << "Setting " << setting << " failed for agent " << instance.identifier();
  }

  instance.reconfigure();
  delete iface;

  // remember we set this one up already
  KConfigGroup cfg( mConfig, "ProcessedDefaults" );
  cfg.writeEntry( agentCfg.readEntry( "Id", QString() ), instance.identifier() );
  cfg.sync();

  setupNext();
}

QVariant::Type Firstrun::argumentType( const QMetaObject *mo, const QString &method )
{
  QMetaMethod m;
  for ( int i = 0; i < mo->methodCount(); ++i ) {
    const QString signature = QString::fromLatin1( mo->method(i).signature() );
    if ( signature.startsWith( method ) )
      m = mo->method( i );
  }

  if ( !m.signature() )
    return QVariant::Invalid;

  const QList<QByteArray> argTypes = m.parameterTypes();
  if ( argTypes.count() != 1 )
    return QVariant::Invalid;

  return QVariant::nameToType( argTypes.first() );
}

#include "firstrun.moc"
