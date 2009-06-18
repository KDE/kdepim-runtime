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
#include "storecollectionmodel.h"

#include <akonadi/collection.h>
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

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QPushButton>

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
    mButtonBox( 0 ),
    mSyncAction( 0 ),
    mSyncButton( 0 ),
    mInfoTextLabel( 0 ),
    mSourcesDialog( 0 ),
    mSourcesButton( 0 )
{
  QVBoxLayout *mainLayout = new QVBoxLayout( this );
  mainLayout->setMargin( KDialog::marginHint() );
  mainLayout->setSpacing( KDialog::spacingHint() );

  mCollectionModel = new Akonadi::StoreCollectionModel( this );

  QWidget *widget = new QWidget( this );

  QHBoxLayout *collectionLayout = new QHBoxLayout( widget );
  collectionLayout->setMargin( KDialog::marginHint() );
  collectionLayout->setSpacing( KDialog::spacingHint() );

  Akonadi::CollectionFilterProxyModel *filterModel =
    new Akonadi::CollectionFilterProxyModel( this );
  filterModel->addMimeTypeFilters( mimeList );
  filterModel->setSourceModel( mCollectionModel );

  mCollectionView = new Akonadi::CollectionView( widget );
  mCollectionView->setSelectionMode( QAbstractItemView::SingleSelection );
  mCollectionView->setModel( filterModel );
  mCollectionView->header()->setResizeMode( QHeaderView::ResizeToContents );

  connect( mCollectionView, SIGNAL( currentChanged( const Akonadi::Collection& ) ),
           this, SLOT( collectionChanged( const Akonadi::Collection& ) ) );

  collectionLayout->addWidget( mCollectionView );

  KActionCollection *actionCollection = new KActionCollection( this );

  Akonadi::StandardActionManager *actionManager =
    new Akonadi::StandardActionManager( actionCollection, this );
  actionManager->setCollectionSelectionModel( mCollectionView->selectionModel() );

  mSyncAction = actionManager->createAction( Akonadi::StandardActionManager::SynchronizeCollections );

  mButtonBox = new QDialogButtonBox( Qt::Vertical, widget );
  collectionLayout->addWidget( mButtonBox );

  mSyncButton = new QPushButton( mSyncAction->text() );
  mSyncButton->setIcon( mSyncAction->icon() );
  mButtonBox->addButton( mSyncButton, QDialogButtonBox::ActionRole );
  connect( mSyncButton, SIGNAL( clicked() ), mSyncAction, SLOT( trigger() ) );
  mSourcesDialog = new AkonadiResourceDialog( mimeList, this );

  mSourcesButton = new QPushButton( this );
  mButtonBox->addButton( mSourcesButton, QDialogButtonBox::ActionRole );

  connect( mSourcesButton, SIGNAL( clicked() ), mSourcesDialog, SLOT( show() ) );

  mInfoTextLabel = new QLabel( this );
  mInfoTextLabel->setWordWrap( true );

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

  Akonadi::StoreCollectionModel::StoreItemsByCollection storeMapping;

  mStoreCollections = akonadiResource->storeConfig().storeCollectionsByMimeType();
  StoreConfigIface::CollectionsByMimeType::const_iterator it    = mStoreCollections.constBegin();
  StoreConfigIface::CollectionsByMimeType::const_iterator endIt = mStoreCollections.constEnd();
  for ( ; it != endIt; ++it ) {
    storeMapping[ it.value().id() ] << mItemTypes[ it.key() ];
  }

  mCollectionModel->setStoreMapping( storeMapping );
}

void ResourceConfigBase::saveSettings( KRES::Resource *resource )
{
  SharedResourceIface* akonadiResource = dynamic_cast<SharedResourceIface*>( resource );
  if ( akonadiResource == 0 ) {
    kError( 5650 ) << "Given resource is not an Akonadi bridge";
    return;
  }

  akonadiResource->storeConfig().setStoreCollectionsByMimeType( mStoreCollections );
}

void ResourceConfigBase::connectMimeCheckBoxes()
{
  foreach ( const QCheckBox *checkBox, mMimeCheckBoxes ) {
    connect( checkBox, SIGNAL( toggled( bool ) ), SLOT( mimeCheckBoxToggled( bool ) ) );
  }
}

void ResourceConfigBase::updateCollectionButtonState()
{
  mSyncButton->setEnabled( mSyncAction->isEnabled() );
}

void ResourceConfigBase::collectionChanged( const Akonadi::Collection &collection )
{
  mCollection = collection;

  MimeCheckBoxes::const_iterator it    = mMimeCheckBoxes.constBegin();
  MimeCheckBoxes::const_iterator endIt = mMimeCheckBoxes.constEnd();
  for ( ; it != endIt; ++it ) {
    const QString mimeType = it.key();
    QCheckBox *checkBox = it.value();

    checkBox->blockSignals( true );
    checkBox->setChecked( mStoreCollections.value( mimeType, Akonadi::Collection() ) == mCollection );
    checkBox->blockSignals( false );

    checkBox->setEnabled( mCollection.isValid() );
  }
}

void ResourceConfigBase::mimeCheckBoxToggled( bool checked )
{
  QString mimeType;
  QCheckBox *checkBox = 0;
  MimeCheckBoxes::const_iterator it    = mMimeCheckBoxes.constBegin();
  MimeCheckBoxes::const_iterator endIt = mMimeCheckBoxes.constEnd();
  for ( ; it != endIt; ++it ) {
    if ( it.value() == QObject::sender() ) {
      mimeType = it.key();
      checkBox = it.value();
      break;
    }
  }

  Q_ASSERT( !mimeType.isEmpty() && checkBox != 0 );

  const QString itemType = mItemTypes.value( mimeType, QString() );

  Akonadi::StoreCollectionModel::StoreItemsByCollection storeMapping = mCollectionModel->storeMapping();

  if ( checked ) {
    Akonadi::StoreCollectionModel::StoreItemsByCollection::iterator it =
      storeMapping.begin();
    Akonadi::StoreCollectionModel::StoreItemsByCollection::iterator endIt =
      storeMapping.end();
    for ( ; it != endIt; ++it ) {
      it.value().removeAll( itemType );
    }

    storeMapping[ mCollection.id() ] << itemType;
    mStoreCollections[ mimeType ] = mCollection;
  } else {
    storeMapping[ mCollection.id() ].removeAll( itemType );
    mStoreCollections.remove( mimeType );
  }

  mCollectionModel->setStoreMapping( storeMapping );
}

// kate: space-indent on; indent-width 2; replace-tabs on;
