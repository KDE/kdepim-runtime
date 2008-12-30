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
  mConfig( new KConfig( "akonadi-firstrunrc" ) ),
  mCurrentDefault( 0 )
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
  kDebug();
  const KConfigGroup cfg( mConfig, "ProcessedDefaults" );
  foreach ( const QString &dirName, KGlobal::dirs()->findDirs( "data", "akonadi/firstrun" ) ) {
    const QStringList files = QDir( dirName ).entryList( QDir::Files | QDir::Readable );
    foreach ( const QString &fileName, files ) {
      const QString fullName = dirName + fileName;
      if ( cfg.hasKey( fullName ) )
        continue;
      mPendingDefaults << dirName + fileName;
    }
  }
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

  QDBusInterface *iface = new QDBusInterface( "org.freedesktop.Akonadi.Agent." + instance.identifier(),
                                              "/Settings", QString(), QDBusConnection::sessionBus(), this );
  if ( !iface->isValid() ) {
    kError() << "Unable to obtain the KConfigXT D-Bus interface of " << instance.identifier();
    setupNext();
    return;
  }

  foreach ( const QString &setting, settings.keyList() ) {
    kDebug() << "Setting up " << setting << " for agent " << instance.identifier();
    const QString methodName = "set" + setting;
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
  cfg.writeEntry( mCurrentDefault->name(), true );
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
