/*
    This file is part of libakonadi.

    Copyright (c) 2006 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include <kcmdlineargs.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtDBus/QtDBus>

#include <signal.h>
#include <stdlib.h>

#include "kcrash.h"
#include "resourcebase.h"
#include "resourceadaptor.h"
#include "session.h"

#include "tracerinterface.h"

#include <libakonadi/job.h>
#include <libakonadi/monitor.h>

using namespace Akonadi;

static ResourceBase *sResourceBase = 0;

void crashHandler( int signal )
{
  if ( sResourceBase )
    sResourceBase->crashHandler( signal );

  exit( 255 );
}

class ResourceBase::Private
{
  public:
    Private()
      : mStatusCode( Ready ),
        mProgress( 0 ),
        mSettings( 0 )
    {
      mStatusMessage = defaultReadyMessage();
    }

    QString defaultReadyMessage() const;
    QString defaultSyncingMessage() const;
    QString defaultErrorMessage() const;

    org::kde::Akonadi::Tracer *mTracer;
    QString mId;
    QString mName;

    int mStatusCode;
    QString mStatusMessage;

    uint mProgress;
    QString mProgressMessage;

    QSettings *mSettings;

    Session *session;
    Monitor *monitor;
    QHash<Akonadi::Job*, QDBusMessage> pendingReplys;
};

QString ResourceBase::Private::defaultReadyMessage() const
{
  return tr( "Ready" );
}

QString ResourceBase::Private::defaultSyncingMessage() const
{
  return tr( "Syncing..." );
}

QString ResourceBase::Private::defaultErrorMessage() const
{
  return tr( "Error!" );
}

ResourceBase::ResourceBase( const QString & id )
  : d( new Private )
{
  KCrash::init();
  KCrash::setEmergencyMethod( ::crashHandler );
  sResourceBase = this;

  d->mTracer = new org::kde::Akonadi::Tracer( "org.kde.Akonadi", "/tracing", QDBusConnection::sessionBus(), this );

  if ( !QDBusConnection::sessionBus().registerService( "org.kde.Akonadi.Resource." + id ) )
    error( QString( "Unable to register service at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  new ResourceAdaptor( this );
  if ( !QDBusConnection::sessionBus().registerObject( "/", this, QDBusConnection::ExportAdaptors ) )
    error( QString( "Unable to register object at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  d->mId = id;

  d->mSettings = new QSettings( QString( "%1/.akonadi/resource_config_%2" ).arg( QDir::homePath(), id ), QSettings::IniFormat );

  const QString name = d->mSettings->value( "Resource/Name" ).toString();
  if ( !name.isEmpty() )
    d->mName = name;

  d->session = new Session( d->mId.toLatin1(), this );
  d->monitor = new Monitor( this );
  connect( d->monitor, SIGNAL(itemAdded(Akonadi::DataReference)), SLOT(slotItemAdded(Akonadi::DataReference)) );
  connect( d->monitor, SIGNAL(itemChanged(Akonadi::DataReference)), SLOT(slotItemChanged(Akonadi::DataReference)) );
  connect( d->monitor, SIGNAL(itemRemoved(Akonadi::DataReference)), SLOT(slotItemRemoved(Akonadi::DataReference)) );
  d->monitor->ignoreSession( session() );
  d->monitor->monitorResource( d->mId.toLatin1() );

  // initial configuration
  bool initialized = settings()->value( "Resource/Initialized", false ).toBool();
  if ( !initialized ) {
    QTimer::singleShot( 0, this, SLOT(configure()) ); // finish construction first
    settings()->setValue( "Resource/Initialized", true );
  }
}

ResourceBase::~ResourceBase()
{
  delete d->mSettings;
  delete d;
}

int ResourceBase::status() const
{
  return d->mStatusCode;
}

QString ResourceBase::statusMessage() const
{
  return d->mStatusMessage;
}

uint ResourceBase::progress() const
{
  return d->mProgress;
}

QString ResourceBase::progressMessage() const
{
  return d->mProgressMessage;
}

void ResourceBase::warning( const QString& message )
{
  d->mTracer->warning( QString( "ResourceBase(%1)" ).arg( d->mId ), message );
}

void ResourceBase::error( const QString& message )
{
  d->mTracer->error( QString( "ResourceBase(%1)" ).arg( d->mId ), message );
}

void ResourceBase::changeStatus( Status status, const QString &message )
{
  d->mStatusMessage = message;
  d->mStatusCode = 0;

  switch ( status ) {
    case Ready:
      if ( d->mStatusMessage.isEmpty() )
        d->mStatusMessage = d->defaultReadyMessage();

      d->mStatusCode = 0;
      break;
    case Syncing:
      if ( d->mStatusMessage.isEmpty() )
        d->mStatusMessage = d->defaultSyncingMessage();

      d->mStatusCode = 1;
      break;
    case Error:
      if ( d->mStatusMessage.isEmpty() )
        d->mStatusMessage = d->defaultErrorMessage();

      d->mStatusCode = 2;
      break;
    default:
      Q_ASSERT( !"Unknown status passed" );
      break;
  }

  emit statusChanged( d->mStatusCode, d->mStatusMessage );
}

void ResourceBase::changeProgress( uint progress, const QString &message )
{
  d->mProgress = progress;
  d->mProgressMessage = message;

  emit progressChanged( d->mProgress, d->mProgressMessage );
}

void ResourceBase::configure()
{
}

void ResourceBase::synchronize()
{
}

void ResourceBase::setName( const QString &name )
{
  if ( name == d->mName )
    return;

  // TODO: rename collection
  d->mName = name;

  if ( d->mName.isEmpty() || d->mName == d->mId )
    d->mSettings->remove( "Resource/Name" );
  else
    d->mSettings->setValue( "Resource/Name", d->mName );

  d->mSettings->sync();

  emit nameChanged( d->mName );
}

QString ResourceBase::name() const
{
  if ( d->mName.isEmpty() )
    return d->mId;
  else
    return d->mName;
}

QString ResourceBase::parseArguments( int argc, char **argv )
{
  QString identifier;
  if ( argc && argv ) {
    if ( argc < 3 ) {
      qDebug( "ResourceBase::parseArguments: Not enough arguments passed..." );
      exit( 1 );
    }

    for ( int i = 1; i < argc - 1; ++i ) {
      if ( QString( argv[ i ] ) == "--identifier" )
        identifier = QString( argv[ i + 1 ] );
    }
  } else {
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    if ( args && args->isSet( "identifier" ) )
      identifier = QString::fromLatin1( args->getOption( "identifier" ) );
  }

  if ( identifier.isEmpty() ) {
    qDebug( "ResourceBase::parseArguments: Identifier argument missing" );
    exit( 1 );
  }

  QApplication::setQuitOnLastWindowClosed( false );

  return identifier;
}

void ResourceBase::quit()
{
  aboutToQuit();

  d->mSettings->sync();

  QCoreApplication::exit( 0 );
}

void ResourceBase::aboutToQuit()
{
}

QString ResourceBase::identifier() const
{
  return d->mId;
}

void ResourceBase::cleanup() const
{
  const QString fileName = d->mSettings->fileName();

  /**
   * First destroy the settings object...
   */
  delete d->mSettings;
  d->mSettings = 0;

  /**
   * ... then remove the file from hd.
   */
  QFile::remove( fileName );

  QCoreApplication::exit( 0 );
}

void ResourceBase::crashHandler( int signal )
{
  /**
   * If we retrieved a SIGINT or SIGTERM we close normally
   */
  if ( signal == SIGINT || signal == SIGTERM )
    quit();
}

QSettings* ResourceBase::settings()
{
  return d->mSettings;
}

Session* ResourceBase::session()
{
  return d->session;
}

bool ResourceBase::deliverItem(Akonadi::Job * job, const QDBusMessage & msg)
{
  msg.setDelayedReply( true );
  d->pendingReplys.insert( job, msg.createReply() );
  connect( job, SIGNAL(result(KJob*)), SLOT(slotDeliveryDone(KJob*)) );
  return false;
}

void ResourceBase::slotDeliveryDone(KJob * job)
{
  Q_ASSERT( d->pendingReplys.contains( static_cast<Akonadi::Job*>( job ) ) );
  QDBusMessage reply = d->pendingReplys.take( static_cast<Akonadi::Job*>( job ) );
  if ( job->error() ) {
    error( "Error while creating item: " + job->errorString() );
    reply << false;
  } else {
    reply << true;
  }
  QDBusConnection::sessionBus().send( reply );
}

void ResourceBase::slotItemAdded(const Akonadi::DataReference & ref)
{
  // TODO: offline change tracking
  itemAdded( ref );
}

void ResourceBase::slotItemChanged(const Akonadi::DataReference & ref)
{
  // TODO: offline change tracking
  itemChanged( ref );
}

void ResourceBase::slotItemRemoved(const Akonadi::DataReference & ref)
{
  // TODO: offline change tracking
  itemRemoved( ref );
}

