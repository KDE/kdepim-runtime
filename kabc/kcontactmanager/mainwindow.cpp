/*
    This file is part of KContactManager.

    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <kactioncollection.h>
#include <kstandardaction.h>
#include <ktoolbar.h>

#include "mainwidget.h"

#include "mainwindow.h"

MainWindow::MainWindow()
  : KXmlGuiWindow( 0 )
{
  mMainWidget = new MainWidget( this, this );

  setCentralWidget( mMainWidget );

  initActions();

  setStandardToolBarMenuEnabled( true );

  createGUI( "kcontactmanagerui.rc" );

  toolBar()->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );

  setAutoSaveSettings();
}

MainWindow::~MainWindow()
{
}

void MainWindow::initActions()
{
  KStandardAction::quit( this, SLOT( close() ), actionCollection() );
}
