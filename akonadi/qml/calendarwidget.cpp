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

#include "calendarwidget.h"

#include <QtDeclarative/QmlContext>
#include <QtDeclarative/QmlEngine>
#include <QtDeclarative/QmlComponent>
#include <QtDeclarative/QmlView>

#include <QBoxLayout>
#include <QSplitter>
#include <QListView>
#include <QApplication>

#include "fakegrouplistmodel.h"

#if 0
#include <akonadi/changerecorder.h>

#include <KABC/Addressee>
#include <Akonadi/KCal/IncidenceMimeTypeVisitor>

#include "kdebug.h"

#include <kselectionproxymodel.h>
#include <akonadi/entitytreeview.h>
#include <Akonadi/ItemFetchScope>
#include <akonadi/entitymimetypefiltermodel.h>
#include <KStandardDirs>
#include "eventsortproxymodel.h"
#include "eventgrouplistmodel.h"
#endif

CalendarWidget::CalendarWidget(QWidget* parent)
  : QWidget(parent)
{

  QHBoxLayout *mainLayout = new QHBoxLayout( this );
  QSplitter *splitter = new QSplitter(this);
  mainLayout->addWidget( splitter );

#if 0
  Akonadi::ChangeRecorder *changeRecorder = new Akonadi::ChangeRecorder();
  changeRecorder->setMimeTypeMonitored( Akonadi::IncidenceMimeTypeVisitor::eventMimeType() );
  changeRecorder->setCollectionMonitored( Akonadi::Collection::root() );
  changeRecorder->itemFetchScope().fetchFullPayload();

  Akonadi::EntityTreeModel *etm = new DeclarativeContactModel( changeRecorder );
  Akonadi::EntityTreeView *treeview = new Akonadi::EntityTreeView( splitter );

  Akonadi::EntityMimeTypeFilterModel *collectionFilter = new Akonadi::EntityMimeTypeFilterModel();
  collectionFilter->setHeaderGroup( Akonadi::EntityTreeModel::CollectionTreeHeaders );
  collectionFilter->setSourceModel( etm );
  collectionFilter->addMimeTypeInclusionFilter( Akonadi::Collection::mimeType() );

  treeview->setModel( collectionFilter );

  KSelectionProxyModel *selectionProxyModel = new KSelectionProxyModel( treeview->selectionModel() );
  selectionProxyModel->setSourceModel(etm);
  selectionProxyModel->setFilterBehavior( KSelectionProxyModel::ChildrenOfExactSelection );

  Akonadi::EntityMimeTypeFilterModel *itemFilter = new Akonadi::EntityMimeTypeFilterModel();
  itemFilter->setSourceModel( selectionProxyModel );
  itemFilter->addMimeTypeExclusionFilter( Akonadi::Collection::mimeType() );

  EventSortProxyModel *eventSortProxy = new EventSortProxyModel(this);
  eventSortProxy->setSourceModel( itemFilter );
#endif

  FakeEventGroupListModel *eventGroupModel = new FakeEventGroupListModel(this);

  QStringList monthsList;
  monthsList << "January"
             << "February"
             << "March"
             << "April"
             << "May"
             << "June"
             << "July"
             << "August"
             << "September"
             << "October"
             << "November"
             << "December";

  QStringList yearsList;
  for (int i = 2000; i <= 2038; ++i ) // From 2000 'til the end of the world.
    yearsList << QString::number( i );

  QString contactsQmlUrl =  qApp->applicationDirPath() + "/calendar.qml";
  QmlView *view = new QmlView( splitter );
  view->setUrl( contactsQmlUrl );
  QmlContext *c = view->engine()->rootContext();
  c->setContextProperty( "months_list", monthsList );
  c->setContextProperty( "years_list", yearsList );
  c->setContextProperty( "event_group_model", QVariant::fromValue( static_cast<QObject*>( eventGroupModel ) ) );
  view->setFocus();
  view->execute();
}
