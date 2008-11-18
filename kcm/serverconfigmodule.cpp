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

#include "serverconfigmodule.h"

#include <kdebug.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>

#include <QSettings>

#include <akonadi/control.h>
#include <akonadi/servermanager.h>
#include <akonadi/private/xdgbasedirs_p.h>

K_PLUGIN_FACTORY( ServerConfigModuleFactory, registerPlugin<ServerConfigModule>(); )
K_EXPORT_PLUGIN( ServerConfigModuleFactory( "kcm_akonadi_server" ) )

using namespace Akonadi;

ServerConfigModule::ServerConfigModule( QWidget * parent, const QVariantList & args  ) :
    KCModule( ServerConfigModuleFactory::componentData(), parent, args )
{
  setButtons( KCModule::Default | KCModule::Apply );
  ui.setupUi( this );
  connect( ui.startServer, SIGNAL(toggled(bool)), SLOT(changed()) );
  connect( ui.serverPath, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui.name, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui.host, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui.username, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui.password, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui.options, SIGNAL(textChanged(QString)), SLOT(changed()) );

  connect( ui.startStopButton, SIGNAL(clicked()), SLOT(startStopClicked()) );
  connect( ui.restartButton, SIGNAL(clicked()), SLOT(restartClicked()) );
  connect( ui.selfTestButton, SIGNAL(clicked()), SLOT(selfTestClicked()) );

  connect( ServerManager::self(), SIGNAL(started()), SLOT(updateStatus()) );
  connect( ServerManager::self(), SIGNAL(stopped()), SLOT(updateStatus()) );
}

void ServerConfigModule::load()
{
  const QString serverConfigFile = XdgBaseDirs::akonadiServerConfigFile( XdgBaseDirs::ReadWrite );
  QSettings settings( serverConfigFile, QSettings::IniFormat );
  settings.beginGroup( "QMYSQL" );
  ui.startServer->setChecked( settings.value( "StartServer", true ).toBool() );
  ui.serverPath->setUrl( KUrl::fromPath( settings.value( "ServerPath", "" ).toString() ) );
  ui.name->setText( settings.value( "Name", "akonadi" ).toString() );
  ui.host->setText( settings.value( "Host", "" ).toString() );
  ui.username->setText( settings.value( "User", "" ).toString() );
  ui.password->setText( settings.value( "Password", "" ).toString() );
  ui.options->setText( settings.value( "Options", "" ).toString() );
  settings.endGroup();

  updateStatus();
}

void ServerConfigModule::save()
{
  kDebug();
  const QString serverConfigFile = XdgBaseDirs::akonadiServerConfigFile( XdgBaseDirs::ReadWrite );
  QSettings settings( serverConfigFile, QSettings::IniFormat );
  settings.beginGroup( "QMYSQL" );
  settings.setValue( "StartServer", ui.startServer->isChecked() );
  settings.setValue( "ServerPath", ui.serverPath->url().toLocalFile() );
  settings.setValue( "Name", ui.name->text() );
  settings.setValue( "Host", ui.host->text() );
  settings.setValue( "User", ui.username->text() );
  settings.setValue( "Password", ui.password->text() );
  settings.setValue( "Options", ui.options->text() );
  settings.endGroup();
  settings.sync();

  // TODO: restart server if needed
}

void ServerConfigModule::defaults()
{
  ui.startServer->setChecked( true );
  // TODO: detect default server path
  ui.name->setText( "akonadi" );
}

void ServerConfigModule::updateStatus()
{
  if ( ServerManager::isRunning() ) {
    ui.statusLabel->setText( i18n( "<b>The Akonadi server is running.</b>" ) );
    ui.startStopButton->setText( i18n( "Stop" ) );
    ui.restartButton->setEnabled( true );
  } else {
    ui.statusLabel->setText( i18n( "The Akonadi server is <b>not</b> running." ) );
    ui.startStopButton->setText( i18n( "Start" ) );
    ui.restartButton->setEnabled( false );
  }
}

void ServerConfigModule::startStopClicked()
{
  if ( ServerManager::isRunning() )
    Control::stop( this );
  else
    Control::start( this );
}

void ServerConfigModule::restartClicked()
{
  Control::restart( this );
}

void ServerConfigModule::selfTestClicked()
{
  ServerManager::showSelfTestDialog( this );
}

#include "serverconfigmodule.moc"
