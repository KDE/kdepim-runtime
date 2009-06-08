/****************************************************************************** * *
 *  File : mainwindow.h
 *  Created on Mon 08 Jun 2009 22:38:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filter Console Application
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "mainwindow.h"

#include <akonadi/control.h>

#include <KDebug>

#include <QSplitter>
#include <QListWidget>
#include <QTabWidget>

#include "filteragentinterface.h"

MainWindow::MainWindow()
  : KXmlGuiWindow( 0 )
{
  if( !Akonadi::Control::start( this ) )
  {
    kDebug() << "Could not start akonadi server";
    return;
  }

  QSplitter * spl = new QSplitter( Qt::Horizontal, this );
  setCentralWidget( spl );

  mFilterListWidget = new QListWidget( spl );
  spl->addWidget( mFilterListWidget );

  mTabWidget = new QTabWidget( spl );
  spl->addWidget( mTabWidget );

  QList< int > sizes;
  sizes.append( 200 );
  sizes.append( 500 );

  spl->setSizes( sizes );

  listFilters();
}

MainWindow::~MainWindow()
{
}

void MainWindow::listFilters()
{
  kDebug() << "Creating FilterAgent interface";

  org::freedesktop::Akonadi::FilterAgent a( QLatin1String( "org.freedesktop.Akonadi.Agent.akonadi_filter_agent" ), QLatin1String( "/" ), QDBusConnection::sessionBus() );

  kDebug() << "calling createEngine";

  QDBusPendingReply<> r = a.createEngine( QLatin1String( "pippo" ) );

  kDebug() << "waiting for completion";

  r.waitForFinished();

  kDebug() << "completion ok";

  if( r.isError() )
  {
    kDebug() << "Could not call createEngine: " << r.error();
  } else {
    kDebug() << "createEngine called";
  }
}

