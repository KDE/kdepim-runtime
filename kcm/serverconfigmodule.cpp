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
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>

#include <QSettings>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <akonadi/control.h>
#include <akonadi/servermanager.h>
#include <akonadi/private/xdgbasedirs_p.h>

K_PLUGIN_FACTORY( ServerConfigModuleFactory, registerPlugin<ServerConfigModule>(); )
K_EXPORT_PLUGIN( ServerConfigModuleFactory( "kcm_akonadi_server" ) )

using namespace Akonadi;

ServerConfigModule::ServerConfigModule( QWidget * parent, const QVariantList & args  ) :
    KCModule( ServerConfigModuleFactory::componentData(), parent, args )
{
  KGlobal::locale()->insertCatalog( QLatin1String("kcm_akonadi") );
  KGlobal::locale()->insertCatalog( QLatin1String("libakonadi") );

  QVBoxLayout *layout = new QVBoxLayout( this );
  setLayout( layout );

  QWidget *storage_driver = new QWidget( this );
  layout->addWidget( storage_driver );
  ui_driver.setupUi( storage_driver );

  m_stackWidget = new QStackedWidget( this );
  layout->addWidget( m_stackWidget );

  // supported drivers
  ui_driver.driverBox->addItem( QLatin1String("Mysql"), QVariant( QLatin1String("QMYSQL") ) );
  ui_driver.driverBox->addItem( QLatin1String("PostgreSQL"), QVariant( QLatin1String("QPSQL") ) );
  ui_driver.driverBox->addItem( QLatin1String("SQLite"), QVariant( QLatin1String("QSQLITE3") ) );

  setButtons( KCModule::Default | KCModule::Apply );

  // MySQL
  m_mysqlWidget = new QWidget( this );
  m_stackWidget->addWidget( m_mysqlWidget );
  ui_mysql.setupUi( m_mysqlWidget );

  // PSQL
  m_psqlWidget = new QWidget( this );
  m_stackWidget->addWidget( m_psqlWidget );
  ui_psql.setupUi( m_psqlWidget );

  // SQLite
  m_sqliteWidget = new QWidget( this );
  m_stackWidget->addWidget( m_sqliteWidget );
  // ui_sqlite.setupUi( m_sqliteWidget );

  m_stackWidget->setCurrentWidget( m_mysqlWidget );

  QWidget *widget = new QWidget( this );
  layout->addWidget( widget );
  ui.setupUi( widget );

  connect( ui_mysql.startServer, SIGNAL(toggled(bool)), SLOT(changed()) );
  connect( ui_mysql.serverPath, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui_mysql.name, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui_mysql.host, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui_mysql.username, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui_mysql.password, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui_mysql.options, SIGNAL(textChanged(QString)), SLOT(changed()) );

  connect( ui_psql.startServer, SIGNAL(toggled(bool)), SLOT(changed()) );
  connect( ui_psql.name, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui_psql.host, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui_psql.username, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui_psql.password, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui_psql.port, SIGNAL(textChanged(QString)), SLOT(changed()) );
  connect( ui_psql.startServer, SIGNAL(toggled(bool)), ui_psql.messagewidget, SLOT(setHidden(bool)));

  connect( ui.startStopButton, SIGNAL(clicked()), SLOT(startStopClicked()) );
  connect( ui.restartButton, SIGNAL(clicked()), SLOT(restartClicked()) );
  connect( ui.selfTestButton, SIGNAL(clicked()), SLOT(selfTestClicked()) );

  connect( ServerManager::self(), SIGNAL(started()), SLOT(updateStatus()) );
  connect( ServerManager::self(), SIGNAL(stopped()), SLOT(updateStatus()) );

  connect( ui_driver.driverBox, SIGNAL(currentIndexChanged(int)), SLOT(driverChanged(int)) );
  connect( ui_driver.driverBox, SIGNAL(currentIndexChanged(int)), SLOT(changed()) );
}

void ServerConfigModule::load()
{
  const QString serverConfigFile = XdgBaseDirs::akonadiServerConfigFile( XdgBaseDirs::ReadWrite );
  QSettings settings( serverConfigFile, QSettings::IniFormat );
  settings.beginGroup( QLatin1String("QMYSQL") );
  ui_mysql.startServer->setChecked( settings.value( QLatin1String("StartServer"), true ).toBool() );
  ui_mysql.serverPath->setUrl( KUrl::fromPath( settings.value( QLatin1String("ServerPath"), QString() ).toString() ) );
  ui_mysql.name->setText( settings.value( QLatin1String("Name"), QLatin1String("akonadi") ).toString() );
  ui_mysql.host->setText( settings.value( QLatin1String("Host"), QString() ).toString() );
  ui_mysql.username->setText( settings.value( QLatin1String("User"), QString() ).toString() );
  ui_mysql.password->setText( settings.value( QLatin1String("Password"), QString() ).toString() );
  ui_mysql.options->setText( settings.value( QLatin1String("Options"), QString() ).toString() );
  settings.endGroup();

  // postgresql group
  settings.beginGroup( QLatin1String("QPSQL") );
  ui_psql.startServer->setChecked( settings.value( QLatin1String("StartServer"), true ).toBool() );
  ui_psql.name->setText( settings.value( QLatin1String("Name"), QLatin1String("akonadi") ).toString() );
  ui_psql.host->setText( settings.value( QLatin1String("Host"), QString() ).toString() );
  ui_psql.username->setText( settings.value( QLatin1String("User"), QString() ).toString() );
  ui_psql.password->setText( settings.value( QLatin1String("Password"), QString() ).toString() );
  ui_psql.port->setText( settings.value( QLatin1String("Port"), QLatin1String("5432") ).toString() );
  ui_psql.messagewidget->setVisible( !ui_psql.startServer->isChecked() );
  ui_psql.messagewidget->setCloseButtonVisible( false );
  ui_psql.messagewidget->setWordWrap( true );
  ui_psql.messagewidget->setMessageType( KMessageWidget::Information );
  ui_psql.messagewidget->setText( i18nc( "@info: special setting to configure",
                                  "Make sure you have %1 in your server postgres.conf "
                                  "file before starting Akonadi.", QLatin1String( "<b>standard_conforming_strings = on</b>" ) ) );
  settings.endGroup();

  // selected driver
  settings.beginGroup( QLatin1String("GENERAL") );
  ui_driver.driverBox->setCurrentIndex( ui_driver.driverBox->findData( settings.value ( QLatin1String("Driver"), QLatin1String("QMYSQL") ) ) );
  driverChanged( ui_driver.driverBox->currentIndex() );
  settings.endGroup();

  updateStatus();
}

void ServerConfigModule::save()
{
  kDebug();
  const QString serverConfigFile = XdgBaseDirs::akonadiServerConfigFile( XdgBaseDirs::ReadWrite );
  QSettings settings( serverConfigFile, QSettings::IniFormat );
  settings.beginGroup( QLatin1String("QMYSQL") );
  settings.setValue( QLatin1String("StartServer"), ui_mysql.startServer->isChecked() );
  settings.setValue( QLatin1String("ServerPath"), ui_mysql.serverPath->url().toLocalFile() );
  settings.setValue( QLatin1String("Name"), ui_mysql.name->text() );
  settings.setValue( QLatin1String("Host"), ui_mysql.host->text() );
  settings.setValue( QLatin1String("User"), ui_mysql.username->text() );
  settings.setValue( QLatin1String("Password"), ui_mysql.password->text() );
  settings.setValue( QLatin1String("Options"), ui_mysql.options->text() );
  settings.endGroup();

  // postgresql group
  settings.beginGroup( QLatin1String("QPSQL") );
  settings.setValue( QLatin1String("StartServer"), ui_psql.startServer->isChecked() );
  settings.setValue( QLatin1String("Name"), ui_psql.name->text() );
  settings.setValue( QLatin1String("Host"), ui_psql.host->text() );
  settings.setValue( QLatin1String("User"), ui_psql.username->text() );
  settings.setValue( QLatin1String("Password"), ui_psql.password->text() );
  settings.setValue( QLatin1String("Port"), ui_psql.port->text() );
  settings.endGroup();

  // sqlite group
  settings.beginGroup( QLatin1String("SQLITE") );
  // TODO: make it configurable
  settings.setValue( QLatin1String("Name"), QLatin1String("akonadi") );
  settings.endGroup();

  // selected driver
  settings.beginGroup( QLatin1String("GENERAL") );
  settings.setValue( QLatin1String("Driver"), ui_driver.driverBox->itemData( ui_driver.driverBox->currentIndex() ).toString() );
  settings.endGroup();
  settings.sync();

  // TODO: restart server if needed
}

void ServerConfigModule::defaults()
{
  ui_mysql.startServer->setChecked( true );
  // TODO: detect default server path
  ui_mysql.name->setText( QLatin1String("akonadi") );

  ui_driver.driverBox->setCurrentIndex( ui_driver.driverBox->findData( QLatin1String("QMYSQL") ) );
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

void ServerConfigModule::driverChanged( int index )
{
  if ( ui_driver.driverBox->itemData( index ).toString() == QLatin1String("QMYSQL") ) {
    m_stackWidget->setCurrentWidget( m_mysqlWidget );
  } else if ( ui_driver.driverBox->itemData( index ).toString() == QLatin1String("QPSQL") ) {
    m_stackWidget->setCurrentWidget( m_psqlWidget );
  } else {
    m_stackWidget->setCurrentWidget( m_sqliteWidget );
  }
}

