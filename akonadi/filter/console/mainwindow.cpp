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
#include <akonadi/collectionfetchjob.h>
#include <akonadi/servermanager.h>

#include <akonadi/filter/componentfactoryrfc822.h>
#include <akonadi/filter/program.h>
#include <akonadi/filter/agent.h>
#include <akonadi/filter/ui/editorfactoryrfc822.h>
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
#include "filterlistwidget.h"
#include "mimetypeselectiondialog.h"

MainWindow * MainWindow::mInstance = 0;

MainWindow::MainWindow()
  : KXmlGuiWindow( 0 )
{
  mInstance = this;

  if( !Akonadi::Control::start( this ) )
  {
    kDebug() << "Failed to start the akonadi server";
  }

  Akonadi::Filter::Agent::registerMetaTypes();

  mFilterAgent = new OrgFreedesktopAkonadiFilterAgentInterface( QLatin1String( "org.freedesktop.Akonadi.Agent.akonadi_filter_agent" ), QLatin1String( "/" ), QDBusConnection::sessionBus() );

  mComponentFactories.insert( QLatin1String( "message/rfc822" ), new Akonadi::Filter::ComponentFactoryRfc822() );
  mComponentFactories.insert( QLatin1String( "message/news" ), new Akonadi::Filter::ComponentFactoryRfc822() );

  mEditorFactories.insert( QLatin1String( "message/rfc822" ), new Akonadi::Filter::UI::EditorFactoryRfc822() );
  mEditorFactories.insert( QLatin1String( "message/news" ), new Akonadi::Filter::UI::EditorFactoryRfc822() );

  QWidget * base = new QWidget( this );
  setCentralWidget( base );

  QGridLayout * g = new QGridLayout( base );
  g->setMargin( 2 );

  mFilterListWidget = new FilterListWidget( base );
  g->addWidget( mFilterListWidget, 0, 0, 11, 1 );

  connect( mFilterListWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( slotFilterListWidgetSelectionChanged() ) );

  mNewFilterButtonLBB = new QPushButton( base );
  mNewFilterButtonLBB->setIcon( KIcon( "document-new" ) );
  mNewFilterButtonLBB->setText( i18n( "New Filter") );
  g->addWidget( mNewFilterButtonLBB, 0, 1 );

  connect( mNewFilterButtonLBB, SIGNAL( clicked() ), this, SLOT( slotNewFilterLBBButtonClicked() ) );

  mNewFilterButtonTBB = new QPushButton( base );
  mNewFilterButtonTBB->setIcon( KIcon( "document-new" ) );
  mNewFilterButtonTBB->setText( i18n( "New Filter (Alt. UI)") );
  g->addWidget( mNewFilterButtonTBB, 1, 1 );

  connect( mNewFilterButtonTBB, SIGNAL( clicked() ), this, SLOT( slotNewFilterTBBButtonClicked() ) );

  g->addItem( new QSpacerItem( 10, 10 ), 2, 1 );


  mEditFilterButtonLBB = new QPushButton( base );
  mEditFilterButtonLBB->setIcon( KIcon( "document-edit" ) );
  mEditFilterButtonLBB->setText( i18n( "Edit Filter") );
  g->addWidget( mEditFilterButtonLBB, 3, 1 );

  connect( mEditFilterButtonLBB, SIGNAL( clicked() ), this, SLOT( slotEditFilterLBBButtonClicked() ) );


  mEditFilterButtonTBB = new QPushButton( base );
  mEditFilterButtonTBB->setIcon( KIcon( "document-edit" ) );
  mEditFilterButtonTBB->setText( i18n( "Edit Filter (Alt. UI)") );
  g->addWidget( mEditFilterButtonTBB, 4, 1 );

  connect( mEditFilterButtonTBB, SIGNAL( clicked() ), this, SLOT( slotEditFilterTBBButtonClicked() ) );

  g->addItem( new QSpacerItem( 10, 10 ), 5, 1 );


  mDeleteFilterButton = new QPushButton( base );
  mDeleteFilterButton->setIcon( KIcon( "document-close" ) );
  mDeleteFilterButton->setText( i18n( "Delete Filter") );
  g->addWidget( mDeleteFilterButton, 6, 1 );

  connect( mDeleteFilterButton, SIGNAL( clicked() ), this, SLOT( slotDeleteFilterButtonClicked() ) );

  g->addItem( new QSpacerItem( 10, 10 ), 7, 1 );

  mApplyFilterToItemButton = new QPushButton( base );
  mApplyFilterToItemButton->setIcon( KIcon( "roll" ) );
  mApplyFilterToItemButton->setText( i18n( "Apply To Item") );
  g->addWidget( mApplyFilterToItemButton, 8, 1 );

  connect( mApplyFilterToItemButton, SIGNAL( clicked() ), this, SLOT( slotApplyFilterToItemButtonClicked() ) );

  mApplyFilterToCollectionButton = new QPushButton( base );
  mApplyFilterToCollectionButton->setIcon( KIcon( "roll" ) );
  mApplyFilterToCollectionButton->setText( i18n( "Apply To Collection") );
  g->addWidget( mApplyFilterToCollectionButton, 9, 1 );

  connect( mApplyFilterToCollectionButton, SIGNAL( clicked() ), this, SLOT( slotApplyFilterToCollectionButtonClicked() ) );


  g->setColumnStretch( 0, 1 );
  g->setRowStretch( 10, 1 );

  setMinimumSize( 500, 400 );

  enableDisableButtons();

  Akonadi::Control::widgetNeedsAkonadi( mFilterListWidget );

  QTimer::singleShot( 500, this, SLOT( slotListFilters() ) );
}

MainWindow::~MainWindow()
{
  qDeleteAll( mComponentFactories );
  qDeleteAll( mEditorFactories );
  delete mFilterAgent;
}

void MainWindow::slotListFilters()
{
  if( !Akonadi::ServerManager::isRunning() )
  {
    QTimer::singleShot( 1000, this, SLOT( slotListFilters() ) );
    return;
  }

  listFilters();
}

void MainWindow::enableDisableButtons()
{
  int sel = mFilterListWidget->selectedItems().count();

  bool gotSelected = sel > 0;

  //mNewFilterButtonLBB->setEnabled( true );
  //mNewFilterButtonTBB->setEnabled( true );
  mEditFilterButtonLBB->setEnabled( gotSelected );
  mEditFilterButtonTBB->setEnabled( gotSelected );
  mApplyFilterToItemButton->setEnabled( gotSelected );
  mApplyFilterToCollectionButton->setEnabled( gotSelected );
  mDeleteFilterButton->setEnabled( gotSelected );
}

bool MainWindow::fetchFilterData( Filter * filter, QString &source )
{
  Q_ASSERT( filter );

  QString filterId = filter->id();

  Q_ASSERT( !filterId.isEmpty() );


  QDBusPendingReply< int, QString, QString, QList< Akonadi::Collection::Id > > rProps = mFilterAgent->getFilterProperties( filterId );
  rProps.waitForFinished();

  if( rProps.isError() )
  {
    KMessageBox::error( this, rProps.error().message(), i18n( "Could not fetch filter properties" ) );
    return false;
  }

  Akonadi::Filter::Agent::Status ret = static_cast< Akonadi::Filter::Agent::Status >( rProps.argumentAt< 0 >() );

  if( ret != Akonadi::Filter::Agent::Success )
  {
    KMessageBox::error( this, Akonadi::Filter::Agent::statusDescription( ret ), i18n( "Could not fetch filter properties" ) );
    return false;
  }

  QString mimeType = rProps.argumentAt< 1 >();
  source = rProps.argumentAt< 2 >();
  QList< Akonadi::Collection::Id > attachedCollectionIds = rProps.argumentAt< 3 >();

  Akonadi::Filter::ComponentFactory * componentFactory = mComponentFactories.value( mimeType, 0 );
  if( !componentFactory )
  {
    KMessageBox::error( this, i18n( "The filter mimetype has no installed component factory" ), i18n( "Internal error" ) );
    return false;
  }

  Akonadi::Filter::UI::EditorFactory * editorFactory = mEditorFactories.value( mimeType, 0 );
  if( !editorFactory )
  {
    KMessageBox::error( this, i18n( "The filter mimetype has no installed editor factory" ), i18n( "Internal error" ) );
    return false;
  }

  Akonadi::Filter::IO::SieveDecoder decoder( componentFactory );
  Akonadi::Filter::Program * prog = decoder.run( source.toUtf8() );
  if( !prog )
  {
    KMessageBox::error(
        this,
        decoder.errorStack().htmlErrorMessage(
            i18n( "Failed to decode the filter from sieve format" )
          ),
        i18n( "Could not edit filter" )
      );
    return false;    
  }

  filter->setMimeType( mimeType );
  filter->setComponentFactory( componentFactory );
  filter->setEditorFactory( editorFactory );
  filter->setProgram( prog );

  filter->removeAllCollections();

  foreach( Akonadi::Collection::Id id, attachedCollectionIds )
  {
    Akonadi::CollectionFetchJob * job = new Akonadi::CollectionFetchJob( Akonadi::Collection( id ), Akonadi::CollectionFetchJob::Base );

    if( !job->exec() )
    {
      kDebug() << "Ooops... failed to execute the fetch job for collection" << id;
      continue;
    }

    Akonadi::Collection::List collections = job->collections();

    // The job deletes itself

    if( collections.count() < 1 )
    {
      kDebug() << "Ooops... the collection fetch job for collection" << id << "didn't fetch anything";
      continue;
    }

    Q_ASSERT( collections.count() == 1 );

    Akonadi::Collection collection = collections[0];

    if( !collection.isValid() )
    {
      kDebug() << "Ooops... the filter seems to be attached to an invalid collection" << id;
      continue;
    }

    filter->addCollection( new Akonadi::Collection( collection ) );
  }

  return true;
}

void MainWindow::listFilters()
{
  QDBusPendingReply< QStringList > r = mFilterAgent->enumerateFilters( QString() );
  r.waitForFinished();

  mFilterListWidget->clear();

  if( r.isError() )
  {
    KMessageBox::error( this, r.error().message(), i18n( "Could not enumerate filters" ) );
    enableDisableButtons();
    return;
  }

  QStringList filters = r.value();

  FilterListWidgetItem * item;

  foreach( QString id, filters )
  {
    Filter * filter = new Filter();
    filter->setId( id );

    QString source;

    if( !fetchFilterData( filter, source ) )
    {
      delete filter;
      kWarning() << "Could not fetch data for filter with id" << id;
      continue;
    }

    item = new FilterListWidgetItem( mFilterListWidget, filter );
    item->setToolTip( source );
  }

  enableDisableButtons();
}

void MainWindow::slotEditFilterLBBButtonClicked()
{
  editFilter( true );
}

void MainWindow::slotEditFilterTBBButtonClicked()
{
  editFilter( false );
}

void MainWindow::editFilter( bool lbb )
{
  QString filterId;

  FilterListWidgetItem * item = dynamic_cast< FilterListWidgetItem * >( mFilterListWidget->currentItem() );
  if( !item )
    return;

  QString source;

  if( !fetchFilterData( item->filter(), source ) )
  {
    KMessageBox::error(
        this,
        i18n( "Could not fetch filter data" ),
        i18n( "Could not edit filter" )
      );
    listFilters();
    return;    
  }

  FilterEditor ed( this, item->filter(), lbb );
  if( ed.exec() != KDialog::Accepted )
    return;

  Akonadi::Filter::IO::SieveEncoder encoder;
  QByteArray utf8Source = encoder.run( item->filter()->program() );

  if( utf8Source.isEmpty() )
  {
    KMessageBox::error(
        this,
        encoder.errorStack().htmlErrorMessage(
            i18n( "Failed to encode the filter to sieve format" )
          ),
        i18n( "Could not edit filter" )
      );
    return;    
  }

  source = QString::fromUtf8( utf8Source );

  qDebug( "FILTER SOURCE:" );
  qDebug( "%s", utf8Source.data() );
  qDebug( "END OF FILTER SOURCE:" );


  QDBusPendingReply< int > rChange = mFilterAgent->changeFilter( item->filter()->id(), source, item->filter()->collectionsAsIdList() );
  rChange.waitForFinished();

  if( rChange.isError() )
  {
    KMessageBox::error( this, rChange.error().message(), i18n( "Could not change filter" ) );
    return;
  }

  Akonadi::Filter::Agent::Status ret = static_cast< Akonadi::Filter::Agent::Status >( rChange.argumentAt< 0 >() );

  if( ret != Akonadi::Filter::Agent::Success )
  {
    KMessageBox::error( this, Akonadi::Filter::Agent::statusDescription( ret ), i18n( "Could not fetch filter properties" ) );
    return;
  }

  listFilters();
}

void MainWindow::slotNewFilterLBBButtonClicked()
{
  newFilter( true );
}

void MainWindow::slotNewFilterTBBButtonClicked()
{
  newFilter( false );
}

void MainWindow::newFilter( bool lbb )
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

  FilterEditor ed( this, &filter, lbb );
  if( ed.exec() != KDialog::Accepted )
    return;

  Akonadi::Filter::IO::SieveEncoder encoder;
  QByteArray utf8Source = encoder.run( filter.program() );

  if( utf8Source.isEmpty() )
  {
    KMessageBox::error(
        this,
        encoder.errorStack().htmlErrorMessage(
            i18n( "Failed to encode the filter to sieve format" )
          ),
        i18n( "Could not edit filter" )
      );
    return;    
  }

  QString source = QString::fromUtf8( utf8Source );

  qDebug( "FILTER SOURCE:" );
  qDebug( "%s", utf8Source.data() );
  qDebug( "END OF FILTER SOURCE:" );


  QDBusPendingReply< int > rCreate = mFilterAgent->createFilter( filter.id(), filter.mimeType(), source );
  rCreate.waitForFinished();

  if( rCreate.isError() )
  {
    KMessageBox::error( this, rCreate.error().message(), i18n( "Could not crate new filter" ) );
    return;
  }

  Akonadi::Filter::Agent::Status ret = static_cast< Akonadi::Filter::Agent::Status >( rCreate.argumentAt< 0 >() );

  if( ret != Akonadi::Filter::Agent::Success )
  {
    KMessageBox::error( this, Akonadi::Filter::Agent::statusDescription( ret ), i18n( "Could not crate new filter" ) );
    return;
  }

  QDBusPendingReply< int > rAttach = mFilterAgent->attachFilter( filter.id(), filter.collectionsAsIdList() );

  if( rAttach.isError() )
  {
    KMessageBox::error( this, rAttach.error().message(), i18n( "Could not crate new filter" ) );
    return;
  }

  ret = static_cast< Akonadi::Filter::Agent::Status >( rAttach.argumentAt< 0 >() );

  if( ret != Akonadi::Filter::Agent::Success )
  {
    KMessageBox::error( this, Akonadi::Filter::Agent::statusDescription( ret ), i18n( "Could not crate new filter" ) );
    return;
  }

  listFilters();

  // Done!  
}

void MainWindow::slotDeleteFilterButtonClicked()
{
  FilterListWidgetItem * item = dynamic_cast< FilterListWidgetItem * >( mFilterListWidget->currentItem() );
  if( !item )
    return;

  QDBusPendingReply< int > rDelete = mFilterAgent->deleteFilter( item->filter()->id() );
  rDelete.waitForFinished();

  if( rDelete.isError() )
  {
    KMessageBox::error( this, rDelete.error().message(), i18n( "Could not delete filter" ) );
    return;
  }

  Akonadi::Filter::Agent::Status ret = static_cast< Akonadi::Filter::Agent::Status >( rDelete.argumentAt< 0 >() );

  if( ret != Akonadi::Filter::Agent::Success )
  {
    KMessageBox::error( this, Akonadi::Filter::Agent::statusDescription( ret ), i18n( "Could not delete filter" ) );
    return;
  }

  listFilters();
}

void MainWindow::slotFilterListWidgetSelectionChanged()
{
  enableDisableButtons();
}

void MainWindow::slotApplyFilterToItemButtonClicked()
{
}

void MainWindow::slotApplyFilterToCollectionButtonClicked()
{
}
