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

#include "mainwindow.h"

#include "mainwidget.h"

#include <libakonadi/control.h>
#include <libakonadi/subscriptiondialog.h>

#include <KAction>
#include <KActionCollection>
#include <KLocale>
#include <KStandardAction>
#include <QApplication>

MainWindow::MainWindow( QWidget *parent )
  : KXmlGuiWindow( parent )
{
  Akonadi::Control::start();
  setCentralWidget( new MainWidget( this ) );

  KStandardAction::quit( qApp, SLOT(quit()), actionCollection() );
  KAction* a = new KAction( i18n("Manage Local &Subscriptions..."), this );
  connect( a, SIGNAL(triggered()), SLOT(localSubscriptions()) );
  actionCollection()->addAction( "local_subscription", a );

  setupGUI( Keys /*| ToolBar | StatusBar*/ | Save | Create, "akonadiconsoleui.rc" );
}

void MainWindow::localSubscriptions()
{
  Akonadi::SubscriptionDialog* dlg = new Akonadi::SubscriptionDialog();
  dlg->show();
}
