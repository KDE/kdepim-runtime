/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#include <QtTest>
#include <QtCore>
#include <qtest_kde.h>
#include <qtestevent.h>

#include "dynamictreemodel.h"
#include "../modeltest.h"
#include "../descendantentitiesproxymodel.h"

// #include "fakemonitor.h"

#include <QTreeView>

#include <kdebug.h>

using namespace Akonadi;

class TestProxyModel : public QObject
{
  Q_OBJECT

private slots:
  void testInsertionChangeAndRemoval();
  void testSameParentUp();
  void testSameParentDown();
  void testDifferentParentUp();
  void testDifferentParentDown();
  void testDifferentParentSameLevel();
  void testInsertionWithDescendants();
};

void TestProxyModel::testInsertionChangeAndRemoval()
{
  DynamicTreeModel *model = new DynamicTreeModel(this);

  DescendantEntitiesProxyModel *proxy = new DescendantEntitiesProxyModel(this);
  proxy->setSourceModel(model);

  new ModelTest(proxy, this);

  QList<int> ancestorRows;

  ModelInsertCommand *ins;
  int max_runs = 2;
  for (int i = 0; i < max_runs; i++)
  {
    ins = new ModelInsertCommand(model, this);
    ins->setAncestorRowNumbers(ancestorRows);
    ins->setStartRow(0);
    ins->setEndRow(9);
    ins->doCommand();
    ancestorRows << 2;
  }

  ModelDataChangeCommand *mch;
  mch = new ModelDataChangeCommand(model, this);
  mch->setStartRow(0);
  mch->setEndRow(4);
  mch->doCommand();

  // Item 0-0
  // Item 1-0
  // Item 2-0
  // Item 3-0
  // Item 4-0

  // Item 0-0
  // Item 2-0
  // Item 3-0
  // Item 1-0
  // Item 4-0
//   2, 3 -1
//   1 2

  // Item 0-0
  // Item 3-0
  // Item 1-0
  // Item 2-0
  // Item 4-0
// 3 -2
// 1,2 +1



  ModelRemoveCommand *rem;
  rem = new ModelRemoveCommand(model, this);
  rem->setStartRow(2);
  rem->setEndRow(2);
  rem->doCommand();

  // If we get this far, modeltest didn't assert.
  QVERIFY(true);
}

void TestProxyModel::testSameParentDown()
{
  DynamicTreeModel *model = new DynamicTreeModel(this);

  DescendantEntitiesProxyModel *proxy = new DescendantEntitiesProxyModel(this);
  proxy->setSourceModel(model);

  new ModelTest(proxy, this);

  QList<int> ancestorRows;

  ModelInsertCommand *ins;
  int max_runs = 1;
  for (int i = 0; i < max_runs; i++)
  {
    ins = new ModelInsertCommand(model, this);
    ins->setAncestorRowNumbers(ancestorRows);
    ins->setStartRow(0);
    ins->setEndRow(9);
    ins->doCommand();
    ancestorRows << 2;
  }

  ModelMoveCommand *mmc;
  mmc = new ModelMoveCommand(model, this);
  mmc->setStartRow(3);
  mmc->setEndRow(5);
  mmc->setDestRow(8);
  mmc->doCommand();

  QVERIFY(true);
}

void TestProxyModel::testSameParentUp()
{
  DynamicTreeModel *model = new DynamicTreeModel(this);

  DescendantEntitiesProxyModel *proxy = new DescendantEntitiesProxyModel(this);
  proxy->setSourceModel(model);

  new ModelTest(proxy, this);

  QList<int> ancestorRows;

  ModelInsertCommand *ins;
  ins = new ModelInsertCommand(model, this);
  ins->setAncestorRowNumbers(ancestorRows);
  ins->setStartRow(0);
  ins->setEndRow(9);
  ins->doCommand();

  ModelMoveCommand *mmc;

  mmc = new ModelMoveCommand(model, this);
  mmc->setStartRow(8);
  mmc->setEndRow(8);
  mmc->setDestRow(1);
  mmc->doCommand();

  mmc = new ModelMoveCommand(model, this);
  mmc->setStartRow(7);
  mmc->setEndRow(8);
  mmc->setDestRow(1);
  mmc->doCommand();

  mmc = new ModelMoveCommand(model, this);
  mmc->setStartRow(6);
  mmc->setEndRow(8);
  mmc->setDestRow(1);
  mmc->doCommand();

  mmc = new ModelMoveCommand(model, this);
  mmc->setStartRow(5);
  mmc->setEndRow(8);
  mmc->setDestRow(1);
  mmc->doCommand();

  mmc = new ModelMoveCommand(model, this);
  mmc->setStartRow(4);
  mmc->setEndRow(8);
  mmc->setDestRow(1);
  mmc->doCommand();

  QVERIFY(true);
}

void TestProxyModel::testDifferentParentUp()
{
  DynamicTreeModel *model = new DynamicTreeModel(this);

  DescendantEntitiesProxyModel *proxy = new DescendantEntitiesProxyModel(this);
  proxy->setSourceModel(model);

  new ModelTest(proxy, this);

  QList<int> ancestorRows;

  ModelInsertCommand *ins;
  int max_runs = 2;
  for (int i = 0; i < max_runs; i++)
  {
    ins = new ModelInsertCommand(model, this);
    ins->setAncestorRowNumbers(ancestorRows);
    ins->setStartRow(0);
    ins->setEndRow(9);
    ins->doCommand();
    ancestorRows << 2;
  }

  ancestorRows.clear();
  ancestorRows << 2;

  ModelMoveCommand *mmc;
  mmc = new ModelMoveCommand(model, this);
  mmc->setDestAncestors(ancestorRows);
  mmc->setStartRow(7);
  mmc->setEndRow(8);
  mmc->setDestRow(1);
  mmc->doCommand();

  QVERIFY(true);
}

void TestProxyModel::testDifferentParentDown()
{
  DynamicTreeModel *model = new DynamicTreeModel(this);

  DescendantEntitiesProxyModel *proxy = new DescendantEntitiesProxyModel(this);
  proxy->setSourceModel(model);

  new ModelTest(proxy, this);

  QList<int> ancestorRows;

  ModelInsertCommand *ins;
  int max_runs = 2;
  for (int i = 0; i < max_runs; i++)
  {
    ins = new ModelInsertCommand(model, this);
    ins->setAncestorRowNumbers(ancestorRows);
    ins->setStartRow(0);
    ins->setEndRow(9);
    ins->doCommand();
    ancestorRows << 2;
  }

  ancestorRows.clear();
  ancestorRows << 2;

  ModelMoveCommand *mmc;
  mmc = new ModelMoveCommand(model, this);
  mmc->setDestAncestors(ancestorRows);
  mmc->setStartRow(6);
  mmc->setEndRow(7);
  mmc->setDestRow(9);
  mmc->doCommand();

  QVERIFY(true);
}

void TestProxyModel::testDifferentParentSameLevel()
{
  DynamicTreeModel *model = new DynamicTreeModel(this);

  DescendantEntitiesProxyModel *proxy = new DescendantEntitiesProxyModel(this);
  proxy->setSourceModel(model);

  new ModelTest(proxy, this);

  QList<int> ancestorRows;

  ModelInsertCommand *ins;
  int max_runs = 2;
  for (int i = 0; i < max_runs; i++)
  {
    ins = new ModelInsertCommand(model, this);
    ins->setAncestorRowNumbers(ancestorRows);
    ins->setStartRow(0);
    ins->setEndRow(9);
    ins->doCommand();
    ancestorRows << 2;
  }

  ancestorRows.clear();
  ancestorRows << 2;

  ModelMoveCommand *mmc;

  mmc = new ModelMoveCommand(model, this);
  mmc->setDestAncestors(ancestorRows);
  mmc->setStartRow(6);
  mmc->setEndRow(7);
  mmc->setDestRow(6);
  mmc->doCommand();

  QVERIFY(true);

}

void TestProxyModel::testInsertionWithDescendants()
{
  DynamicTreeModel *model = new DynamicTreeModel(this);

  DescendantEntitiesProxyModel *proxy = new DescendantEntitiesProxyModel(this);
  proxy->setSourceModel(model);

  new ModelTest(proxy, this);

  // First insert 4 items to the root.
  ModelInsertCommand *ins = new ModelInsertCommand(model, this);
  ins->setStartRow(0);
  ins->setEndRow(3);
  ins->doCommand();

  ModelInsertWithDescendantsCommand *insDesc = new ModelInsertWithDescendantsCommand(model, this);
  QList<QPair<int, int> > descs;
  QPair<int, int> pair;
  pair.first = 1;   // On the first row,
  pair.second = 4;  // insert 4 items.
  descs << pair;
  pair.first = 2;   // Make the 6th new item
  pair.second = 5;  // have 5 descendants itself.
  descs << pair;
  insDesc->setNumDescendants(descs);
  insDesc->doCommand();

  QVERIFY(true);
}


QTEST_KDEMAIN(TestProxyModel, GUI)
#include "descendantentitiesproxymodeltest.moc"
