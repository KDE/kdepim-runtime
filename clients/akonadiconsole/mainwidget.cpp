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
#include "debugwidget.h"
#include "searchdialog.h"

#include <akonadi/agentinstancewidget.h>
#include <akonadi/agentfilterproxymodel.h>
#include <akonadi/standardactionmanager.h>
#include <akonadi/searchcreatejob.h>

#include <KAction>
#include <KActionCollection>
#include <KXmlGuiWindow>

#include <QtGui/QTabWidget>
#include <QtGui/QVBoxLayout>

MainWidget::MainWidget( KXmlGuiWindow *parent )
  : QWidget( parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  QTabWidget *tabWidget = new QTabWidget( this );
  layout->addWidget( tabWidget );

  tabWidget->addTab( new AgentWidget( tabWidget ), "Agents" );
  AgentWidget *resView = new AgentWidget( tabWidget );
  resView->widget()->agentFilterProxyModel()->addCapabilityFilter( "Resource" );
  tabWidget->addTab( resView, "Resources" );
  BrowserWidget *browser = new BrowserWidget( parent, tabWidget );
  tabWidget->addTab( browser, "Browser" );
  tabWidget->addTab( new DebugWidget( tabWidget ), "Debugger" );

  Akonadi::StandardActionManager *actMgr = new Akonadi::StandardActionManager( parent->actionCollection(), this );
  actMgr->setCollectionSelectionModel( browser->collectionSelectionModel() );
  actMgr->setItemSelectionModel( browser->itemSelectionModel() );
  actMgr->createAllActions();

  KAction *action = parent->actionCollection()->addAction( "akonadiconsole_search" );
  action->setText( "Create Search" );
  connect( action, SIGNAL( triggered() ), this, SLOT( createSearch() ) );
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

#include "mainwidget.moc"
