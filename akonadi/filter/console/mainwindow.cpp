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
#include <KMessageBox>
#include <KLocale>
#include <KIcon>

#include <QPushButton>
#include <QListWidget>
#include <QLayout>

#include "filteragentinterface.h"

MainWindow::MainWindow()
  : KXmlGuiWindow( 0 )
{
  if( !Akonadi::Control::start( this ) )
  {
    kDebug() << "Could not start akonadi server";
    return;
  }

  QWidget * base = new QWidget( this );
  setCentralWidget( base );

  QGridLayout * g = new QGridLayout( base );
  g->setMargin( 2 );

  mFilterListWidget = new QListWidget( base );
  g->addWidget( mFilterListWidget, 0, 0, 1, 2 );

  mNewFilterButton = new QPushButton( base );
  mNewFilterButton->setIcon( KIcon( "edit-new" ) );
  mNewFilterButton->setText( i18n( "New Filter") );
  g->addWidget( mNewFilterButton, 1, 0 );

  connect( mNewFilterButton, SIGNAL( clicked() ), this, SLOT( slotNewFilterButtonClicked() ) );

  mDeleteFilterButton = new QPushButton( base );
  mDeleteFilterButton->setIcon( KIcon( "edit-delete" ) );
  mDeleteFilterButton->setText( i18n( "Delete Filter") );
  g->addWidget( mDeleteFilterButton, 1, 1 );

  connect( mDeleteFilterButton, SIGNAL( clicked() ), this, SLOT( slotDeleteFilterButtonClicked() ) );

  g->setRowStretch( 0, 1 );

  listFilters();
}

MainWindow::~MainWindow()
{
}

void MainWindow::listFilters()
{
  org::freedesktop::Akonadi::FilterAgent a( QLatin1String( "org.freedesktop.Akonadi.Agent.akonadi_filter_agent" ), QLatin1String( "/" ), QDBusConnection::sessionBus() );

  QDBusPendingReply< QStringList > r = a.enumerateFilters( QLatin1String( "message/rfc822" ) );
  r.waitForFinished();

  mFilterListWidget->clear();

  if( r.isError() )
  {
    KMessageBox::error( this, r.error().message(), i18n( "Could not enumerate filters" ) );
    return;
  }

  mFilterListWidget->addItems( r.value() );
}

void MainWindow::slotNewFilterButtonClicked()
{
  org::freedesktop::Akonadi::FilterAgent a( QLatin1String( "org.freedesktop.Akonadi.Agent.akonadi_filter_agent" ), QLatin1String( "/" ), QDBusConnection::sessionBus() );

  QDBusPendingReply< bool > r = a.createFilter( QLatin1String( "pippo" ), QLatin1String( "message/rfc822" ), QLatin1String( "" ) );
  r.waitForFinished();

  if( r.isError() )
  {
    KMessageBox::error( this, r.error().message(), i18n( "Could not crate new filter" ) );
    return;
  }

  listFilters();
}

void MainWindow::slotDeleteFilterButtonClicked()
{
}

