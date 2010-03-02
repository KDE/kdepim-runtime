/*
* This file is part of Akonadi
*
* Copyright 2010 Stephen Kelly <steveire@gmail.com>
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

#include "contactswidget.h"

#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeComponent>
#include <QtDeclarative/QDeclarativeView>

#include <QBoxLayout>
#include <QSplitter>
#include <QListView>
#include <QTreeView>
#include <QApplication>
#include <QDebug>

#if 0
#include <KABC/Addressee>

#include "declarativecontactmodel.h"
#include <akonadi/changerecorder.h>
#include <kselectionproxymodel.h>
#include <akonadi/entitytreeview.h>
#include <Akonadi/ItemFetchScope>
#include <akonadi/entitymimetypefiltermodel.h>
#include <KStandardDirs>
#endif

#include "fakeentitytreemodel.h"

#include "qtselectionproxymodel.h"


ContactsWidget::ContactsWidget(QWidget* parent)
  : QWidget(parent)
{

  QHBoxLayout *mainLayout = new QHBoxLayout( this );
  QSplitter *splitter = new QSplitter(this);
  mainLayout->addWidget( splitter );

  QStringList args = qApp->arguments();

  int numCollections = 10;
  int numItems = 10;
  int argsSize = args.size();
  if (argsSize == 4)
  {
    numCollections = args.value(2).toInt();
    numItems = args.value(3).toInt();
  }

  FakeEntityTreeModel *fakeModel = new FakeEntityTreeModel(numCollections, numItems, FakeEntityTreeModel::ContactsData);

  QListView *fakeView = new QListView( splitter );
  fakeView->setModel(fakeModel);

#if 0
  Akonadi::ChangeRecorder *changeRecorder = new Akonadi::ChangeRecorder();
  changeRecorder->setMimeTypeMonitored( KABC::Addressee::mimeType() );
  changeRecorder->setCollectionMonitored( Akonadi::Collection::root() );
  changeRecorder->itemFetchScope().fetchFullPayload();

  Akonadi::EntityTreeModel *etm = new DeclarativeContactModel( changeRecorder );
  Akonadi::EntityTreeView *treeview = new Akonadi::EntityTreeView( splitter );

  Akonadi::EntityMimeTypeFilterModel *collectionFilter = new Akonadi::EntityMimeTypeFilterModel();
  collectionFilter->setHeaderGroup( Akonadi::EntityTreeModel::CollectionTreeHeaders );
  collectionFilter->setObjectName( "collectionFilter" );
  collectionFilter->setSourceModel( etm );
  collectionFilter->addMimeTypeInclusionFilter( Akonadi::Collection::mimeType() );

  treeview->setModel( collectionFilter );

  KSelectionProxyModel *selectionProxyModel = new KSelectionProxyModel( treeview->selectionModel() );
  selectionProxyModel->setSourceModel(etm);
  selectionProxyModel->setFilterBehavior( KSelectionProxyModel::ChildrenOfExactSelection );

  Akonadi::EntityMimeTypeFilterModel *itemFilter = new Akonadi::EntityMimeTypeFilterModel();
  itemFilter->setSourceModel( selectionProxyModel );
  itemFilter->addMimeTypeExclusionFilter( Akonadi::Collection::mimeType() );
#endif

  QtSelectionProxyModel *selectionProxyModel = new QtSelectionProxyModel( fakeView->selectionModel() );
  selectionProxyModel->setSourceModel( fakeModel );
  selectionProxyModel->setFilterBehavior( QtSelectionProxyModel::ChildrenOfExactSelection );

  QString contactsQmlUrl = qApp->applicationDirPath() + "/contacts.qml";
  QDeclarativeView *view = new QDeclarativeView( splitter );
  view->setSource( contactsQmlUrl );
//   view->engine()->rootContext()->setContextProperty( "entity_tree_model", QVariant::fromValue( static_cast<QObject*>( itemFilter ) ) );
  view->engine()->rootContext()->setContextProperty( "entity_tree_model", QVariant::fromValue( static_cast<QObject*>( selectionProxyModel ) ) );

}


