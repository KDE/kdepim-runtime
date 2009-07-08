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

#include "proxymodeltest.h"

#include <QSignalSpy>

#include "dynamictreemodel.h"
#include "../modeltest.h"
#include <QItemSelectionModel>

typedef QList<ModelChangeCommand*> CommandList;

Q_DECLARE_METATYPE( CommandList )

ModelSpy::ModelSpy(QObject *parent)
    : QObject(parent), QList<QVariantList>()
{
  qRegisterMetaType<QModelIndex>("QModelIndex");
  qRegisterMetaType<IndexFinder>("IndexFinder");
}

void ModelSpy::setModel(AbstractProxyModel *model)
{
  Q_ASSERT(model);
  m_model = model;

  connect(m_model, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
          SLOT(rowsAboutToBeInserted(const QModelIndex &, int, int)));
  connect(m_model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
          SLOT(rowsInserted(const QModelIndex &, int, int)));
  connect(m_model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
          SLOT(rowsAboutToBeRemoved(const QModelIndex &, int, int)));
  connect(m_model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
          SLOT(rowsRemoved(const QModelIndex &, int, int)));
  connect(m_model, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int,const QModelIndex &, int)),
          SLOT(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  connect(m_model, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
          SLOT(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));

  connect(m_model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
          SLOT(dataChanged(const QModelIndex &, const QModelIndex &)));

}

void ModelSpy::rowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
  append(QVariantList() << RowsAboutToBeInserted << QVariant::fromValue(parent) << start << end);
}

void ModelSpy::rowsInserted(const QModelIndex &parent, int start, int end)
{
  append(QVariantList() << RowsInserted << QVariant::fromValue(parent) << start << end);
}

void ModelSpy::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
  append(QVariantList() << RowsAboutToBeRemoved << QVariant::fromValue(parent) << start << end);
}

void ModelSpy::rowsRemoved(const QModelIndex &parent, int start, int end)
{
  append(QVariantList() << RowsRemoved << QVariant::fromValue(parent) << start << end);
}

void ModelSpy::rowsAboutToBeMoved(const QModelIndex &srcParent, int start, int end, const QModelIndex &destParent, int destStart)
{
  append(QVariantList() << RowsAboutToBeMoved << QVariant::fromValue(srcParent) << start << end << QVariant::fromValue(destParent) << destStart);
}

void ModelSpy::rowsMoved(const QModelIndex &srcParent, int start, int end, const QModelIndex &destParent, int destStart)
{
  append(QVariantList() << RowsMoved << QVariant::fromValue(srcParent) << start << end << QVariant::fromValue(destParent) << destStart);
}

void ModelSpy::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
  append(QVariantList() << DataChanged << QVariant::fromValue(topLeft) << QVariant::fromValue(bottomRight));
}


ProxyModelTest::ProxyModelTest(QObject *parent)
: QObject(parent), m_model(new DynamicTreeModel(this)), m_proxyModel(0), m_modelSpy(new ModelSpy(this))
{
}

void ProxyModelTest::initTestCase()
{
}

DynamicTreeModel* ProxyModelTest::sourceModel()
{
  return m_model;
}


QVariantList ProxyModelTest::getSignal(SignalType type, IndexFinder parentFinder, int start, int end)
{
  return QVariantList() << type << QVariant::fromValue(parentFinder) << start << end;
}

void ProxyModelTest::signalInsertion(const QString &name, IndexFinder parentFinder, int startRow, int rowsAffected, int rowCount)
{
  QVERIFY(startRow >= 0 && rowsAffected > 0);
  QList<QVariantList> signalList;
  signalList << getSignal(RowsAboutToBeInserted, parentFinder, startRow, startRow + rowsAffected - 1 );
  signalList << getSignal(RowsInserted, parentFinder, startRow, startRow + rowsAffected - 1 );

  QList<PersistentIndexChange> persistentList;
  if (rowCount > 0)
    persistentList << getChange(parentFinder, startRow, rowCount - 1, rowsAffected);

  setExpected(name, signalList, persistentList);
}

void ProxyModelTest::signalRemoval(const QString &name, IndexFinder parentFinder, int startRow, int rowsAffected, int rowCount)
{
  QVERIFY(startRow >= 0 && rowsAffected > 0);
  QList<QVariantList> signalList;
  signalList << getSignal(RowsAboutToBeRemoved, parentFinder, startRow, startRow + rowsAffected - 1 );
  signalList << getSignal(RowsRemoved, parentFinder, startRow, startRow + rowsAffected - 1 );

  QList<PersistentIndexChange> persistentList;
  if (rowCount > 0)
  {
    persistentList << getChange(parentFinder, startRow, startRow + rowsAffected - 1, -1, true);
    persistentList << getChange(parentFinder, startRow + rowsAffected, rowCount - 1, rowsAffected * -1 );
  }

  setExpected(name, signalList, persistentList);
}

void ProxyModelTest::signalDataChange(const QString &name, IndexFinder topLeft, IndexFinder bottomRight)
{
  QList<QVariantList> signalList;
  QVariantList signal;
  signal << DataChanged << QVariant::fromValue(topLeft) << QVariant::fromValue(bottomRight);
  signalList << signal;
  setExpected(name, signalList);
}

void ProxyModelTest::noSignal(const QString &name)
{
  QList<PersistentIndexChange> persistentList;
  QList<QVariantList> signalList;
  signalList << (QVariantList() << NoSignal);
  signalList << (QVariantList() << NoSignal);

  setExpected(name, signalList, persistentList);
}

PersistentIndexChange ProxyModelTest::getChange(IndexFinder parentFinder, int start, int end, int difference, bool toInvalid)
{
  PersistentIndexChange change;
  change.parentFinder = parentFinder;
  change.startRow = start;
  change.endRow = end;
  change.difference = difference;
  change.toInvalid = toInvalid;
  return change;
}

void ProxyModelTest::handleSignal(QVariantList expected)
{
  int signalType = expected.takeAt(0).toInt();
  if (NoSignal == signalType)
    return;

  QVariantList result = getResultSignal();
  QCOMPARE(result.takeAt(0).toInt(), signalType);
  // Check that the signal we expected to recieve was emitted exactly.
  switch (signalType)
  {
  case RowsAboutToBeInserted:
  case RowsInserted:
  case RowsAboutToBeRemoved:
  case RowsRemoved:
  {
    QVERIFY( expected.size() == 3 );
    IndexFinder parentFinder = qvariant_cast<IndexFinder>(expected.at(0));
    QModelIndex parent = parentFinder.getIndex();
    QCOMPARE(qvariant_cast<QModelIndex>(result.at(0)), parent );
    QCOMPARE(result.at(1), expected.at(1) );
    QCOMPARE(result.at(2), expected.at(2) );
    break;
  }
  case RowsAboutToBeMoved:
  case RowsMoved:
  {
    QVERIFY( expected.size() == 5 );
    IndexFinder scrParentFinder = qvariant_cast<IndexFinder>(expected.at(0));
    QModelIndex srcParent = scrParentFinder.getIndex();
    QCOMPARE(qvariant_cast<QModelIndex>(result.at(0)), srcParent );
    QCOMPARE(result.at(1), expected.at(1) );
    QCOMPARE(result.at(2), expected.at(2) );
    IndexFinder destParentFinder = qvariant_cast<IndexFinder>(expected.at(3));
    QModelIndex destParent = destParentFinder.getIndex();
    QCOMPARE(qvariant_cast<QModelIndex>(result.at(3)), destParent );
    QCOMPARE(result.at(4), expected.at(4) );
  }
  case DataChanged:
  {
    QVERIFY( expected.size() == 2 );
    IndexFinder topLeftFinder = qvariant_cast<IndexFinder>(expected.at(0));
    QModelIndex topLeft = topLeftFinder.getIndex();
    IndexFinder bottomRightFinder = qvariant_cast<IndexFinder>(expected.at(1));
    QModelIndex bottomRight = bottomRightFinder.getIndex();

    QVERIFY(topLeft.isValid() && bottomRight.isValid());

    QCOMPARE(qvariant_cast<QModelIndex>(result.at(0)), topLeft );
    QCOMPARE(qvariant_cast<QModelIndex>(result.at(1)), bottomRight );
  }

  }
}

QVariantList ProxyModelTest::getResultSignal()
{
  return m_modelSpy->takeAt(0);
}

void ProxyModelTest::setProxyModel(AbstractProxyModel *proxyModel)
{
  m_proxyModel = proxyModel;
  m_proxyModel->setSourceModel(m_model);
  m_modelSpy->setModel(m_proxyModel);
}

void ProxyModelTest::setExpected(const QString &name, const QList<QVariantList> &list, const QList<PersistentIndexChange> &persistentChanges)
{
  m_expectedSignals.insert(name, list);
  m_persistentChanges.insert(name, persistentChanges);
}

QModelIndexList ProxyModelTest::getDescendantIndexes(const QModelIndex &parent)
{
  QModelIndexList list;
  const int column = 0;
  for(int row = 0; row < m_proxyModel->rowCount(parent); ++row)
  {
    QModelIndex idx = m_proxyModel->index(row, column, parent);
    list << idx;
    list << getDescendantIndexes(idx);
  }
  return list;
}

QList< QPersistentModelIndex > ProxyModelTest::toPersistent(QModelIndexList list)
{
  QList<QPersistentModelIndex > persistentList;
  foreach(QModelIndex idx, list)
  {
    persistentList << QPersistentModelIndex(idx);
  }
  return persistentList;
}


QModelIndexList ProxyModelTest::getUnchangedIndexes(const QModelIndex &parent, QList<QItemSelectionRange> ignoredRanges)
{
  QModelIndexList list;
  int rowCount = m_proxyModel->rowCount(parent);
  for (int row = 0; row < rowCount; )
  {
    int column = 0;
    QModelIndex idx = m_proxyModel->index( row, column, parent);
    bool found = false;
    foreach(QItemSelectionRange range, ignoredRanges)
    {
      if (range.topLeft().parent() == parent &&  range.topLeft().row() == idx.row())
      {
        row = range.bottomRight().row() + 1;
        found = true;
        break;
      }
    }
    if (!found)
    {
      for (column = 0; column < m_proxyModel->columnCount(); ++column )
        list << m_proxyModel->index( row, column, parent);
      list << getUnchangedIndexes(idx, ignoredRanges);
      ++row;
    }
  }
  return list;
}


void ProxyModelTest::doTest()
{
  QFETCH( CommandList, commandList );

  const char *currentTag = QTest::currentDataTag();

  QVERIFY(currentTag != 0);
  QVERIFY(m_expectedSignals.contains(currentTag));
  QVERIFY(m_persistentChanges.contains(currentTag));

  // The signals we expect to recieve to pass this test.
  QList<QVariantList> signalList = m_expectedSignals.value(currentTag);

  // The persistent indexes that we expect to be changed (and by how much)
  QList<PersistentIndexChange> changeList = m_persistentChanges.value(currentTag);

  QList<QPersistentModelIndex> persistentIndexes;

  const int columnCount = m_model->columnCount();
  QMutableListIterator<PersistentIndexChange> it(changeList);

  // The indexes are defined by the test are described with IndexFinder before anything in the model exists.
  // Now that the indexes should exist, resolve them in the change objects.
  QList<QItemSelectionRange> changedRanges;
  while (it.hasNext())
  {
    PersistentIndexChange change = it.next();
    QModelIndex parent = change.parentFinder.getIndex();

    QVERIFY(change.startRow >= 0);
    QVERIFY(change.endRow < m_proxyModel->rowCount(parent));

    QModelIndex topLeft = m_proxyModel->index( change.startRow, 0, parent );
    QModelIndex bottomRight = m_proxyModel->index( change.endRow, columnCount - 1, parent );

    // We store the changed ranges so that we know which ranges should not be changed
    changedRanges << QItemSelectionRange(topLeft, bottomRight);

    // Store the inital state of the indexes in the model which we expect to change.
    for (int row = change.startRow; row <= change.endRow; ++row )
    {
      for (int column = 0; column < columnCount; ++column)
      {
        QModelIndex idx = m_proxyModel->index(row, column, parent);
        QVERIFY(idx.isValid());
        change.indexes << idx;
        change.persistentIndexes << QPersistentModelIndex(idx);
      }

      // Also store the descendants of changed indexes so that we can verify the effect on them
      QModelIndex idx = m_proxyModel->index(row, 0, parent);
      QModelIndexList descs = getDescendantIndexes(idx);
      change.descendantIndexes << descs;
      change.persistentDescendantIndexes << toPersistent(descs);
    }
    it.setValue(change);
  }

  // Any indexes outside of the ranges we expect to be changed are stored
  // so that we can later verify that they remain unchanged.
  QModelIndexList unchangedIndexes = getUnchangedIndexes(QModelIndex(), changedRanges);

  QList<QPersistentModelIndex> unchangedPersistentIndexes = toPersistent(unchangedIndexes);

  // Run the test.
  foreach(ModelChangeCommand *command, commandList)
  {
    command->doCommand();
  }

  while (!signalList.isEmpty())
  {
    // Process each signal we recieved as a result of running the test.
    QVariantList expected = signalList.takeAt(0);
    handleSignal(expected);
  }
  // Make sure we didn't get any signals we didn't expect.
  QVERIFY(m_modelSpy->isEmpty());


  // Persistent indexes should change by the amount described in change objects.
  foreach (PersistentIndexChange change, changeList)
  {
    for (int i = 0; i < change.indexes.size(); i++)
    {
      QModelIndex idx = change.indexes.at(i);
      QPersistentModelIndex persistentIndex = change.persistentIndexes.at(i);

      // Persistent indexes go to an invalid state if they are removed from the model.
      if (change.toInvalid)
      {
        QVERIFY(!persistentIndex.isValid());
        continue;
      }
      QCOMPARE(idx.row() + change.difference, persistentIndex.row());
      QCOMPARE(idx.column(), persistentIndex.column());
      QCOMPARE(idx.parent(), persistentIndex.parent());
    }

    for (int i = 0; i < change.descendantIndexes.size(); i++)
    {
      QModelIndex idx = change.descendantIndexes.at(i);
      QPersistentModelIndex persistentIndex = change.persistentDescendantIndexes.at(i);

      // The descendant indexes of indexes which were removed should now also be invalid.
      if (change.toInvalid)
      {
        QVERIFY(!persistentIndex.isValid());
        continue;
      }
      // Otherwise they should be unchanged.
      QCOMPARE(idx.row(), persistentIndex.row());
      QCOMPARE(idx.column(), persistentIndex.column());
      QCOMPARE(idx.parent(), persistentIndex.parent());
    }
  }
  // Indexes unaffected by the signals should be unchanged.
  for (int i = 0; i < unchangedIndexes.size(); ++i)
  {
    QModelIndex unchangedIdx = unchangedIndexes.at(i);
    QPersistentModelIndex persistentIndex = unchangedPersistentIndexes.at(i);
    QCOMPARE(unchangedIdx.row(), persistentIndex.row());
    QCOMPARE(unchangedIdx.column(), persistentIndex.column());
    QCOMPARE(unchangedIdx.parent(), persistentIndex.parent());
  }
}


void ProxyModelTest::testInsertAndRemove_data()
{
  QTest::addColumn<CommandList>("commandList");

  CommandList commandList;

  // Insert a single item at the top.
  ModelInsertCommand *ins;
  ins = new ModelInsertCommand(sourceModel(), this);
  ins->setStartRow(0);
  ins->setEndRow(0);
  ins->doCommand();

  QTest::newRow("insert01") << commandList;
  commandList.clear();

  // Give the top level item 10 children.
  ins = new ModelInsertCommand(m_model, this);
  ins->setAncestorRowNumbers(QList<int>() << 0 );
  ins->setStartRow(0);
  ins->setEndRow(9);

  commandList << ins;

  QTest::newRow("insert02") << commandList;
  commandList.clear();

  // Give the top level item 10 'older' siblings.
  ins = new ModelInsertCommand(m_model, this);
  ins->setStartRow(0);
  ins->setEndRow(9);

  commandList << ins;

  QTest::newRow("insert03") << commandList;
  commandList.clear();

  // Give the top level item 10 'younger' siblings.
  ins = new ModelInsertCommand(m_model, this);
  ins->setStartRow(11);
  ins->setEndRow(20);

  commandList << ins;

  QTest::newRow("insert04") << commandList;
  commandList.clear();

  // Add more children to the top level item.
  // First 'older'
  ins = new ModelInsertCommand(m_model, this);
  ins->setAncestorRowNumbers(QList<int>() << 10 );
  ins->setStartRow(0);
  ins->setEndRow(9);

  commandList << ins;

  QTest::newRow("insert05") << commandList;
  commandList.clear();

  // Then younger
  ins = new ModelInsertCommand(m_model, this);
  ins->setAncestorRowNumbers(QList<int>() << 10 );
  ins->setStartRow(20);
  ins->setEndRow(29);

  commandList << ins;

  QTest::newRow("insert06") << commandList;
  commandList.clear();

  // Then somewhere in the middle.
  ins = new ModelInsertCommand(m_model, this);
  ins->setAncestorRowNumbers(QList<int>() << 10 );
  ins->setStartRow(10);
  ins->setEndRow(19);

  commandList << ins;

  QTest::newRow("insert07") << commandList;
  commandList.clear();

  // Add some more items for removing later.
  ins = new ModelInsertCommand(m_model, this);
  ins->setAncestorRowNumbers(QList<int>() << 10 << 5 );
  ins->setStartRow(0);
  ins->setEndRow(9);
  commandList << ins;
  ins = new ModelInsertCommand(m_model, this);
  ins->setAncestorRowNumbers(QList<int>() << 10 << 5 << 5 );
  ins->setStartRow(0);
  ins->setEndRow(9);
  commandList << ins;
  ins = new ModelInsertCommand(m_model, this);
  ins->setAncestorRowNumbers(QList<int>() << 10 << 5 << 5 << 5 );
  ins->setStartRow(0);
  ins->setEndRow(9);
  commandList << ins;
  ins = new ModelInsertCommand(m_model, this);
  ins->setAncestorRowNumbers(QList<int>() << 10 << 6 );
  ins->setStartRow(0);
  ins->setEndRow(9);
  commandList << ins;
  ins = new ModelInsertCommand(m_model, this);
  ins->setAncestorRowNumbers(QList<int>() << 10 << 7 );
  ins->setStartRow(0);
  ins->setEndRow(9);
  commandList << ins;

  QTest::newRow("insert08") << commandList;
  commandList.clear();

  // Insert a tree of items in one go.
  ModelInsertWithDescendantsCommand *insWithDescs = new ModelInsertWithDescendantsCommand(m_model, this);
  insWithDescs->setStartRow(2);
  insWithDescs->setAncestorRowNumbers(QList<int>() << 10 );
  QList<ModelInsertWithDescendantsCommand::InsertFragment> fragments;
  ModelInsertWithDescendantsCommand::InsertFragment fragment;

  ModelInsertWithDescendantsCommand::InsertFragment subFragment;
  subFragment.numRows = 10;

  ModelInsertWithDescendantsCommand::InsertFragment subSubFragment;
  subSubFragment.numRows = 10;
  subFragment.subfragments.insert(4, subSubFragment);

  fragment.numRows = 10;
  fragment.subfragments.insert(5, subFragment);
  fragment.subfragments.insert(2, subFragment);
  fragments << fragment;
  insWithDescs->setFragments(fragments );
  commandList << insWithDescs;

  QTest::newRow("insert09") << commandList;
  commandList.clear();

  ModelDataChangeCommand *dataChange = new ModelDataChangeCommand(m_model, this);

  dataChange->setAncestorRowNumbers(QList<int>() << 10 );
  dataChange->setStartRow(0);
  dataChange->setEndRow(0);

  commandList << dataChange;

  QTest::newRow("change01") << commandList;
  commandList.clear();

  dataChange = new ModelDataChangeCommand(m_model, this);
  dataChange->setAncestorRowNumbers(QList<int>() << 10);
  dataChange->setStartRow(4);
  dataChange->setEndRow(7);

  commandList << dataChange;

  QTest::newRow("change02") << commandList;
  commandList.clear();

  ModelRemoveCommand *rem;

  // Remove a single item without children.
  rem = new ModelRemoveCommand(m_model, this);
  rem->setAncestorRowNumbers(QList<int>() << 10 );
  rem->setStartRow(0);
  rem->setEndRow(0);

  commandList << rem;

  QTest::newRow("remove01") << commandList;
  commandList.clear();

  // Remove a single item with 10 children.
  rem = new ModelRemoveCommand(m_model, this);
  rem->setAncestorRowNumbers(QList<int>() << 10 );
  rem->setStartRow(6);
  rem->setEndRow(6);

  commandList << rem;

  QTest::newRow("remove02") << commandList;
  commandList.clear();

  // Remove a single item with no children from the top.
  rem = new ModelRemoveCommand(m_model, this);
  rem->setAncestorRowNumbers(QList<int>() << 10 << 5 );
  rem->setStartRow(0);
  rem->setEndRow(0);

  commandList << rem;

  QTest::newRow("remove03") << commandList;
  commandList.clear();

  // Remove a single second level item with no children from the bottom.
  rem = new ModelRemoveCommand(m_model, this);
  rem->setAncestorRowNumbers(QList<int>() << 10 << 5 );
  rem->setStartRow(8);
  rem->setEndRow(8);

  commandList << rem;

  QTest::newRow("remove04") << commandList;
  commandList.clear();

  // Remove a single second level item with no children from the middle.
  rem = new ModelRemoveCommand(m_model, this);
  rem->setAncestorRowNumbers(QList<int>() << 10 << 5 );
  rem->setStartRow(3);
  rem->setEndRow(3);

  commandList << rem;

  QTest::newRow("remove05") << commandList;
  commandList.clear();

  // clear the children of a second level item.
  rem = new ModelRemoveCommand(m_model, this);
  rem->setAncestorRowNumbers(QList<int>() << 10 << 5 );
  rem->setStartRow(0);
  rem->setEndRow(6);

  commandList << rem;

  QTest::newRow("remove06") << commandList;
  commandList.clear();

  // Clear a sub-tree;
  rem = new ModelRemoveCommand(m_model, this);
  rem->setAncestorRowNumbers(QList<int>() << 10 );
  rem->setStartRow(4);
  rem->setEndRow(4);

  commandList << rem;

  QTest::newRow("remove07") << commandList;
  commandList.clear();

}
