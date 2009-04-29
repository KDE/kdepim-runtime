/*
 * This file is part of the proxy model test suite.
 *
 * Copyright 2009  Stephen Kelly <steveire@gmail.com>
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

#include "mainwindow.h"

#include <QSplitter>

#include "../tests/dynamictreemodel.h"
#include "../entitytreeview.h"

#include "../descendantentitiesproxymodel.h"
#include "../selectionproxymodel.h"

using namespace Akonadi;

MainWindow::MainWindow() : KXmlGuiWindow()
{
  QSplitter *vSplitter = new QSplitter( this );

  m_rootModel = new DynamicTreeModel(this);

  QList<int> ancestorRows;

  ModelInsertCommand *ins;
  int max_runs = 5;
  for (int i = 0; i < max_runs; i++)
  {
    ins = new ModelInsertCommand(m_rootModel, this);
    ins->setAncestorRowNumbers(ancestorRows);
    ins->setStartRow(0);
    ins->setEndRow(9);
    ins->doCommand();
    ancestorRows << 2;
  }

  EntityTreeView *treeview = new EntityTreeView( vSplitter );
  treeview->setModel(m_rootModel);
  treeview->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QSplitter *hSplitter1 = new QSplitter ( Qt::Vertical, vSplitter );

  DescendantEntitiesProxyModel *descProxyModel = new DescendantEntitiesProxyModel(this);
  descProxyModel->setSourceModel(m_rootModel);

  EntityTreeView *descView = new EntityTreeView( hSplitter1 );
  descView->setModel(descProxyModel);

  QSplitter *vSplitter2 = new QSplitter ( Qt::Horizontal, hSplitter1 );

  SelectionProxyModel *selProxyModel = new SelectionProxyModel(treeview->selectionModel(), this);
  selProxyModel->setSourceModel(m_rootModel);

  EntityTreeView *selView = new EntityTreeView( vSplitter2 );
  selView->setModel(selProxyModel);

  DescendantEntitiesProxyModel *descProxyModel2 = new DescendantEntitiesProxyModel(this);
  descProxyModel2->setSourceModel(selProxyModel);

  EntityTreeView *descView2 = new EntityTreeView( vSplitter2 );
  descView2->setModel(descProxyModel2);

  setCentralWidget( vSplitter );
}


MainWindow::~MainWindow()
{
}


