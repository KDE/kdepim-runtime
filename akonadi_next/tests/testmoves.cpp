

#include <QtTest>
#include <QtCore>
#include <qtest_kde.h>
#include <qtestevent.h>

#include "dynamictreemodel.h"
#include "proxymodeltest.h"
#include <QTreeView>

class MoveTester : public QObject
{
  Q_OBJECT
public:
  MoveTester(QObject *parent = 0);

  private slots:
    void init();

    void testMoveSameParentUp_data();
    void testMoveSameParentUp();

    void testMoveSameParentDown_data();
    void testMoveSameParentDown();

    void testMoveToGrandParent_data();
    void testMoveToGrandParent();

    void testMoveToSibling_data();
    void testMoveToSibling();

    void testMoveToUncle_data();
    void testMoveToUncle();

    void testMoveToDescendants();

    void testMoveWithinOwnRange_data();
    void testMoveWithinOwnRange();


private:
  DynamicTreeModel *m_model;

  ModelSpy *spy;

};


MoveTester::MoveTester(QObject* parent): QObject(parent)
{
}

void MoveTester::init()
{
  m_model = new DynamicTreeModel(this);

  ModelInsertCommand *insertCommand = new ModelInsertCommand(m_model, this);
  insertCommand->setNumCols(4);
  insertCommand->setStartRow(0);
  insertCommand->setEndRow(9);
  insertCommand->doCommand();

  insertCommand = new ModelInsertCommand(m_model, this);
  insertCommand->setAncestorRowNumbers(QList<int>() << 5);
  insertCommand->setNumCols(4);
  insertCommand->setStartRow(0);
  insertCommand->setEndRow(9);
  insertCommand->doCommand();

  spy = new ModelSpy(this);
  spy->setModel(m_model);

}

void MoveTester::testMoveSameParentDown_data()
{
  QTest::addColumn<int>("startRow");
  QTest::addColumn<int>("endRow");
  QTest::addColumn<int>("destRow");

  // Move from the start to the middle
  QTest::newRow("move01") << 0 << 2 << 8;
  // Move from the start to the end
  QTest::newRow("move02") << 0 << 2 << 10;
  // Move from the middle to the middle
  QTest::newRow("move03") << 3 << 5 << 8;
  // Move from the middle to the end
  QTest::newRow("move04") << 3 << 5 << 10;
}

void MoveTester::testMoveSameParentDown()
{
  QFETCH( int, startRow);
  QFETCH( int, endRow);
  QFETCH( int, destRow);

  QList<QPersistentModelIndex> persistentList;
  QModelIndexList indexList;

  for (int column = 0; column < m_model->columnCount(); ++column)
  {
    for (int row= 0; row < m_model->rowCount(); ++row)
    {
      QModelIndex idx = m_model->index(row, column);
      QVERIFY(idx.isValid());
      indexList << idx;
      persistentList << QPersistentModelIndex(idx);
    }
  }

  QModelIndex parent = m_model->index(5, 0);
  for (int column = 0; column < m_model->columnCount(); ++column)
  {
    for (int row= 0; row < m_model->rowCount(parent); ++row)
    {
      QModelIndex idx = m_model->index(row, column, parent);
      QVERIFY(idx.isValid());
      indexList << idx;
      persistentList << QPersistentModelIndex(idx);
    }
  }

  QSignalSpy beforeSpy(m_model, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  QSignalSpy afterSpy(m_model, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));

  ModelMoveCommand *moveCommand = new ModelMoveCommand(m_model, this);
  moveCommand->setNumCols(4);
  moveCommand->setStartRow(startRow);
  moveCommand->setEndRow(endRow);
  moveCommand->setDestRow(destRow);
  moveCommand->doCommand();

  QVariantList beforeSignal = beforeSpy.takeAt(0);
  QVariantList afterSignal = afterSpy.takeAt(0);

  QCOMPARE(beforeSignal.size(), 5);
  QCOMPARE(beforeSignal.at(0).value<QModelIndex>(), QModelIndex());
  QCOMPARE(beforeSignal.at(1).toInt(), startRow);
  QCOMPARE(beforeSignal.at(2).toInt(), endRow);
  QCOMPARE(beforeSignal.at(3).value<QModelIndex>(), QModelIndex());
  QCOMPARE(beforeSignal.at(4).toInt(), destRow);

  QCOMPARE(afterSignal.size(), 5);
  QCOMPARE(afterSignal.at(0).value<QModelIndex>(), QModelIndex());
  QCOMPARE(afterSignal.at(1).toInt(), startRow);
  QCOMPARE(afterSignal.at(2).toInt(), endRow);
  QCOMPARE(afterSignal.at(3).value<QModelIndex>(), QModelIndex());
  QCOMPARE(afterSignal.at(4).toInt(), destRow);

  for (int i = 0; i < indexList.size(); i++)
  {
    QModelIndex idx = indexList.at(i);
    QModelIndex persistentIndex = persistentList.at(i);
    if (idx.parent() == QModelIndex())
    {
      int row = idx.row();
      if ( row >= startRow)
      {
        if (row <= endRow)
        {
          QCOMPARE(row + destRow - endRow - 1, persistentIndex.row() );
          QCOMPARE(idx.column(), persistentIndex.column());
          QCOMPARE(idx.parent(), persistentIndex.parent());
          QCOMPARE(idx.model(), persistentIndex.model());
        } else if ( row < destRow)
        {
          QCOMPARE(row - (endRow - startRow + 1), persistentIndex.row() );
          QCOMPARE(idx.column(), persistentIndex.column());
          QCOMPARE(idx.parent(), persistentIndex.parent());
          QCOMPARE(idx.model(), persistentIndex.model());
        } else
        {
           QCOMPARE(idx, persistentIndex);
        }
      } else
      {
        QCOMPARE(idx, persistentIndex);
      }
    } else
    {
      QCOMPARE(idx, persistentIndex);
    }
  }
}

void MoveTester::testMoveSameParentUp_data()
{
  QTest::addColumn<int>("startRow");
  QTest::addColumn<int>("endRow");
  QTest::addColumn<int>("destRow");

  // Move from the middle to the start
  QTest::newRow("move01") << 5 << 7 << 0;
  // Move from the end to the start
  QTest::newRow("move02") << 8 << 9 << 0;
  // Move from the middle to the middle
  QTest::newRow("move03") << 5 << 7 << 2;
  // Move from the end to the middle
  QTest::newRow("move04") << 8 << 9 << 5;
}

void MoveTester::testMoveSameParentUp()
{

  QFETCH( int, startRow);
  QFETCH( int, endRow);
  QFETCH( int, destRow);

  QList<QPersistentModelIndex> persistentList;
  QModelIndexList indexList;

  for (int column = 0; column < m_model->columnCount(); ++column)
  {
    for (int row= 0; row < m_model->rowCount(); ++row)
    {
      QModelIndex idx = m_model->index(row, column);
      QVERIFY(idx.isValid());
      indexList << idx;
      persistentList << QPersistentModelIndex(idx);
    }
  }

  QModelIndex parent = m_model->index(2, 0);
  for (int column = 0; column < m_model->columnCount(); ++column)
  {
    for (int row= 0; row < m_model->rowCount(parent); ++row)
    {
      QModelIndex idx = m_model->index(row, column, parent);
      QVERIFY(idx.isValid());
      indexList << idx;
      persistentList << QPersistentModelIndex(idx);
    }
  }

  QSignalSpy beforeSpy(m_model, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  QSignalSpy afterSpy(m_model, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));


  ModelMoveCommand *moveCommand = new ModelMoveCommand(m_model, this);
  moveCommand->setNumCols(4);
  moveCommand->setStartRow(startRow);
  moveCommand->setEndRow(endRow);
  moveCommand->setDestRow(destRow);
  moveCommand->doCommand();

  QVariantList beforeSignal = beforeSpy.takeAt(0);
  QVariantList afterSignal = afterSpy.takeAt(0);

  QCOMPARE(beforeSignal.size(), 5);
  QCOMPARE(beforeSignal.at(0).value<QModelIndex>(), QModelIndex());
  QCOMPARE(beforeSignal.at(1).toInt(), startRow);
  QCOMPARE(beforeSignal.at(2).toInt(), endRow);
  QCOMPARE(beforeSignal.at(3).value<QModelIndex>(), QModelIndex());
  QCOMPARE(beforeSignal.at(4).toInt(), destRow);

  QCOMPARE(afterSignal.size(), 5);
  QCOMPARE(afterSignal.at(0).value<QModelIndex>(), QModelIndex());
  QCOMPARE(afterSignal.at(1).toInt(), startRow);
  QCOMPARE(afterSignal.at(2).toInt(), endRow);
  QCOMPARE(afterSignal.at(3).value<QModelIndex>(), QModelIndex());
  QCOMPARE(afterSignal.at(4).toInt(), destRow);


  for (int i = 0; i < indexList.size(); i++)
  {
    QModelIndex idx = indexList.at(i);
    QModelIndex persistentIndex = persistentList.at(i);
    if (idx.parent() == QModelIndex())
    {
      int row = idx.row();
      if ( row >= destRow)
      {
        if (row < startRow)
        {
          QCOMPARE(row + endRow - startRow + 1, persistentIndex.row() );
          QCOMPARE(idx.column(), persistentIndex.column());
          QCOMPARE(idx.parent(), persistentIndex.parent());
          QCOMPARE(idx.model(), persistentIndex.model());
        } else if ( row <= endRow)
        {
          QCOMPARE(row + destRow - startRow, persistentIndex.row() );
          QCOMPARE(idx.column(), persistentIndex.column());
          QCOMPARE(idx.parent(), persistentIndex.parent());
          QCOMPARE(idx.model(), persistentIndex.model());
        } else
        {
          QCOMPARE(idx, persistentIndex);
        }
      } else
      {
        QCOMPARE(idx, persistentIndex);
      }
    } else
    {
      QCOMPARE(idx, persistentIndex);
    }
  }
}

void MoveTester::testMoveToGrandParent_data()
{
  QTest::addColumn<int>("startRow");
  QTest::addColumn<int>("endRow");
  QTest::addColumn<int>("destRow");

  // Move from the start to the middle
  QTest::newRow("move01") << 0 << 2 << 8;
  // Move from the start to the end
  QTest::newRow("move02") << 0 << 2 << 10;
  // Move from the middle to the middle
  QTest::newRow("move03") << 3 << 5 << 8;
  // Move from the middle to the end
  QTest::newRow("move04") << 3 << 5 << 10;

  // Move from the middle to the start
  QTest::newRow("move05") << 5 << 7 << 0;
  // Move from the end to the start
  QTest::newRow("move06") << 8 << 9 << 0;
  // Move from the middle to the middle
  QTest::newRow("move07") << 5 << 7 << 2;
  // Move from the end to the middle
  QTest::newRow("move08") << 8 << 9 << 5;

  // Moving to the same row in a different parent doesn't confuse things.
  QTest::newRow("move09") << 8 << 8 << 8;

  // Moving to the row of my parent and its neighbours doesn't confuse things
  QTest::newRow("move09") << 8 << 8 << 4;
  QTest::newRow("move10") << 8 << 8 << 5;
  QTest::newRow("move11") << 8 << 8 << 6;

  // Moving everything from one parent to another
  QTest::newRow("move12") << 0 << 9 << 10;
}

void MoveTester::testMoveToGrandParent()
{

  QFETCH( int, startRow);
  QFETCH( int, endRow);
  QFETCH( int, destRow);

  QList<QPersistentModelIndex> persistentList;
  QModelIndexList indexList;
  QModelIndexList parentsList;

  for (int column = 0; column < m_model->columnCount(); ++column)
  {
    for (int row= 0; row < m_model->rowCount(); ++row)
    {
      QModelIndex idx = m_model->index(row, column);
      QVERIFY(idx.isValid());
      indexList << idx;
      parentsList << idx.parent();
      persistentList << QPersistentModelIndex(idx);
    }
  }

  QModelIndex sourceIndex = m_model->index(5, 0);
  for (int column = 0; column < m_model->columnCount(); ++column)
  {
    for (int row= 0; row < m_model->rowCount(sourceIndex); ++row)
    {
      QModelIndex idx = m_model->index(row, column, sourceIndex);
      QVERIFY(idx.isValid());
      indexList << idx;
      parentsList << idx.parent();
      persistentList << QPersistentModelIndex(idx);
    }
  }

  QSignalSpy beforeSpy(m_model, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  QSignalSpy afterSpy(m_model, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));


  ModelMoveCommand *moveCommand = new ModelMoveCommand(m_model, this);
  moveCommand->setAncestorRowNumbers(QList<int>() << 5);
  moveCommand->setNumCols(4);
  moveCommand->setStartRow(startRow);
  moveCommand->setEndRow(endRow);
  moveCommand->setDestRow(destRow);
  moveCommand->doCommand();

  QVariantList beforeSignal = beforeSpy.takeAt(0);
  QVariantList afterSignal = afterSpy.takeAt(0);

  QCOMPARE(beforeSignal.size(), 5);
  QCOMPARE(beforeSignal.at(0).value<QModelIndex>(), sourceIndex);
  QCOMPARE(beforeSignal.at(1).toInt(), startRow);
  QCOMPARE(beforeSignal.at(2).toInt(), endRow);
  QCOMPARE(beforeSignal.at(3).value<QModelIndex>(), QModelIndex());
  QCOMPARE(beforeSignal.at(4).toInt(), destRow);

  QCOMPARE(afterSignal.size(), 5);
  QCOMPARE(afterSignal.at(0).value<QModelIndex>(), sourceIndex);
  QCOMPARE(afterSignal.at(1).toInt(), startRow);
  QCOMPARE(afterSignal.at(2).toInt(), endRow);
  QCOMPARE(afterSignal.at(3).value<QModelIndex>(), QModelIndex());
  QCOMPARE(afterSignal.at(4).toInt(), destRow);

  for (int i = 0; i < indexList.size(); i++)
  {
    QModelIndex idx = indexList.at(i);
    QModelIndex idxParent = parentsList.at(i);
    QModelIndex persistentIndex = persistentList.at(i);
    int row = idx.row();
    if (idxParent == QModelIndex())
    {
      if ( row >= destRow)
      {
          QCOMPARE(row + endRow - startRow + 1, persistentIndex.row() );
          QCOMPARE(idx.column(), persistentIndex.column());
          QCOMPARE(idxParent, persistentIndex.parent());
          QCOMPARE(idx.model(), persistentIndex.model());
      } else
      {
        QCOMPARE(idx, persistentIndex);
      }
    } else
    {
      if (row < startRow)
      {
        QCOMPARE(idx, persistentIndex);
      } else if (row <= endRow)
      {
        QCOMPARE(row + destRow - startRow, persistentIndex.row() );
        QCOMPARE(idx.column(), persistentIndex.column());
        QCOMPARE(QModelIndex(), persistentIndex.parent());
        QCOMPARE(idx.model(), persistentIndex.model());
      } else {
        QCOMPARE(row - (endRow - startRow + 1), persistentIndex.row() );
        QCOMPARE(idx.column(), persistentIndex.column());

        if (idxParent.row() >= destRow)
        {
          QModelIndex adjustedParent;
          adjustedParent = idxParent.sibling( idxParent.row() + endRow - startRow + 1,  idxParent.column());
          QCOMPARE(adjustedParent, persistentIndex.parent());
        } else
        {
          QCOMPARE(idxParent, persistentIndex.parent());
        }
        QCOMPARE(idx.model(), persistentIndex.model());
      }
    }
  }
}

void MoveTester::testMoveToSibling_data()
{
  QTest::addColumn<int>("startRow");
  QTest::addColumn<int>("endRow");
  QTest::addColumn<int>("destRow");

  // Move from the start to the middle
  QTest::newRow("move01") << 0 << 2 << 8;
  // Move from the start to the end
  QTest::newRow("move02") << 0 << 2 << 10;
  // Move from the middle to the middle
  QTest::newRow("move03") << 2 << 4 << 8;
  // Move from the middle to the end
  QTest::newRow("move04") << 2 << 4 << 10;

  // Move from the middle to the start
  QTest::newRow("move05") << 8 << 8 << 0;
  // Move from the end to the start
  QTest::newRow("move06") << 8 << 9 << 0;
  // Move from the middle to the middle
  QTest::newRow("move07") << 6 << 8 << 2;
  // Move from the end to the middle
  QTest::newRow("move08") << 8 << 9 << 5;

  // Moving to the same row in a different parent doesn't confuse things.
  QTest::newRow("move09") << 8 << 8 << 8;

  // Moving to the row of my target and its neighbours doesn't confuse things
  QTest::newRow("move09") << 8 << 8 << 4;
  QTest::newRow("move10") << 8 << 8 << 5;
  QTest::newRow("move11") << 8 << 8 << 6;
}

void MoveTester::testMoveToSibling()
{

  QFETCH( int, startRow);
  QFETCH( int, endRow);
  QFETCH( int, destRow);

  QList<QPersistentModelIndex> persistentList;
  QModelIndexList indexList;
  QModelIndexList parentsList;

  const int column = 0;

  for (int i= 0; i < m_model->rowCount(); ++i)
  {
    QModelIndex idx = m_model->index(i, column);
    QVERIFY(idx.isValid());
    indexList << idx;
    parentsList << idx.parent();
    persistentList << QPersistentModelIndex(idx);
  }

  QModelIndex destIndex = m_model->index(5, 0);
  QModelIndex sourceIndex;
  for (int i= 0; i < m_model->rowCount(destIndex); ++i)
  {
    QModelIndex idx = m_model->index(i, column, destIndex);
    QVERIFY(idx.isValid());
    indexList << idx;
    parentsList << idx.parent();
    persistentList << QPersistentModelIndex(idx);
  }

  QSignalSpy beforeSpy(m_model, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  QSignalSpy afterSpy(m_model, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));


  ModelMoveCommand *moveCommand = new ModelMoveCommand(m_model, this);
  moveCommand->setNumCols(4);
  moveCommand->setStartRow(startRow);
  moveCommand->setEndRow(endRow);
  moveCommand->setDestAncestors(QList<int>() << 5);
  moveCommand->setDestRow(destRow);
  moveCommand->doCommand();

  QVariantList beforeSignal = beforeSpy.takeAt(0);
  QVariantList afterSignal = afterSpy.takeAt(0);

  QCOMPARE(beforeSignal.size(), 5);
  QCOMPARE(beforeSignal.at(0).value<QModelIndex>(), sourceIndex);
  QCOMPARE(beforeSignal.at(1).toInt(), startRow);
  QCOMPARE(beforeSignal.at(2).toInt(), endRow);
  QCOMPARE(beforeSignal.at(3).value<QModelIndex>(), destIndex);
  QCOMPARE(beforeSignal.at(4).toInt(), destRow);

  QCOMPARE(afterSignal.size(), 5);
  QCOMPARE(afterSignal.at(0).value<QModelIndex>(), sourceIndex);
  QCOMPARE(afterSignal.at(1).toInt(), startRow);
  QCOMPARE(afterSignal.at(2).toInt(), endRow);
  QCOMPARE(afterSignal.at(3).value<QModelIndex>(), destIndex);
  QCOMPARE(afterSignal.at(4).toInt(), destRow);

  for (int i = 0; i < indexList.size(); i++)
  {
    QModelIndex idx = indexList.at(i);
    QModelIndex idxParent = parentsList.at(i);
    QModelIndex persistentIndex = persistentList.at(i);

    QModelIndex adjustedDestination = destIndex.sibling(destIndex.row() - (endRow - startRow + 1), destIndex.column());
    int row = idx.row();
    if (idxParent == destIndex)
    {
      if ( row >= destRow)
      {
          QCOMPARE(row + endRow - startRow + 1, persistentIndex.row() );
          QCOMPARE(idx.column(), persistentIndex.column());
          if (idxParent.row() > startRow)
          {
            QCOMPARE(adjustedDestination, persistentIndex.parent());
          } else {
            QCOMPARE(destIndex, persistentIndex.parent());
          }
          QCOMPARE(idx.model(), persistentIndex.model());
      } else
      {
        QCOMPARE(idx, persistentIndex);
      }
    } else
    {
      if (row < startRow)
      {
        QCOMPARE(idx, persistentIndex);
      } else if (row <= endRow)
      {
        QCOMPARE(row + destRow - startRow, persistentIndex.row() );
        QCOMPARE(idx.column(), persistentIndex.column());
        if (destIndex.row() > startRow)
        {
          QCOMPARE(adjustedDestination, persistentIndex.parent());
        } else {
          QCOMPARE(destIndex, persistentIndex.parent());
        }

        QCOMPARE(idx.model(), persistentIndex.model());

      } else {
        QCOMPARE(row - (endRow - startRow + 1), persistentIndex.row() );
        QCOMPARE(idx.column(), persistentIndex.column());
        QCOMPARE(idxParent, persistentIndex.parent());
        QCOMPARE(idx.model(), persistentIndex.model());
      }
    }
  }
}

void MoveTester::testMoveToUncle_data()
{

  QTest::addColumn<int>("startRow");
  QTest::addColumn<int>("endRow");
  QTest::addColumn<int>("destRow");

  // Move from the start to the middle
  QTest::newRow("move01") << 0 << 2 << 8;
  // Move from the start to the end
  QTest::newRow("move02") << 0 << 2 << 10;
  // Move from the middle to the middle
  QTest::newRow("move03") << 3 << 5 << 8;
  // Move from the middle to the end
  QTest::newRow("move04") << 3 << 5 << 10;

  // Move from the middle to the start
  QTest::newRow("move05") << 5 << 7 << 0;
  // Move from the end to the start
  QTest::newRow("move06") << 8 << 9 << 0;
  // Move from the middle to the middle
  QTest::newRow("move07") << 5 << 7 << 2;
  // Move from the end to the middle
  QTest::newRow("move08") << 8 << 9 << 5;

  // Moving to the same row in a different parent doesn't confuse things.
  QTest::newRow("move09") << 8 << 8 << 8;

  // Moving to the row of my parent and its neighbours doesn't confuse things
  QTest::newRow("move09") << 8 << 8 << 4;
  QTest::newRow("move10") << 8 << 8 << 5;
  QTest::newRow("move11") << 8 << 8 << 6;

  // Moving everything from one parent to another
  QTest::newRow("move12") << 0 << 9 << 10;
}

void MoveTester::testMoveToUncle()
{
  // Need to have some extra rows available.
  ModelInsertCommand *insertCommand = new ModelInsertCommand(m_model, this);
  insertCommand->setAncestorRowNumbers(QList<int>() << 9);
  insertCommand->setNumCols(4);
  insertCommand->setStartRow(0);
  insertCommand->setEndRow(9);
  insertCommand->doCommand();

  QFETCH( int, startRow);
  QFETCH( int, endRow);
  QFETCH( int, destRow);

  QList<QPersistentModelIndex> persistentList;
  QModelIndexList indexList;
  QModelIndexList parentsList;

  const int column = 0;

  QModelIndex sourceIndex = m_model->index(9, 0);
  for (int i= 0; i < m_model->rowCount(sourceIndex); ++i)
  {
    QModelIndex idx = m_model->index(i, column, sourceIndex);
    QVERIFY(idx.isValid());
    indexList << idx;
    parentsList << idx.parent();
    persistentList << QPersistentModelIndex(idx);
  }

  QModelIndex destIndex = m_model->index(5, 0);
  for (int i= 0; i < m_model->rowCount(destIndex); ++i)
  {
    QModelIndex idx = m_model->index(i, column, destIndex);
    QVERIFY(idx.isValid());
    indexList << idx;
    parentsList << idx.parent();
    persistentList << QPersistentModelIndex(idx);
  }

  QSignalSpy beforeSpy(m_model, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  QSignalSpy afterSpy(m_model, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));

  ModelMoveCommand *moveCommand = new ModelMoveCommand(m_model, this);
  moveCommand->setAncestorRowNumbers(QList<int>() << 9);
  moveCommand->setNumCols(4);
  moveCommand->setStartRow(startRow);
  moveCommand->setEndRow(endRow);
  moveCommand->setDestAncestors(QList<int>() << 5);
  moveCommand->setDestRow(destRow);
  moveCommand->doCommand();

  QVariantList beforeSignal = beforeSpy.takeAt(0);
  QVariantList afterSignal = afterSpy.takeAt(0);

  QCOMPARE(beforeSignal.size(), 5);
  QCOMPARE(beforeSignal.at(0).value<QModelIndex>(), sourceIndex);
  QCOMPARE(beforeSignal.at(1).toInt(), startRow);
  QCOMPARE(beforeSignal.at(2).toInt(), endRow);
  QCOMPARE(beforeSignal.at(3).value<QModelIndex>(), destIndex);
  QCOMPARE(beforeSignal.at(4).toInt(), destRow);

  QCOMPARE(afterSignal.size(), 5);
  QCOMPARE(afterSignal.at(0).value<QModelIndex>(), sourceIndex);
  QCOMPARE(afterSignal.at(1).toInt(), startRow);
  QCOMPARE(afterSignal.at(2).toInt(), endRow);
  QCOMPARE(afterSignal.at(3).value<QModelIndex>(), destIndex);
  QCOMPARE(afterSignal.at(4).toInt(), destRow);

  for (int i = 0; i < indexList.size(); i++)
  {
    QModelIndex idx = indexList.at(i);
    QModelIndex idxParent = parentsList.at(i);
    QModelIndex persistentIndex = persistentList.at(i);

    int row = idx.row();
    if (idxParent == destIndex)
    {
      if ( row >= destRow)
      {
          QCOMPARE(row + endRow - startRow + 1, persistentIndex.row() );
          QCOMPARE(idx.column(), persistentIndex.column());
          QCOMPARE(destIndex, persistentIndex.parent());
          QCOMPARE(idx.model(), persistentIndex.model());
      } else
      {
        QCOMPARE(idx, persistentIndex);
      }
    } else
    {
      if (row < startRow)
      {
        QCOMPARE(idx, persistentIndex);
      } else if (row <= endRow)
      {
        QCOMPARE(row + destRow - startRow, persistentIndex.row() );
        QCOMPARE(idx.column(), persistentIndex.column());
        QCOMPARE(destIndex, persistentIndex.parent());
        QCOMPARE(idx.model(), persistentIndex.model());

      } else {
        QCOMPARE(row - (endRow - startRow + 1), persistentIndex.row() );
        QCOMPARE(idx.column(), persistentIndex.column());
        QCOMPARE(idxParent, persistentIndex.parent());
        QCOMPARE(idx.model(), persistentIndex.model());
      }
    }
  }
}

void MoveTester::testMoveToDescendants()
{
  // Attempt to move a row to its ancestors depth rows deep.
  const int depth = 6;

  // Need to have some extra rows available in a tree.
  QList<int> rows;
  ModelInsertCommand *insertCommand;
  for (int i = 0; i < depth; i++)
  {
    insertCommand = new ModelInsertCommand(m_model, this);
    insertCommand->setAncestorRowNumbers(rows);
    insertCommand->setNumCols(4);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(9);
    insertCommand->doCommand();
    rows << 9;
  }

  QList<QPersistentModelIndex> persistentList;
  QModelIndexList indexList;
  QModelIndexList parentsList;

  const int column = 0;

  QModelIndex sourceIndex = m_model->index(9, 0);
  for (int i= 0; i < m_model->rowCount(sourceIndex); ++i)
  {
    QModelIndex idx = m_model->index(i, column, sourceIndex);
    QVERIFY(idx.isValid());
    indexList << idx;
    parentsList << idx.parent();
    persistentList << QPersistentModelIndex(idx);
  }

  QModelIndex destIndex = m_model->index(5, 0);
  for (int i= 0; i < m_model->rowCount(destIndex); ++i)
  {
    QModelIndex idx = m_model->index(i, column, destIndex);
    QVERIFY(idx.isValid());
    indexList << idx;
    parentsList << idx.parent();
    persistentList << QPersistentModelIndex(idx);
  }

  QSignalSpy beforeSpy(m_model, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  QSignalSpy afterSpy(m_model, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));

  ModelFakeMoveCommand *moveCommand;
  QList<int> ancestors;
  while (ancestors.size() < depth)
  {
    ancestors << 9;
    for (int row = 0; row <= 9; row++)
    {
      moveCommand = new ModelFakeMoveCommand(m_model, this);
      moveCommand->setNumCols(4);
      moveCommand->setStartRow(9);
      moveCommand->setEndRow(9);
      moveCommand->setDestAncestors(ancestors);
      moveCommand->setDestRow(row);
      moveCommand->doCommand();

      QVERIFY(beforeSpy.size() == 0);
      QVERIFY(afterSpy.size() == 0);
    }
  }
}

void MoveTester::testMoveWithinOwnRange_data()
{
  QTest::addColumn<int>("startRow");
  QTest::addColumn<int>("endRow");
  QTest::addColumn<int>("destRow");

  QTest::newRow("move01") << 0 << 0 << 0;
  QTest::newRow("move02") << 0 << 0 << 1;
  QTest::newRow("move03") << 0 << 5 << 0;
  QTest::newRow("move04") << 0 << 5 << 1;
  QTest::newRow("move05") << 0 << 5 << 2;
  QTest::newRow("move06") << 0 << 5 << 3;
  QTest::newRow("move07") << 0 << 5 << 4;
  QTest::newRow("move08") << 0 << 5 << 5;
  QTest::newRow("move09") << 0 << 5 << 6;
  QTest::newRow("move08") << 3 << 5 << 5;
  QTest::newRow("move08") << 3 << 5 << 6;
  QTest::newRow("move09") << 4 << 5 << 5;
  QTest::newRow("move10") << 4 << 5 << 6;
  QTest::newRow("move11") << 5 << 5 << 5;
  QTest::newRow("move12") << 5 << 5 << 6;
  QTest::newRow("move13") << 5 << 9 << 9;
  QTest::newRow("move14") << 5 << 9 << 10;
  QTest::newRow("move15") << 6 << 9 << 9;
  QTest::newRow("move16") << 6 << 9 << 10;
  QTest::newRow("move17") << 7 << 9 << 9;
  QTest::newRow("move18") << 7 << 9 << 10;
  QTest::newRow("move19") << 8 << 9 << 9;
  QTest::newRow("move20") << 8 << 9 << 10;
  QTest::newRow("move21") << 9 << 9 << 9;
  QTest::newRow("move22") << 0 << 9 << 10;

}

void MoveTester::testMoveWithinOwnRange()
{

  QFETCH( int, startRow);
  QFETCH( int, endRow);
  QFETCH( int, destRow);


  QSignalSpy beforeSpy(m_model, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  QSignalSpy afterSpy(m_model, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));

  ModelMoveCommand *moveCommand = new ModelMoveCommand(m_model, this);
  moveCommand->setNumCols(4);
  moveCommand->setStartRow(startRow);
  moveCommand->setEndRow(endRow);
  moveCommand->setDestRow(destRow);
  moveCommand->doCommand();

  QVERIFY(beforeSpy.size() == 0);
  QVERIFY(afterSpy.size() == 0);


}



QTEST_KDEMAIN(MoveTester, GUI)
#include "movetester.moc"
