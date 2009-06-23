
#ifndef PROXY_MODEL_TEST_H
#define PROXY_MODEL_TEST_H

#include <QtTest>
#include <QtCore>
#include <qtest_kde.h>
#include <qtestevent.h>
#include <QItemSelectionRange>

#include "../abstractproxymodel.h"
#include "../abstractitemmodel.h"
#include "dynamictreemodel.h"

Q_DECLARE_METATYPE( QModelIndex )


class IndexFinder
{
public:
  IndexFinder() : m_model(0) {}

  IndexFinder(QAbstractItemModel *model, QList<int> rows = QList<int>() )
      :  m_rows(rows), m_model(model)
  {
  }

  QModelIndex getIndex()
  {
    const int col = 0;
    QModelIndex parent = QModelIndex();
    QListIterator<int> i(m_rows);
    while (i.hasNext())
    {
      parent = m_model->index(i.next(), col, parent);
      Q_ASSERT(parent.isValid());
    }
    return parent;
  }

private:
  QList<int> m_rows;
  QAbstractItemModel *m_model;
};


Q_DECLARE_METATYPE( IndexFinder )

class ModelSpy : public QObject, public QList<QVariantList>
{
  Q_OBJECT
public:
  ModelSpy(QObject *parent);

  void setModel(AbstractProxyModel *model);

protected slots:
  void rowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
  void rowsInserted(const QModelIndex &parent, int start, int end);
  void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
  void rowsRemoved(const QModelIndex &parent, int start, int end);
  void rowsAboutToBeMoved(const QModelIndex &srcParent, int start, int end, const QModelIndex &destParent, int destStart);
  void rowsMoved(const QModelIndex &srcParent, int start, int end, const QModelIndex &destParent, int destStart);

  void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

private:
  AbstractProxyModel *m_model;
};

struct PersistentIndexChange
{
  IndexFinder parentFinder;
  int startRow;
  int endRow;
  int difference;
  bool toInvalid;
  QModelIndexList indexes;
  QList<QPersistentModelIndex> persistentIndexes;

  QModelIndexList descendantIndexes;
  QList<QPersistentModelIndex> persistentDescendantIndexes;
};

enum SignalType
{
  NoSignal,
  RowsAboutToBeInserted,
  RowsInserted,
  RowsAboutToBeRemoved,
  RowsRemoved,
  RowsAboutToBeMoved,
  RowsMoved,
  DataChanged
};


class ProxyModelTest : public QObject
{
  Q_OBJECT
public:
  ProxyModelTest(QObject *parent = 0);

  void setProxyModel(AbstractProxyModel *proxyModel);
  DynamicTreeModel* sourceModel();

private slots:
  void initTestCase();

  void testInsertAndRemove_data();
  void testInsertAndRemove() { doTest(); }



//   void testMove_data();
//   void testMove() { doTest(); }

protected:
  QModelIndexList getUnchangedIndexes(const QModelIndex &parent, QList< QItemSelectionRange > ignoredRanges);

  QModelIndexList getDescendantIndexes(const QModelIndex &index);
  QList< QPersistentModelIndex > toPersistent(QModelIndexList list);

  PersistentIndexChange getChange(IndexFinder parentFinder, int start, int end, int difference, bool toInvalid = false);
  QVariantList getSignal(SignalType type, IndexFinder parentFinder, int start, int end);
  void signalInsertion(const QString &name, IndexFinder parentFinder, int startRow, int rowsAffected, int rowCount = -1);
  void signalRemoval(const QString &name, IndexFinder parentFinder, int startRow, int rowsAffected, int rowCount = -1);
  void noSignal(const QString &name);

  void doTest();
  void setExpected(const QString &name, const QList<QVariantList> &list, const QList<PersistentIndexChange> &persistentChanges);
  void handleSignal(QVariantList expected);
  QVariantList getResultSignal();

private:
  QHash<QString, QList<QVariantList> > m_expectedSignals;
  QHash<QString, QList<PersistentIndexChange> > m_persistentChanges;
  DynamicTreeModel *m_model;
  AbstractProxyModel *m_proxyModel;
  ModelSpy *m_modelSpy;

};

#endif
