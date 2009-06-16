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
#include <akonadi/filter/program.h>
#include <akonadi/filter/ui/editorfactory.h>
#include <akonadi/filter/io/sieveencoder.h>
#include <akonadi/filter/io/sievedecoder.h>

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
  g->addWidget( mFilterListWidget, 0, 0, 1, 3 );

  mNewFilterButton = new QPushButton( base );
  mNewFilterButton->setIcon( KIcon( "edit-new" ) );
  mNewFilterButton->setText( i18n( "New Filter") );
  g->addWidget( mNewFilterButton, 1, 0 );

  connect( mNewFilterButton, SIGNAL( clicked() ), this, SLOT( slotNewFilterButtonClicked() ) );

  mEditFilterButton = new QPushButton( base );
  mEditFilterButton->setIcon( KIcon( "edit" ) );
  mEditFilterButton->setText( i18n( "Edit Filter") );
  g->addWidget( mEditFilterButton, 1, 1 );

  connect( mEditFilterButton, SIGNAL( clicked() ), this, SLOT( slotEditFilterButtonClicked() ) );


  mDeleteFilterButton = new QPushButton( base );
  mDeleteFilterButton->setIcon( KIcon( "edit-delete" ) );
  mDeleteFilterButton->setText( i18n( "Delete Filter") );
  g->addWidget( mDeleteFilterButton, 1, 2 );

  connect( mDeleteFilterButton, SIGNAL( clicked() ), this, SLOT( slotDeleteFilterButtonClicked() ) );

  g->setRowStretch( 0, 1 );

  setMinimumSize( 300, 400 );

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

void MainWindow::slotEditFilterButtonClicked()
{
  QString filterId;

  QListWidgetItem * item = mFilterListWidget->currentItem();
  if( !item )
    return;  

  filterId = item->text();

  QDBusPendingReply<bool, QString, QString, QVariantList> rProps = mFilterAgent->getFilterProperties( filterId );
  rProps.waitForFinished();

  if( rProps.isError() )
  {
    KMessageBox::error( this, rProps.error().message(), i18n( "Could not fetch filter properties" ) );
    return;
  }

  QString mimeType = rProps.argumentAt< 1 >();
  QString source = rProps.argumentAt< 2 >();
  QVariantList attachedCollectionIds = rProps.argumentAt< 3 >();

  Akonadi::Filter::ComponentFactory * componentFactory = mComponentFactories.value( mimeType, 0 );
  if( !componentFactory )
  {
    KMessageBox::error( this, i18n( "The filter mimetype has no installed component factory" ), i18n( "Internal error" ) );
    return;
  }

  Akonadi::Filter::UI::EditorFactory * editorFactory = mEditorFactories.value( mimeType, 0 );
  if( !editorFactory )
  {
    KMessageBox::error( this, i18n( "The filter mimetype has no installed editor factory" ), i18n( "Internal error" ) );
    return;
  }

  Akonadi::Filter::IO::SieveDecoder decoder( componentFactory );
  Akonadi::Filter::Program * prog = decoder.run( source );
  if( !prog )
  {
    KMessageBox::error( this, i18n( "Failed to decode the filter from sieve format: %1", decoder.lastError() ), i18n( "Could not edit filter" ) );
    return;    
  }


  Filter filter;

  filter.setId( filterId );
  filter.setMimeType( mimeType );
  filter.setComponentFactory( componentFactory );
  filter.setEditorFactory( editorFactory );
  filter.setProgram( prog );

  FilterEditor ed( this, &filter );
  if( ed.exec() != KDialog::Accepted )
    return;

  Akonadi::Filter::IO::SieveEncoder encoder;
  source = encoder.run( filter.program() );

  if( source.isEmpty() )
  {
    KMessageBox::error( this, i18n( "Failed to encode the filter to sieve format: %1", encoder.lastError() ), i18n( "Could not edit filter" ) );
    return;    
  }

  qDebug( "FILTER SOURCE:" );
  qDebug( "%s", source.toUtf8().data() );
  qDebug( "END OF FILTER SOURCE:" );

  QDBusPendingReply< bool > rDelete = mFilterAgent->deleteFilter( filterId );
  rDelete.waitForFinished();

  if( rDelete.isError() )
  {
    KMessageBox::error( this, rDelete.error().message(), i18n( "Could not delete the old filter" ) );
    return;
  }

  QDBusPendingReply< bool > rCreate = mFilterAgent->createFilter( filter.id(), filter.mimeType(), source );
  rCreate.waitForFinished();

  if( rCreate.isError() )
  {
    KMessageBox::error( this, rCreate.error().message(), i18n( "Could not crate new filter" ) );
    return;
  }

  listFilters();
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
  filter.setProgram( new Akonadi::Filter::Program() );

  FilterEditor ed( this, &filter );
  if( ed.exec() != KDialog::Accepted )
    return;

  Akonadi::Filter::IO::SieveEncoder encoder;
  QString source = encoder.run( filter.program() );

  if( source.isEmpty() )
  {
    KMessageBox::error( this, i18n( "Failed to encode the filter to sieve format: %1", encoder.lastError() ), i18n( "Could not crate new filter" ) );
    return;    
  }

  qDebug( "FILTER SOURCE:" );
  qDebug( "%s", source.toUtf8().data() );
  qDebug( "END OF FILTER SOURCE:" );

  QDBusPendingReply< bool > rCreate = mFilterAgent->createFilter( filter.id(), filter.mimeType(), source );
  rCreate.waitForFinished();

  if( rCreate.isError() )
  {
    KMessageBox::error( this, rCreate.error().message(), i18n( "Could not crate new filter" ) );
    return;
  }

  listFilters();

  // Done!  
}

void MainWindow::slotDeleteFilterButtonClicked()
{
}

