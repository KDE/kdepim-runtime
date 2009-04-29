/*
    This file is part of kdepim.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "resourceconfigbase.h"

#include "sharedresourceiface.h"

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

class AkonadiResourceDialog : public KDialog
{
  public:
    AkonadiResourceDialog( const QStringList &mimeList, QWidget *parent )
      : KDialog( parent )
    {
      QWidget *widget = KCModuleLoader::loadModule( "kcm_akonadi_resources",
                                                    KCModuleLoader::Inline, this, mimeList );
      setMainWidget( widget );

      setButtons( Close );
      setDefaultButton( Close );
    }
};

ResourceConfigBase::ResourceConfigBase( const QStringList &mimeList, QWidget *parent )
  : KRES::ConfigWidget( parent ),
    mCollectionView( 0 ),
    mCreateAction( 0 ),
    mDeleteAction( 0 ),
    mSyncAction( 0 ),
    mSubscriptionAction( 0 ),
    mCreateButton( 0 ),
    mDeleteButton( 0 ),
    mSyncButton( 0 ),
    mSubscriptionButton( 0 ),
    mInfoTextLabel( 0 ),
    mSourcesDialog( 0 ),
    mSourcesButton( 0 )
{
  QVBoxLayout *mainLayout = new QVBoxLayout( this );
  mainLayout->setMargin( KDialog::marginHint() );
  mainLayout->setSpacing( KDialog::spacingHint() );

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

  mSourcesDialog = new AkonadiResourceDialog( mimeList, this );

  mSourcesButton = new QPushButton( this );
  buttonBox->addButton( mSourcesButton, QDialogButtonBox::ActionRole );

  connect( mSourcesButton, SIGNAL( clicked() ), mSourcesDialog, SLOT( show() ) );

  mInfoTextLabel = new QLabel( this );

  mainLayout->addWidget( mInfoTextLabel );
  mainLayout->addWidget( widget );

  updateCollectionButtonState();

  connect( actionManager, SIGNAL( actionStateUpdated() ),
           this, SLOT( updateCollectionButtonState() ) );
}

ResourceConfigBase::~ResourceConfigBase()
{
}

void ResourceConfigBase::loadSettings( KRES::Resource *resource )
{
  SharedResourceIface* akonadiResource = dynamic_cast<SharedResourceIface*>( resource );
  if ( akonadiResource == 0 ) {
    kError( 5650 ) << "Given resource is not an Akonadi bridge";
    return;
  }

  mCollection = akonadiResource->storeCollection();

  QModelIndex index = findCollection( mCollection, mCollectionView->rootIndex(), mCollectionView->model() );
  if ( index.isValid() )
    mCollectionView->setCurrentIndex( index );
}

void ResourceConfigBase::saveSettings( KRES::Resource *resource )
{
  SharedResourceIface* akonadiResource = dynamic_cast<SharedResourceIface*>( resource );
  if ( akonadiResource == 0 ) {
    kError( 5650 ) << "Given resource is not an Akonadi bridge";
    return;
  }

  akonadiResource->setStoreCollection( mCollection );
}

void ResourceConfigBase::updateCollectionButtonState()
{
  mCreateButton->setEnabled( mCreateAction->isEnabled() );
  mDeleteButton->setEnabled( mDeleteAction->isEnabled() );
  mSyncButton->setEnabled( mSyncAction->isEnabled() );
  mSubscriptionButton->setEnabled( mSubscriptionAction->isEnabled() );
}

void ResourceConfigBase::collectionChanged( const Akonadi::Collection &collection )
{
  mCollection = collection;
}

void ResourceConfigBase::collectionsInserted( const QModelIndex &parent, int start, int end )
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
// kate: space-indent on; indent-width 2; replace-tabs on;
