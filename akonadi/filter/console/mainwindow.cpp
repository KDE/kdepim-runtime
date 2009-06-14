/******************************************************************************
 *
 *  File : mainwindow.cpp
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

#include <akonadi/filter/componentfactory.h>
#include <akonadi/filter/ui/editorfactory.h>

#include <KDebug>
#include <KMessageBox>
#include <KLocale>
#include <KIcon>

#include <QPushButton>
#include <QListWidget>
#include <QLayout>

#include "filter.h"
#include "filtereditor.h"
#include "filteragentinterface.h"
#include "mimetypeselectiondialog.h"

MainWindow * MainWindow::mInstance = 0;

MainWindow::MainWindow()
  : KXmlGuiWindow( 0 )
{
  mInstance = this;

  if( !Akonadi::Control::start( this ) )
  {
    kDebug() << "Could not start akonadi server";
    return;
  }

  mFilterAgent = new OrgFreedesktopAkonadiFilterAgentInterface( QLatin1String( "org.freedesktop.Akonadi.Agent.akonadi_filter_agent" ), QLatin1String( "/" ), QDBusConnection::sessionBus() );

  Akonadi::Filter::ComponentFactory * componentFactory = new Akonadi::Filter::ComponentFactory();
  mComponentFactories.insert( QLatin1String( "message/rfc822" ), componentFactory );

  componentFactory = new Akonadi::Filter::ComponentFactory();
  mComponentFactories.insert( QLatin1String( "message/news" ), componentFactory );

  Akonadi::Filter::UI::EditorFactory * editorFactory = new Akonadi::Filter::UI::EditorFactory();
  mEditorFactories.insert( QLatin1String( "message/rfc822" ), editorFactory );

  editorFactory = new Akonadi::Filter::UI::EditorFactory();
  mEditorFactories.insert( QLatin1String( "message/news" ), editorFactory );


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
  qDeleteAll( mComponentFactories );
  qDeleteAll( mEditorFactories );
  delete mFilterAgent;
}

void MainWindow::listFilters()
{
  QDBusPendingReply< QStringList > r = mFilterAgent->enumerateFilters( QString() );
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
  QDBusPendingReply< QStringList > rMimeTypes = mFilterAgent->enumerateMimeTypes();
  rMimeTypes.waitForFinished();

  if( rMimeTypes.isError() )
  {
    KMessageBox::error( this, rMimeTypes.error().message(), i18n( "Could not enumerate mimetypes" ) );
    return;
  }

  MimeTypeSelectionDialog dlg( this, rMimeTypes.value() );
  if( dlg.exec() != KDialog::Accepted )
    return;

  QString mimeType = dlg.selectedMimeType();

  Akonadi::Filter::ComponentFactory * componentFactory = mComponentFactories.value( mimeType, 0 );
  if( !componentFactory )
  {
    KMessageBox::error( this, i18n( "The specified mimetype has no installed component factory" ), i18n( "Internal error" ) );
    return;
  }

  Akonadi::Filter::UI::EditorFactory * editorFactory = mEditorFactories.value( mimeType, 0 );
  if( !editorFactory )
  {
    KMessageBox::error( this, i18n( "The specified mimetype has no installed editor factory" ), i18n( "Internal error" ) );
    return;
  }

  Filter filter;
  filter.setMimeType( mimeType );
  filter.setComponentFactory( componentFactory );
  filter.setEditorFactory( editorFactory );

  FilterEditor ed( this, &filter );
  if( ed.exec() != KDialog::Accepted )
    return;

  QDBusPendingReply< bool > rCreate = mFilterAgent->createFilter( filter.id(), filter.mimeType(), QLatin1String( "" ) );
  rCreate.waitForFinished();

  if( rCreate.isError() )
  {
    KMessageBox::error( this, rCreate.error().message(), i18n( "Could not crate new filter" ) );
    return;
  }

  listFilters();

  QDBusPendingReply< bool > rAttach = mFilterAgent->attachFilter( QLatin1String( "pippo" ), 3 );
  rAttach.waitForFinished();

  if( rAttach.isError() )
  {
    KMessageBox::error( this, rAttach.error().message(), i18n( "Could not attach filter to collection" ) );
    return;
  }

  // Done!  
}

void MainWindow::slotDeleteFilterButtonClicked()
{
}

