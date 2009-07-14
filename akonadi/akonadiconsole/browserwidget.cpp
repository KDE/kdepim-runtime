/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "browserwidget.h"

#include "collectionattributespage.h"
#include "collectioninternalspage.h"
#include "collectionaclpage.h"
#include "settings.h"

#include <akonadi/attributefactory.h>
#include <akonadi/job.h>
#include <akonadi/collectionmodel.h>
#include <akonadi/collectionstatisticsmodel.h>
#include <akonadi/collectionview.h>
#include <akonadi/control.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/collectionfilterproxymodel.h>
#include <akonadi/collectionpropertiesdialog.h>
#include <akonadi/standardactionmanager.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>
#include <xml/xmlwritejob.h>

#include <akonadi_next/entitytreemodel.h>
#include <akonadi_next/entitytreeview.h>
#include <akonadi_next/statisticsproxymodel.h>
#include <akonadi_next/statisticstooltipproxymodel.h>

#include <kcal/kcalmodel.h>
#include <kcal/incidence.h>
#include <kabc/kabcmodel.h>
#include <kabc/kabcitembrowser.h>
#include <akonadi/kmime/messagemodel.h>

#include <libkdepim-copy/addresseeview.h>

#include <kdebug.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kxmlguiwindow.h>

#ifdef NEPOMUK_FOUND
#include <nepomuk/resource.h>
#include <nepomuk/resourcemanager.h>
#include <nepomuk/variant.h>
#endif

#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QtGui/QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTimer>

using namespace Akonadi;

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionAttributePageFactory, CollectionAttributePage)
AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionInternalsPageFactory, CollectionInternalsPage)
AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionAclPageFactory, CollectionAclPage)

BrowserWidget::BrowserWidget(KXmlGuiWindow *xmlGuiWindow, QWidget * parent) :
    QWidget( parent ),
    mItemModel( 0 ),
    mCurrentCollection( 0 ),
    mAttrModel( 0 ),
#ifdef NEPOMUK_FOUND
    mNepomukModel( 0 ),
#endif
    mStdActionManager( 0 ),
    mMonitor( 0 )
{
  Q_ASSERT( xmlGuiWindow );
  QVBoxLayout *layout = new QVBoxLayout( this );

  QSplitter *splitter = new QSplitter( Qt::Horizontal, this );
  splitter->setObjectName( "collectionSplitter" );
  layout->addWidget( splitter );

  mCollectionView = new Akonadi::EntityTreeView( xmlGuiWindow, this );
  connect( mCollectionView, SIGNAL(clicked(QModelIndex)), SLOT(collectionActivated(QModelIndex)) );
  splitter->addWidget( mCollectionView );

  Session *session = new Session( "AkonadiConsole Browser Widget", this );

  // monitor collection changes
  Monitor *monitor = new Monitor( this );
  monitor->setCollectionMonitored( Collection::root() );
  monitor->fetchCollection( true );
  monitor->setAllMonitored( true );

  mCollectionModel = new EntityTreeModel( session, monitor, this );

  StatisticsToolTipProxyModel *proxy1 = new StatisticsToolTipProxyModel( this );
  proxy1->setSourceModel( mCollectionModel );

  StatisticsProxyModel *proxy2 = new StatisticsProxyModel( this );
  proxy2->setSourceModel( proxy1 );

  QSortFilterProxyModel *sortModel = new QSortFilterProxyModel( this );
  sortModel->setDynamicSortFilter( true );
  sortModel->setSortCaseSensitivity( Qt::CaseInsensitive );
  sortModel->setSourceModel( proxy2 );
  mCollectionView->setModel( sortModel );

  QSplitter *splitter2 = new QSplitter( Qt::Vertical, this );
  splitter2->setObjectName( "itemSplitter" );
  splitter->addWidget( splitter2 );

  QWidget *itemViewParent = new QWidget( this );
  itemUi.setupUi( itemViewParent );

  itemUi.modelBox->addItem( "Generic" );
  itemUi.modelBox->addItem( "Mail" );
  itemUi.modelBox->addItem( "Contacts" );
  itemUi.modelBox->addItem( "Calendar" );
  connect( itemUi.modelBox, SIGNAL(activated(int)), SLOT(modelChanged()) );
  QTimer::singleShot( 0, this, SLOT(modelChanged()) );

  itemUi.itemView->setXmlGuiWindow( xmlGuiWindow );
  itemUi.itemView->setModel( mItemModel );
  itemUi.itemView->setSelectionMode( QAbstractItemView::ExtendedSelection );
  connect( itemUi.itemView, SIGNAL(activated(QModelIndex)), SLOT(itemActivated(QModelIndex)) );
  connect( itemUi.itemView, SIGNAL(clicked(QModelIndex)), SLOT(itemActivated(QModelIndex)) );
  splitter2->addWidget( itemViewParent );
  itemViewParent->layout()->setMargin( 0 );

  QWidget *contentViewParent = new QWidget( this );
  contentUi.setupUi( contentViewParent );
  connect( contentUi.saveButton, SIGNAL(clicked()), SLOT(save()) );
  splitter2->addWidget( contentViewParent );

  connect( contentUi.attrAddButton, SIGNAL(clicked()), SLOT(addAttribute()) );
  connect( contentUi.attrDeleteButton, SIGNAL(clicked()), SLOT(delAttribute()) );

  CollectionPropertiesDialog::registerPage( new CollectionAclPageFactory() );
  CollectionPropertiesDialog::registerPage( new CollectionAttributePageFactory() );
  CollectionPropertiesDialog::registerPage( new CollectionInternalsPageFactory() );

  Control::widgetNeedsAkonadi( this );

  mStdActionManager = new StandardActionManager( xmlGuiWindow->actionCollection(), xmlGuiWindow );
  mStdActionManager->setCollectionSelectionModel( mCollectionView->selectionModel() );
  mStdActionManager->setItemSelectionModel( itemUi.itemView->selectionModel() );
  mStdActionManager->createAllActions();

#ifndef NEPOMUK_FOUND
  contentUi.mainTabWidget->removeTab( contentUi.mainTabWidget->indexOf( contentUi.nepomukTab ) );
#else
  Nepomuk::ResourceManager::instance()->init();
#endif
}

void BrowserWidget::clear()
{
  contentUi.stack->setCurrentWidget( contentUi.unsupportedTypePage );
  contentUi.dataView->clear();
  contentUi.id->clear();
  contentUi.remoteId->clear();
  contentUi.mimeType->clear();
  contentUi.revision->clear();
  contentUi.size->clear();
  contentUi.modificationtime->clear();
  contentUi.flags->clear();
  contentUi.attrView->setModel( 0 );
}

void BrowserWidget::collectionActivated(const QModelIndex & index)
{
  mCurrentCollection = mCollectionView->model()->data( index, EntityTreeModel::CollectionIdRole ).toLongLong();
  if ( mCurrentCollection <= 0 )
    return;
  const Collection col = mCollectionView->currentIndex().data( EntityTreeModel::CollectionRole ).value<Collection>();
  mItemModel->setCollection( col );
  clear();
}

void BrowserWidget::itemActivated(const QModelIndex & index)
{
  const Item item = mItemModel->itemForIndex( index );
  if ( !item.isValid() ) {
    clear();
    return;
  }

  ItemFetchJob *job = new ItemFetchJob( item, this );
  job->fetchScope().fetchFullPayload();
  job->fetchScope().fetchAllAttributes();
  connect( job, SIGNAL(result(KJob*)), SLOT(itemFetchDone(KJob*)) );
}

void BrowserWidget::itemFetchDone(KJob * job)
{
  ItemFetchJob *fetch = static_cast<ItemFetchJob*>( job );
  if ( job->error() ) {
    kWarning( 5265 ) << "Item fetch failed: " << job->errorString();
  } else if ( fetch->items().isEmpty() ) {
    kWarning( 5265 ) << "No item found!";
  } else {
    const Item item = fetch->items().first();
    setItem( item );
  }
}

void BrowserWidget::setItem( const Akonadi::Item &item )
{
  mCurrentItem = item;
  if ( item.hasPayload<KABC::Addressee>() ) {
    const KABC::Addressee addr = item.payload<KABC::Addressee>();

    contentUi.addresseeView->setAddressee( addr );
    contentUi.stack->setCurrentWidget( contentUi.addresseeViewPage );
  } else if ( item.hasPayload<KCal::Incidence::Ptr>() ) {
    contentUi.incidenceView->setItem( item );
    contentUi.stack->setCurrentWidget( contentUi.incidenceViewPage );
  } else if ( item.mimeType() == "message/rfc822" )
  {
    contentUi.mailView->setMessageItem( item, KMReaderWin::Force );
    contentUi.stack->setCurrentWidget( contentUi.mailViewPage );
  } else
  {
    contentUi.stack->setCurrentWidget( contentUi.unsupportedTypePage );
  }

  QByteArray data = item.payloadData();
  contentUi.dataView->setPlainText( data );

  contentUi.id->setText( QString::number( item.id() ) );
  contentUi.remoteId->setText( item.remoteId() );
  contentUi.mimeType->setText( item.mimeType() );
  contentUi.revision->setText( QString::number( item.revision() ) );
  contentUi.size->setText( QString::number( item.size() ) );
  contentUi.modificationtime->setText( item.modificationTime().toString() + ( " UTC" ) );
  QStringList flags;
  foreach ( const Item::Flag &f, item.flags() )
    flags << QString::fromUtf8( f );
  contentUi.flags->setItems( flags );

  Attribute::List list = item.attributes();
  mAttrModel = new QStandardItemModel( list.count(), 2 );
  QStringList labels;
  labels << i18n( "Attribute" ) << i18n( "Value" );
  mAttrModel->setHorizontalHeaderLabels( labels );
  for ( int i = 0; i < list.count(); ++i ) {
    QModelIndex index = mAttrModel->index( i, 0 );
    Q_ASSERT( index.isValid() );
    mAttrModel->setData( index, QString::fromLatin1( list[i]->type() ) );
    index = mAttrModel->index( i, 1 );
    Q_ASSERT( index.isValid() );
    mAttrModel->setData( index, QString::fromLatin1( list[i]->serialized() ) );
    mAttrModel->itemFromIndex( index )->setFlags( Qt::ItemIsEditable | mAttrModel->flags( index ) );
  }
  contentUi.attrView->setModel( mAttrModel );

#ifdef NEPOMUK_FOUND
  if ( Settings::self()->nepomukEnabled() ) {
    Nepomuk::Resource res( item.url() );
    delete mNepomukModel;
    mNepomukModel = 0;
    if ( res.isValid() ) {
      QHash<QUrl, Nepomuk::Variant> props = res.properties();
      mNepomukModel = new QStandardItemModel( props.count(), 2, this );
      QStringList labels;
      labels << i18n( "Property" ) << i18n( "Value" );
      mNepomukModel->setHorizontalHeaderLabels( labels );
      int row = 0;
      for ( QHash<QUrl, Nepomuk::Variant>::ConstIterator it = props.constBegin(); it != props.constEnd(); ++it, ++row ) {
        QModelIndex index = mNepomukModel->index( row, 0 );
        Q_ASSERT( index.isValid() );
        mNepomukModel->setData( index, it.key().toString() );
        index = mNepomukModel->index( row, 1 );
        Q_ASSERT( index.isValid() );
        mNepomukModel->setData( index, it.value().toString() );
      }
      contentUi.nepomukView->setEnabled( true );
    } else {
      contentUi.nepomukView->setEnabled( false );
    }
    contentUi.nepomukView->setModel( mNepomukModel );
    contentUi.nepomukTab->setEnabled( true );
  } else {
    contentUi.nepomukTab->setEnabled( false );
  }
#endif

  if ( mMonitor )
    mMonitor->deleteLater(); // might be the one calling us
  mMonitor = new Monitor( this );
  mMonitor->setItemMonitored( item );
  mMonitor->itemFetchScope().fetchFullPayload();
  mMonitor->itemFetchScope().fetchAllAttributes();
  connect( mMonitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)), SLOT(setItem(Akonadi::Item)) );
}

void BrowserWidget::modelChanged()
{
  delete mItemModel;
  switch ( itemUi.modelBox->currentIndex() ) {
    case 1:
      mItemModel = new MessageModel( this );
      break;
    case 2:
      mItemModel = new KABCModel( this );
      break;
    case 3:
      mItemModel = new KCalModel( this );
      break;
    default:
      mItemModel = new ItemModel( this );
  }
  itemUi.itemView->setModel( mItemModel );
  if ( mCurrentCollection > 0 ) {
    const Collection col = mCollectionView->currentIndex().data( CollectionModel::CollectionRole ).value<Collection>();
    mItemModel->setCollection( col );
  }
  if ( mStdActionManager )
    mStdActionManager->setItemSelectionModel( itemUi.itemView->selectionModel() );
}

void BrowserWidget::save()
{
  Q_ASSERT( mAttrModel );

  const QByteArray data = contentUi.dataView->toPlainText().toUtf8();
  Item item = mCurrentItem;
  item.setRemoteId( contentUi.remoteId->text() );
  foreach ( const Item::Flag &f, mCurrentItem.flags() )
    item.clearFlag( f );
  foreach ( const QString &s, contentUi.flags->items() )
    item.setFlag( s.toUtf8() );
  item.setPayloadFromData( data );

  item.clearAttributes();
  for ( int i = 0; i < mAttrModel->rowCount(); ++i ) {
    const QModelIndex typeIndex = mAttrModel->index( i, 0 );
    Q_ASSERT( typeIndex.isValid() );
    const QModelIndex valueIndex = mAttrModel->index( i, 1 );
    Q_ASSERT( valueIndex.isValid() );
    Attribute* attr = AttributeFactory::createAttribute( mAttrModel->data( typeIndex ).toString().toLatin1() );
    Q_ASSERT( attr );
    attr->deserialize( mAttrModel->data( valueIndex ).toString().toLatin1() );
    item.addAttribute( attr );
  }

  ItemModifyJob *store = new ItemModifyJob( item, this );
  connect( store, SIGNAL(result(KJob*)), SLOT(saveResult(KJob*)) );
}

void BrowserWidget::saveResult(KJob * job)
{
  if ( job->error() ) {
    KMessageBox::error( this, i18n( "Failed to save changes: %1", job->errorString() ) );
  }
}

void BrowserWidget::addAttribute()
{
  if ( contentUi.attrName->text().isEmpty() )
    return;
  const int row = mAttrModel->rowCount();
  mAttrModel->insertRow( row );
  QModelIndex index = mAttrModel->index( row, 0 );
  Q_ASSERT( index.isValid() );
  mAttrModel->setData( index, contentUi.attrName->text() );
  contentUi.attrName->clear();
}

void BrowserWidget::delAttribute()
{
  QModelIndexList selection = contentUi.attrView->selectionModel()->selectedRows();
  if ( selection.count() != 1 )
    return;
  mAttrModel->removeRow( selection.first().row() );
}

void BrowserWidget::dumpToXml()
{
  const Collection root = mCollectionView->currentIndex().data( CollectionModel::CollectionRole ).value<Collection>();
  if ( !root.isValid() )
    return;
  const QString fileName = KFileDialog::getSaveFileName( KUrl(), "*.xml", this, i18n( "Select XML file" ) );
  if ( fileName.isEmpty() )
    return;
  XmlWriteJob *job = new XmlWriteJob( root, fileName, this );
  connect( job, SIGNAL(result(KJob*)), SLOT(dumpToXmlResult(KJob*)) );
}

void BrowserWidget::dumpToXmlResult( KJob* job )
{
  if ( job->error() )
    KMessageBox::error( this, job->errorString() );
}

#include "browserwidget.moc"
