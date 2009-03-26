/*
 * This file is part of the Akonadi Mail example.
 *
 * Copyright 2009  Stephen Kelly <steveire@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mainwindow.h"

#include <akonadi/session.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>

#include "mailwidget.h"
#include "mailcomposer.h"

MainWindow::MainWindow() : KXmlGuiWindow()
{
  mw = new MailWidget( this );
  setCentralWidget( mw );

  KAction *action;
  action = actionCollection()->addAction( "compose_mail");
  action->setText( i18n("New Mail") );
  connect(action, SIGNAL(triggered()), SLOT(compose()));

  setupGUI();
}


MainWindow::~MainWindow()
{
}

void MainWindow::compose()
{
  Akonadi::Session *session = new Akonadi::Session( QByteArray( "MailWindow-" ) + QByteArray::number( qrand() ), this );
  MailComposer *mc = new MailComposer(session, this);
  mc->show();

}
