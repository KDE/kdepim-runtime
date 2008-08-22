/*
    This file is part of libkabc.
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "resourceakonadiconfig.h"
#include "resourceakonadi.h"

#include <akonadi/collection.h>
#include <akonadi/collectionmodel.h>
#include <akonadi/collectionfilterproxymodel.h>
#include <akonadi/collectionview.h>
#include <akonadi/standardactionmanager.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kconfig.h>
#include <kcmoduleloader.h>
#include <ktabwidget.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>

static QModelIndex findCollection( const Akonadi::Collection &collection,
        const QModelIndex &parent, QAbstractItemModel *model)
{
  const int rowCount = model->rowCount( parent );

  for ( int row = 0; row < rowCount; ++row ) {
    QModelIndex index = model->index( row, 0, parent );
    if ( !index.isValid() )
      continue;

    QVariant data = model->data( index, Akonadi::CollectionModel::CollectionIdRole );
    if ( !data.isValid() )
      continue;

    if ( data.toInt() == collection.id() ) {
      return index;
    }

    index = findCollection( collection, index, model );
    if ( index.isValid() )
      return index;
  }

  return QModelIndex();
}

using namespace KABC;

ResourceAkonadiConfig::ResourceAkonadiConfig( QWidget *parent )
  : KRES::ConfigWidget( parent ),
    mCollectionView( 0 ),
    mCreateAction( 0 ),
    mDeleteAction( 0 ),
    mSyncAction( 0 ),
    mSubscriptionAction( 0 ),
    mCreateButton( 0 ),
    mDeleteButton( 0 ),
    mSyncButton( 0 ),
    mSubscriptionButton( 0 )
{
  QVBoxLayout *mainLayout = new QVBoxLayout( this );
  mainLayout->setMargin( KDialog::marginHint() );
  mainLayout->setSpacing( KDialog::spacingHint() );

  KTabWidget *tabWidget = new KTabWidget( this );

  // list of contact data MIME types
  // TODO: check if we need to add text/x-vcard
  QStringList mimeList;
  mimeList << "text/directory";

  Akonadi::CollectionModel *model = new Akonadi::CollectionModel( this );

  QWidget *widget = new QWidget( this );

  QHBoxLayout *collectionLayout = new QHBoxLayout( widget );
  collectionLayout->setMargin( KDialog::marginHint() );
  collectionLayout->setSpacing( KDialog::spacingHint() );

  Akonadi::CollectionFilterProxyModel *filterModel =
    new Akonadi::CollectionFilterProxyModel( this );
  filterModel->addMimeTypeFilters( mimeList );
  filterModel->setSourceModel( model );

  mCollectionView = new Akonadi::CollectionView( widget );
  mCollectionView->setSelectionMode( QAbstractItemView::SingleSelection );
  mCollectionView->setModel( filterModel );

  connect( mCollectionView, SIGNAL( currentChanged( const Akonadi::Collection& ) ),
           this, SLOT( collectionChanged( const Akonadi::Collection& ) ) );
  connect( mCollectionView->model(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
           this, SLOT( collectionsInserted( const QModelIndex&, int, int ) ) );

  collectionLayout->addWidget( mCollectionView );

  KActionCollection *actionCollection = new KActionCollection( this );

  Akonadi::StandardActionManager *actionManager =
    new Akonadi::StandardActionManager( actionCollection, this );
  actionManager->setCollectionSelectionModel( mCollectionView->selectionModel() );

  mCreateAction = actionManager->createAction( Akonadi::StandardActionManager::CreateCollection );

  mDeleteAction = actionManager->createAction( Akonadi::StandardActionManager::DeleteCollections );

  mSyncAction = actionManager->createAction( Akonadi::StandardActionManager::SynchronizeCollections );

  mSubscriptionAction = actionManager->createAction( Akonadi::StandardActionManager::ManageLocalSubscriptions );

  QDialogButtonBox *buttonBox = new QDialogButtonBox( Qt::Vertical, widget );
  collectionLayout->addWidget( buttonBox );

  mCreateButton = new QPushButton( mCreateAction->text() );
  mCreateButton->setIcon( mCreateAction->icon() );
  buttonBox->addButton( mCreateButton, QDialogButtonBox::ActionRole );
  connect( mCreateButton, SIGNAL( clicked() ), mCreateAction, SLOT( trigger() ) );

  mDeleteButton = new QPushButton( mDeleteAction->text() );
  mDeleteButton->setIcon( mDeleteAction->icon() );
  buttonBox->addButton( mDeleteButton, QDialogButtonBox::DestructiveRole );
  connect( mDeleteButton, SIGNAL( clicked() ), mDeleteAction, SLOT( trigger() ) );

  mSyncButton = new QPushButton( mSyncAction->text() );
  mSyncButton->setIcon( mSyncAction->icon() );
  buttonBox->addButton( mSyncButton, QDialogButtonBox::ActionRole );
  connect( mSyncButton, SIGNAL( clicked() ), mSyncAction, SLOT( trigger() ) );

  mSubscriptionButton = new QPushButton( mSubscriptionAction->text() );
  mSubscriptionButton->setIcon( mSubscriptionAction->icon() );
  buttonBox->addButton( mSubscriptionButton, QDialogButtonBox::ActionRole );
  connect( mSubscriptionButton, SIGNAL( clicked() ), mSubscriptionAction, SLOT( trigger() ) );

  const QString foldersTitle = i18nc( "@title:tab", "Manage addressbook folders" );
  tabWidget->addTab( widget, foldersTitle );

  widget = KCModuleLoader::loadModule( "kcm_akonadi_resources",
                                       KCModuleLoader::Inline, this, mimeList );

  const QString sourcesTitle = i18nc( "@title:tab", "Manage addressbook sources" );
  tabWidget->addTab( widget, sourcesTitle );

  QLabel *label = new QLabel( this );
  label->setText( i18nc( "@info", "<title>Please select the folder for storing"
                         " newly created contacts.</title><note>If the folder"
                         " list below is empty, you might have to add an"
                         " addressbook source through <interface>%1</interface>"
                         " <br>or change which folders you are subscribed to"
                         " through <interface>%2</interface></note>",
                         sourcesTitle, mSubscriptionButton->text() ) );

  mainLayout->addWidget( label );
  mainLayout->addWidget( tabWidget );

  updateCollectionButtonState();

  connect( actionManager, SIGNAL( actionStateUpdated() ),
           this, SLOT( updateCollectionButtonState() ) );
}

void ResourceAkonadiConfig::loadSettings( KRES::Resource *res )
{
  ResourceAkonadi *resource = dynamic_cast<ResourceAkonadi*>( res );

  if ( !resource ) {
    kDebug(5700) << "cast failed";
    return;
  }

  mCollection = resource->storeCollection();

  QModelIndex index = findCollection( mCollection, mCollectionView->rootIndex(), mCollectionView->model() );
  if ( index.isValid() )
    mCollectionView->setCurrentIndex( index );
}

void ResourceAkonadiConfig::saveSettings( KRES::Resource *res )
{
  ResourceAkonadi *resource = dynamic_cast<ResourceAkonadi*>( res );

  if ( !resource ) {
    kDebug(5700) << "cast failed";
    return;
  }

  resource->setStoreCollection( mCollection );
}

void ResourceAkonadiConfig::updateCollectionButtonState()
{
  mCreateButton->setEnabled( mCreateAction->isEnabled() );
  mDeleteButton->setEnabled( mDeleteAction->isEnabled() );
  mSyncButton->setEnabled( mSyncAction->isEnabled() );
  mSubscriptionButton->setEnabled( mSubscriptionAction->isEnabled() );
}

void ResourceAkonadiConfig::collectionChanged( const Akonadi::Collection &collection )
{
  mCollection = collection;
}

void ResourceAkonadiConfig::collectionsInserted( const QModelIndex &parent, int start, int end )
{
  QAbstractItemModel *model = mCollectionView->model();

  for ( int row = start; row <= end; ++row ) {
    QModelIndex index = model->index( row, 0, parent );
    if ( !index.isValid() )
      continue;

    QVariant data = model->data( index, Akonadi::CollectionModel::CollectionIdRole );
    if ( !data.isValid() )
      continue;

    if ( data.toInt() == mCollection.id() ) {
      mCollectionView->setCurrentIndex( index );
      return;
    }

    index = findCollection( mCollection, index, model );
    if ( index.isValid() ) {
      mCollectionView->setCurrentIndex( index );
      return;
    }
  }
}

ResourceAkonadiConfigDialog::ResourceAkonadiConfigDialog( KRES::Resource *resource )
  : KDialog(), mResource( resource ), mConfig( 0 )
{
  setCaption( i18nc( "@title:window", "Select Addressbook Folder") );

  setButtons( KDialog::Ok | KDialog::Cancel );

  mConfig = new ResourceAkonadiConfig( this );
  setMainWidget( mConfig );

  mConfig->loadSettings( mResource );
}

void ResourceAkonadiConfigDialog::accept()
{
  mConfig->saveSettings( mResource );
  KDialog::accept();
}

#include "resourceakonadiconfig.moc"
