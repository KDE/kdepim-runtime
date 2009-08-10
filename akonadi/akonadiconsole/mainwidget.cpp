/*
    This file is part of Akonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "mainwidget.h"

#include "agentwidget.h"
#include "browserwidget.h"
#include "dbbrowser.h"
#include "dbconsole.h"
#include "debugwidget.h"
#include "rawsocketconsole.h"
#include "searchdialog.h"
#include "searchwidget.h"
#include "jobtrackerwidget.h"
#include "notificationmonitor.h"

#include <akonadi/agentinstancewidget.h>
#include <akonadi/agentfilterproxymodel.h>
#include <akonadi/control.h>
#include <akonadi/searchcreatejob.h>

#include <KAction>
#include <KActionCollection>
#include <KCMultiDialog>
#include <KXmlGuiWindow>

#include <QtGui/QTabWidget>
#include <QtGui/QVBoxLayout>

MainWidget::MainWidget( KXmlGuiWindow *parent )
  : QWidget( parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  QTabWidget *tabWidget = new QTabWidget( this );
  tabWidget->setObjectName( "mainTab" );
  layout->addWidget( tabWidget );

  tabWidget->addTab( new AgentWidget( tabWidget ), "Agents" );
  BrowserWidget *browser = new BrowserWidget( parent, tabWidget );
  tabWidget->addTab( browser, "Browser" );
  tabWidget->addTab( new DebugWidget( tabWidget ), "Debugger" );
  tabWidget->addTab( new RawSocketConsole( tabWidget ), "Raw Socket" );
  tabWidget->addTab( new DbBrowser( tabWidget ), "DB Browser" );
  tabWidget->addTab( new DbConsole( tabWidget ), "DB Console" );
  tabWidget->addTab( new JobTrackerWidget( "jobtracker", tabWidget ), "Job Tracker" );
  tabWidget->addTab( new JobTrackerWidget( "resourcesJobtracker", tabWidget ), "Resources Schedulers" );
  tabWidget->addTab( new NotificationMonitor( tabWidget ), "Notification Monitor" );
  tabWidget->addTab( new SearchWidget( tabWidget ), "Item Search" );

  KAction *action = parent->actionCollection()->addAction( "akonadiconsole_search" );
  action->setText( "Create Search..." );
  connect( action, SIGNAL( triggered() ), this, SLOT( createSearch() ) );

  action = parent->actionCollection()->addAction( "akonadiconsole_akonadi2xml" );
  action->setText( "Dump to XML..." );
  connect( action, SIGNAL(triggered()), browser, SLOT(dumpToXml()) );

  action = parent->actionCollection()->addAction( "akonadiserver_start" );
  action->setText( "Start Server" );
  connect( action, SIGNAL(triggered()), SLOT(startServer()) );

  action = parent->actionCollection()->addAction( "akonadiserver_stop" );
  action->setText( "Stop Server" );
  connect( action, SIGNAL(triggered()), SLOT(stopServer()) );

  action = parent->actionCollection()->addAction( "akonadiserver_restart" );
  action->setText( "Restart Server" );
  connect( action, SIGNAL(triggered()), SLOT(restartServer()) );

  action = parent->actionCollection()->addAction( "akonadiserver_configure" );
  action->setText( "Configure Server..." );
  action->setIcon( KIcon("configure") );
  connect( action, SIGNAL(triggered()), SLOT(configureServer()) );
}

void MainWidget::createSearch()
{
  SearchDialog dlg;
  if ( !dlg.exec() )
    return;

  const QString query = dlg.searchQuery();
  if ( query.isEmpty() )
    return;

  QString name = dlg.searchName();
  if ( name.isEmpty() )
    name = "My Search";

  new Akonadi::SearchCreateJob( name, query );
}

void MainWidget::startServer()
{
  Akonadi::Control::start( this );
}

void MainWidget::stopServer()
{
  Akonadi::Control::stop( this );
}

void MainWidget::restartServer()
{
  Akonadi::Control::restart( this );
}

void MainWidget::configureServer()
{
  KCMultiDialog dlg;
  dlg.addModule( "kcm_akonadi_server" );
  dlg.exec();
}


#include "mainwidget.moc"
