// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 2; -*-
/**
 * pluginmanager.cpp
 * Most of this code has been lifted from Martijn's KopetePluginManager class
 *
 * Copyright (C)  2004  Zack Rusin <zack@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#include "pluginmanager.h"

#include "plugin.h"

#include <QApplication>
#include <QFile>
#include <QRegExp>
#include <QTimer>
#include <Q3ValueStack>

#include <kapplication.h>
#include <kdebug.h>
#include <kparts/componentfactory.h>
#include <kplugininfo.h>
#include <ksettings/dispatcher.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kurl.h>


namespace Komposer
{

class PluginManager::Private
{
public:
  // All available plugins, regardless of category, and loaded or not
  QValueList<KPluginInfo*> plugins;

  // Dict of all currently loaded plugins, mapping the KPluginInfo to
  // a plugin
  QMap<KPluginInfo*, Plugin*> loadedPlugins;

  // The plugin manager's mode. The mode is StartingUp until loadAllPlugins()
  // has finished loading the plugins, after which it is set to Running.
  // ShuttingDown and DoneShutdown are used during Komposer shutdown by the
  // async unloading of plugins.
  enum ShutdownMode { StartingUp, Running, ShuttingDown, DoneShutdown };
  ShutdownMode shutdownMode;

  KSharedConfig::Ptr config;
  // Plugins pending for loading
  QValueStack<QString> pluginsToLoad;
};

PluginManager::PluginManager( QObject *parent )
  : QObject( parent )
{
  d = new Private;

  // We want to add a reference to the application's event loop so we
  // can remain in control when all windows are removed.
  // This way we can unload plugins asynchronously, which is more
  // robust if they are still doing processing.
  kapp->ref();
  d->shutdownMode = Private::StartingUp;

  KSettings::Dispatcher::self()->registerInstance( KGlobal::mainComponent(),
                                                   this, SLOT( loadAllPlugins() ) );

  d->plugins = KPluginInfo::fromServices(
    KTrader::self()->query( QString::fromLatin1( "Komposer/Plugin" ),
                            QString::fromLatin1( "[X-Komposer-Version] == 1" ) ) );
}

PluginManager::~PluginManager()
{
  if ( d->shutdownMode != Private::DoneShutdown ) {
    slotShutdownTimeout();
#if 0
    kWarning()
                << "Destructing plugin manager without going through"
                << "the shutdown process!"
                << endl
                << kBacktrace(10);
#endif
  }

  // Quick cleanup of the remaining plugins, hope it helps
  QMap<KPluginInfo*, Plugin*>::ConstIterator it;
  for ( it = d->loadedPlugins.begin(); it != d->loadedPlugins.end(); /* EMPTY */ )
  {
    // Remove causes the iterator to become invalid, so pre-increment first
    QMap<KPluginInfo*, Plugin*>::ConstIterator nextIt( it );
    ++nextIt;
    kWarning() <<"Deleting stale plugin '"
                       << it.value()->name() << "'";
    delete it.value();
    it = nextIt;
  }

  delete d;
}

QValueList<KPluginInfo*>
PluginManager::availablePlugins( const QString &category ) const
{
  if ( category.isEmpty() )
    return d->plugins;

  QValueList<KPluginInfo*> result;
  QValueList<KPluginInfo*>::ConstIterator it;
  for ( it = d->plugins.begin(); it != d->plugins.end(); ++it )
  {
    if ( ( *it )->category() == category )
      result.append( *it );
  }

  return result;
}

QMap<KPluginInfo*, Plugin*>
PluginManager::loadedPlugins( const QString &category ) const
{
  QMap<KPluginInfo*, Plugin*> result;
  QMap<KPluginInfo*, Plugin*>::ConstIterator it;
  for ( it = d->loadedPlugins.begin(); it != d->loadedPlugins.end(); ++it )
  {
    if ( category.isEmpty() || it.key()->category() == category )
      result.insert( it.key(), it.value() );
  }

  return result;
}

void
PluginManager::shutdown()
{
  d->shutdownMode = Private::ShuttingDown;

  // Remove any pending plugins to load, we're shutting down now :)
  d->pluginsToLoad.clear();

  // Ask all plugins to unload
  if ( d->loadedPlugins.empty() ) {
    d->shutdownMode = Private::DoneShutdown;
  } else {
    QMap<KPluginInfo*, Plugin*>::ConstIterator it;
    for ( it = d->loadedPlugins.begin(); it != d->loadedPlugins.end(); /* EMPTY */ )
    {
      // Remove causes the iterator to become invalid, so pre-increment first
      QMap<KPluginInfo*, Plugin*>::ConstIterator nextIt( it );
      ++nextIt;
      it.value()->aboutToUnload();
      it = nextIt;
    }
  }

  QTimer::singleShot( 3000, this, SLOT(slotShutdownTimeout()) );
}

void
PluginManager::slotPluginReadyForUnload()
{
  // Using QObject::sender() is on purpose here, because otherwise all
  // plugins would have to pass 'this' as parameter, which makes the API
  // less clean for plugin authors
  Plugin* plugin = dynamic_cast<Plugin*>( const_cast<QObject*>( sender() ) );
  if ( !plugin )
  {
    kWarning() <<"Calling object is not a plugin!";
    return;

  }
  kDebug()<<"manager unloading";
  plugin->deleteLater();
}

void
PluginManager::slotShutdownTimeout()
{
  // When we were already done the timer might still fire.
  // Do nothing in that case.
  if ( d->shutdownMode == Private::DoneShutdown )
    return;

#ifndef NDEBUG
  QStringList remaining;
  for ( QMap<KPluginInfo*, Plugin*>::ConstIterator it = d->loadedPlugins.begin();
        it != d->loadedPlugins.end(); ++it )
    remaining.append( it.key()->pluginName() );

  kWarning() <<"Some plugins didn't shutdown in time!" << endl
              << "Remaining plugins:"
              << remaining.join( QString::fromLatin1( ", " ) ) << endl
              << "Forcing Komposer shutdown now.";
#endif

  slotShutdownDone();
}

void
PluginManager::slotShutdownDone()
{
  d->shutdownMode = Private::DoneShutdown;

  kapp->deref();
}

void
PluginManager::loadAllPlugins()
{
  // FIXME: We need session management here - Martijn

  if ( !d->config )
    d->config = KSharedConfig::openConfig( "komposerrc" );

  KConfigGroup pluginGroup = d->config->group( "Plugins" );
  const QStringList entries = pluginGroup.keyList();

  QStringList::ConstIterator it;
  for ( it = entries.begin(); it != entries.end(); ++it )
  {
    QString key = *it;
    if ( key.endsWith( QString::fromLatin1( "Enabled" ) ) )
    {
      key.setLength( key.length() - 7 );
      bool value = pluginGroup.readEntry( *it, true );
      //kDebug() <<"Set" << key <<" to" << value;

      if ( value )
      {
        if ( !plugin( key ) )
          d->pluginsToLoad.push( key );
      }
      else
      {
        // FIXME: Does this ever happen? As loadAllPlugins is only called on startup
        //        I'd say 'no'. If it does, it should be made async
        //        though. - Martijn
        if ( plugin( key ) )
          unloadPlugin( key );
      }
    }
  }

  // Schedule the plugins to load
  QTimer::singleShot( 0, this, SLOT( slotLoadNextPlugin() ) );
}

void PluginManager::slotLoadNextPlugin()
{
  if ( d->pluginsToLoad.isEmpty() )
  {
    if ( d->shutdownMode == Private::StartingUp )
    {
      d->shutdownMode = Private::Running;
      emit allPluginsLoaded();
    }
    return;
  }

  QString key = d->pluginsToLoad.pop();
  loadPluginInternal( key );

  // Schedule the next run unconditionally to avoid code duplication on the
  // allPluginsLoaded() signal's handling. This has the added benefit that
  // the signal is delayed one event loop, so the accounts are more likely
  // to be instantiated.
  QTimer::singleShot( 0, this, SLOT( slotLoadNextPlugin() ) );
}

Plugin*
PluginManager::loadPlugin( const QString &pluginId,
                           PluginLoadMode mode /* = LoadSync */ )
{
  if ( mode == LoadSync ) {
    return loadPluginInternal( pluginId );
  } else {
    d->pluginsToLoad.push( pluginId );
    QTimer::singleShot( 0, this, SLOT( slotLoadNextPlugin() ) );
    return 0;
  }
}

Plugin*
PluginManager::loadPluginInternal( const QString &pluginId )
{
  KPluginInfo* info = infoForPluginId( pluginId );
  if ( !info ) {
    kWarning() <<"Unable to find a plugin named '"
                << pluginId << "'!";
    return 0;
  }

  if ( d->loadedPlugins.contains( info ) )
    return d->loadedPlugins[ info ];

  int error = 0;
  Plugin *plugin = KParts::ComponentFactory::createInstanceFromQuery<Komposer::Plugin>(
    QString::fromLatin1( "Komposer/Plugin" ),
    QString::fromLatin1( "[X-KDE-PluginInfo-Name]=='%1'" ).arg( pluginId ),
    this, 0, QStringList(), &error );

  if ( plugin ) {
    d->loadedPlugins.insert( info, plugin );
    info->setPluginEnabled( true );

    connect( plugin, SIGNAL(destroyed(QObject*)),
             this, SLOT(slotPluginDestroyed(QObject*)) );
    connect( plugin, SIGNAL(readyForUnload()),
             this, SLOT(slotPluginReadyForUnload()) );

    kDebug() <<"Successfully loaded plugin '"
              << pluginId << "'";

    emit pluginLoaded( plugin );
  } else {
    switch ( error ) {
    case KParts::ComponentFactory::ErrNoServiceFound:
      kDebug() <<"No service implementing the given mimetype"
                << "and fullfilling the given constraint expression can be found.";
      break;

    case KParts::ComponentFactory::ErrServiceProvidesNoLibrary:
      kDebug() <<"the specified service provides no shared library.";
      break;

    case KParts::ComponentFactory::ErrNoLibrary:
      kDebug() <<"the specified library could not be loaded.";
      break;

    case KParts::ComponentFactory::ErrNoFactory:
      kDebug() <<"the library does not export a factory for creating components.";
      break;

    case KParts::ComponentFactory::ErrNoComponent:
      kDebug() <<"the factory does not support creating components"
                << "of the specified type.";
      break;
    }

    kDebug() <<"Loading plugin '" << pluginId
              << "' failed, KLibLoader reported error: '"
              << KLibLoader::self()->lastErrorMessage()
              << "'";
  }

  return plugin;
}

bool
PluginManager::unloadPlugin( const QString &spec )
{
  QMap<KPluginInfo*, Plugin*>::ConstIterator it;
  for ( it = d->loadedPlugins.begin(); it != d->loadedPlugins.end(); ++it )
  {
    if ( it.key()->pluginName() == spec )
    {
      it.value()->aboutToUnload();
      return true;
    }
  }

  return false;
}

void
PluginManager::slotPluginDestroyed( QObject *plugin )
{
  QMap<KPluginInfo*, Plugin*>::Iterator it;
  for ( it = d->loadedPlugins.begin(); it != d->loadedPlugins.end(); ++it )
  {
    if ( it.value() == plugin )
    {
      d->loadedPlugins.erase( it );
      break;
    }
  }

  if ( d->shutdownMode == Private::ShuttingDown && d->loadedPlugins.isEmpty() )
  {
    // Use a timer to make sure any pending deleteLater() calls have
    // been handled first
    QTimer::singleShot( 0, this, SLOT(slotShutdownDone()) );
  }
}

Plugin*
PluginManager::plugin( const QString &pluginId ) const
{
  KPluginInfo *info = infoForPluginId( pluginId );
  if ( !info )
    return 0;

  if ( d->loadedPlugins.contains( info ) )
    return d->loadedPlugins[ info ];
  else
    return 0;
}

QString
PluginManager::pluginName( const Plugin *plugin ) const
{
  QMap<KPluginInfo*, Plugin*>::ConstIterator it;
  for ( it = d->loadedPlugins.begin(); it != d->loadedPlugins.end(); ++it )
  {
    if ( it.value() == plugin )
      return it.key()->name();
  }

  return QString::fromLatin1( "Unknown" );
}

QString
PluginManager::pluginId( const Plugin *plugin ) const
{
  QMap<KPluginInfo*, Plugin*>::ConstIterator it;
  for ( it = d->loadedPlugins.begin(); it != d->loadedPlugins.end(); ++it )
  {
    if ( it.value() == plugin )
      return it.key()->pluginName();
  }

  return QString::fromLatin1( "unknown" );
}

QString
PluginManager::pluginIcon( const Plugin *plugin ) const
{
  QMap<KPluginInfo*, Plugin*>::ConstIterator it;
  for ( it = d->loadedPlugins.begin(); it != d->loadedPlugins.end(); ++it )
  {
    if ( it.value() == plugin )
      return it.key()->icon();
  }

  return QString::fromLatin1( "Unknown" );
}

KPluginInfo*
PluginManager::infoForPluginId( const QString &pluginId ) const
{
  QValueList<KPluginInfo*>::ConstIterator it;
  for ( it = d->plugins.begin(); it != d->plugins.end(); ++it )
  {
    if ( ( *it )->pluginName() == pluginId )
      return *it;
  }

  return 0;
}

bool
PluginManager::setPluginEnabled( const QString &pluginId, bool enabled /* = true */ )
{
  if ( !d->config )
    d->config = KSharedConfig::openConfig( "komposerrc" );

  KConfigGroup pluginGroup = d->config( "Plugins" );

  if ( !infoForPluginId( pluginId ) )
    return false;

  pluginGroup.writeEntry( pluginId + QString::fromLatin1( "Enabled" ), enabled );
  pluginGroup.sync();

  return true;
}

}

#include "pluginmanager.moc"
