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

#include "descendantpmwidget.h"

#include <QTreeView>
#include <QSplitter>

#include "dynamictreemodel.h"
#include "descendantentitiesproxymodel.h"
#include <QHBoxLayout>

using namespace Akonadi;

DescendantProxyModelWidget::DescendantProxyModelWidget(QWidget* parent): QWidget(parent)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  QSplitter *vSplitter = new QSplitter( this );
  layout->addWidget(vSplitter);


  m_rootModel = new DynamicTreeModel(this);

  QList<int> ancestorRows;

  ModelInsertCommand *ins;
  int max_runs = 4;
  for (int i = 0; i < max_runs; i++)
  {
    ins = new ModelInsertCommand(m_rootModel, this);
    ins->setAncestorRowNumbers(ancestorRows);
    ins->setStartRow(0);
    ins->setEndRow(4);
    ins->doCommand();
    ancestorRows << 2;
  }

  ancestorRows.clear();
  ancestorRows << 3;
  for (int i = 0; i < max_runs - 1; i++)
  {
    ins = new ModelInsertCommand(m_rootModel, this);
    ins->setAncestorRowNumbers(ancestorRows);
    ins->setStartRow(0);
    ins->setEndRow(4);
    ins->doCommand();
    ancestorRows << 3;
  }

  ModelDataChangeCommand *dataChCmd = new ModelDataChangeCommand(m_rootModel, this);
  dataChCmd->setStartRow(0);
  dataChCmd->setEndRow(4);
  dataChCmd->doCommand();

  QTreeView *treeview = new QTreeView( vSplitter );
  treeview->setModel(m_rootModel);
  treeview->setSelectionMode(QAbstractItemView::ExtendedSelection);

  DescendantEntitiesProxyModel *descProxyModel = new DescendantEntitiesProxyModel(this);
  descProxyModel->setSourceModel(m_rootModel);

  QTreeView *descView = new QTreeView( vSplitter );
  descView->setModel(descProxyModel);

  DescendantEntitiesProxyModel *descProxyModel2 = new DescendantEntitiesProxyModel(this);
  descProxyModel2->setSourceModel(m_rootModel);
  descProxyModel2->setDisplayAncestorData(true);

  QTreeView *descView2 = new QTreeView( vSplitter );
  descView2->setModel(descProxyModel2);

  setLayout(layout);

}



