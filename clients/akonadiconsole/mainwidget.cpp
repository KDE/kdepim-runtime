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

#include <libakonadi/agentinstanceview.h>
#include <libakonadi/agentfilterproxymodel.h>
#include <libakonadi/standardactionmanager.h>

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
  resView->view()->agentFilterProxyModel()->addCapability( "Resource" );
  tabWidget->addTab( resView, "Resources" );
  BrowserWidget *browser = new BrowserWidget( parent, tabWidget );
  tabWidget->addTab( browser, "Browser" );
  tabWidget->addTab( new DebugWidget( tabWidget ), "Debugger" );

  Akonadi::StandardActionManager *actMgr = new Akonadi::StandardActionManager( parent->actionCollection(), this );
  actMgr->setCollectionSelectionModel( browser->collectionSelectionModel() );
  actMgr->setItemSelectionModel( browser->itemSelectionModel() );
  actMgr->createAllActions();
}
