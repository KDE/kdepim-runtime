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

#include "selectionpmwidget.h"

#include <QSplitter>
#include <QTreeView>
#include <QHBoxLayout>
#include "dynamictreemodel.h"
#include "selectionproxymodel.h"

using namespace Akonadi;

SelectionProxyWidget::SelectionProxyWidget(QWidget* parent): QWidget(parent)
{

  QHBoxLayout *layout = new QHBoxLayout(this);
  QSplitter *vSplitter = new QSplitter( this );
  QSplitter *hSplitter1 = new QSplitter ( Qt::Vertical, vSplitter );
  QSplitter *hSplitter2 = new QSplitter ( Qt::Vertical, vSplitter );
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

  QTreeView *treeview = new QTreeView( hSplitter1 );
  treeview->setModel(m_rootModel);
  treeview->setSelectionMode(QAbstractItemView::ExtendedSelection);

//   DescendantEntitiesProxyModel *descProxyModel = new DescendantEntitiesProxyModel(this);
//   descProxyModel->setSourceModel(m_rootModel);

//   QTreeView *descView = new QTreeView( hSplitter1 );
//   descView->setModel(descProxyModel);

  SelectionProxyModel *selProxyModel1 = new SelectionProxyModel(treeview->selectionModel(), this);
  selProxyModel1->setSourceModel(m_rootModel);

//   new ModelTest(selProxyModel, this);

  QTreeView *selView1 = new QTreeView( hSplitter2 );
  selView1->setModel(selProxyModel1);

  SelectionProxyModel *selProxyModel2 = new SelectionProxyModel(treeview->selectionModel(), this);
  selProxyModel2->setSourceModel(m_rootModel);
  selProxyModel2->setStartWithChildTrees(true);

  QTreeView *selView2 = new QTreeView( hSplitter1 );
  selView2->setModel(selProxyModel2);

  SelectionProxyModel *selProxyModel3 = new SelectionProxyModel(treeview->selectionModel(), this);
  selProxyModel3->setSourceModel(m_rootModel);
  selProxyModel3->setOmitDescendants(true);

  QTreeView *selView3 = new QTreeView( hSplitter2 );
  selView3->setModel(selProxyModel3);

  SelectionProxyModel *selProxyModel4 = new SelectionProxyModel(treeview->selectionModel(), this);
  selProxyModel4->setSourceModel(m_rootModel);
  selProxyModel4->setStartWithChildTrees(true);
  selProxyModel4->setOmitDescendants(true);

  QTreeView *selView4 = new QTreeView( hSplitter1 );
  selView4->setModel(selProxyModel4);

  SelectionProxyModel *selProxyModel5 = new SelectionProxyModel(treeview->selectionModel(), this);
  selProxyModel5->setSourceModel(m_rootModel);
  selProxyModel5->setStartWithChildTrees(true);
  selProxyModel5->setOmitDescendants(true);
  selProxyModel5->setIncludeAllSelected(true);

  QTreeView *selView5 = new QTreeView( hSplitter2 );
  selView5->setModel(selProxyModel5);

  setLayout(layout);

}
