/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

#include "mainwindow.h"

#include <QHBoxLayout>
#include <QApplication>
#include <QTreeView>
#include <QSplitter>
#include <QDeclarativeView>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>

#include "dynamictreemodel.h"
#include "dynamictreewidget.h"

#include "breadcrumbnavigationcontext.h"
#include <qcolumnview.h>
#include <QFile>
#include "kresettingproxymodel.h"
#include "qmllistselectionmodel.h"
#include "kselectionproxymodel.h"
#include "checkableitemproxymodel.h"

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags f )
  : QWidget(parent, f)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  QSplitter *splitter = new QSplitter;
  layout->addWidget(splitter);

  m_treeModel = new DynamicTreeModel(this);

  DynamicTreeWidget *widget = new DynamicTreeWidget(m_treeModel, splitter);

  QString initialTree;
  if (qApp->arguments().size() == 2)
  {
    QString filename = qApp->arguments().at(1);
    QFile f(filename);
    if (f.open(QFile::ReadOnly))
    {
      initialTree = f.readAll();
    }
    f.close();
  }
  if (initialTree.isEmpty()){
    initialTree =
    "- 1"
    "- - 1"
    "- - 1"
    "- - - 1"
    "- - - - 1"
    "- - - 1"
    "- - - 1"
    "- - - - 1"
    "- - - - 1"
    "- - - - - 1"
    "- - - - - - 1"
    "- - - - - 1"
    "- - - - - -1"
    "- - - - - - 1"
    "- - - - - - - 1"
    "- - - - - - - 1"
    "- - - - - - 1"
    "- - - - - 1"
    "- - - - - 1"
    "- - - - 1"
    "- - - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - - 1"
    "- - - - 1"
    "- - - - - 1"
    "- - - - - - 1"
    "- - - - - 1"
    "- - - - - -1"
    "- - - - - - 1"
    "- - - - - - - 1"
    "- - - - - - - 1"
    "- - - - - - 1"
    "- - - - - 1"
    "- - - - - 1"
    "- - - - 1"
    "- - - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- 1"
    "- - 1"
    "- - 1"
    "- - 1"
    "- - - 1"
    "- - - 1"
    "- - 1"
    "- 1"
    "- - 1"
    "- - - 1"
    "- - - 1"
    "- - 1"
    "- - 1"
    "- - 1"
    "- 1";
  }

  widget->setInitialTree(initialTree);
  m_declarativeView = new QDeclarativeView(splitter);

  QDeclarativeContext *context = m_declarativeView->engine()->rootContext();

  m_bnf = new KBreadcrumbNavigationFactory(this);
  m_bnf->setBreadcrumbDepth(1);
  m_bnf->createCheckableBreadcrumbContext( m_treeModel, this );

  context->setContextProperty( "_breadcrumbNavigationFactory", m_bnf );

//   widget->treeView()->setSelectionModel( m_bnf->selectionModel() );

  context->setContextProperty( "application", QVariant::fromValue( static_cast<QObject*>( this ) ) );

  m_declarativeView->setResizeMode( QDeclarativeView::SizeRootObjectToView );
  m_declarativeView->setSource( QUrl( "./mainview.qml" ) );

  splitter->setSizes(QList<int>() << 1 << 1);

}

bool MainWindow::childCollectionHasChildren( int row )
{
  return m_bnf->childCollectionHasChildren(row);
}

int MainWindow::selectedCollectionRow()
{
  const QModelIndexList list = m_bnf->selectionModel()->selectedRows();
  if (list.size() != 1)
    return -1;
  return list.first().row();
}



#include "mainwindow.moc"
