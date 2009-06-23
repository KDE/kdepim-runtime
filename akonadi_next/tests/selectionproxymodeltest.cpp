
#include "proxymodeltest.h"

#include <QTimer>

#include "../selectionproxymodel.h"

using namespace Akonadi;


class ModelWatcher : public QObject
{
  Q_OBJECT
public:
  ModelWatcher(QObject *parent)
      : QObject(parent)
  {
  }

  void setWatchedModel(QAbstractItemModel *model)
  {
    m_model = model;
    setWatch(true);
  }

  void setSelectionModel(QItemSelectionModel *selectionModel)
  {
    m_selectionModel = selectionModel;
  }

  void setWatch(bool watch)
  {
    Q_ASSERT(m_model);
    if (watch)
      connect(m_model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            SLOT(rowsInserted(const QModelIndex &, int, int)));
    else
      disconnect(m_model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            this, SLOT(rowsInserted(const QModelIndex &, int, int)));
  }

  void setWatchedParent(const QModelIndex &parent)
  {
    m_parent = parent;
  }

  void setWatchedRow(int row)
  {
    m_row = row;
  }

signals:
  void newItem(const QModelIndex &idx);

public slots:
  void rowsInserted(const QModelIndex &parent, int start, int end)
  {
    if (m_parent != parent)
      return;

    if (start < m_row || m_row > end)
      return;

    QModelIndex idx = m_model->index(m_row, 0, parent);

    m_selectionModel->select(idx, QItemSelectionModel::SelectCurrent);

    setWatch(false);
  }

private:
  QAbstractItemModel *m_model;
  QItemSelectionModel *m_selectionModel;
  QModelIndex m_parent;
  int m_row;
};

class SelectionProxyModelTest : public ProxyModelTest
{
  Q_OBJECT
public:
  SelectionProxyModelTest(QObject *parent = 0)
      : ProxyModelTest( parent ),
      m_selectionModel(0)
  {
  }

private slots:
  void initTestCase()
  {
    // Make different selections and run all of the tests.
    m_selectionModel = new QItemSelectionModel(sourceModel());
    m_proxyModel = new SelectionProxyModel(m_selectionModel, this);
    setProxyModel(m_proxyModel);

    ModelWatcher *watcher = new ModelWatcher(this);
    watcher->setWatchedModel(sourceModel());
    watcher->setSelectionModel(m_selectionModel);
    watcher->setWatchedParent(QModelIndex());
    watcher->setWatchedRow(0);

    QList<QVariantList> signalList;
    QVariantList expected;
    QList<PersistentIndexChange> persistentList;
    IndexFinder indexFinder;


    int startRow = 0;
    int rowsInserted = 1;
    int rowCount = 0;
    // The selection will cause some signals to be emitted.
    signalInsertion("insert01", indexFinder, startRow, rowsInserted);

    indexFinder = IndexFinder( m_proxyModel, QList<int>() << 0);
    rowsInserted = 10;
    signalInsertion("insert02", indexFinder, startRow, rowsInserted);

    noSignal("insert03");
    noSignal("insert04");

    PersistentIndexChange change;

    rowCount = 10;
    signalInsertion("insert05", indexFinder, startRow, rowsInserted, rowCount);

    startRow = 20;
    signalInsertion("insert06", indexFinder, startRow, rowsInserted);

    startRow = 10;
    rowCount = 30;
    signalInsertion("insert07", indexFinder, startRow, rowsInserted, rowCount);

    indexFinder = IndexFinder(m_proxyModel, QList<int>() << 0 << 5);

    startRow = 0;
    rowCount = 0;
    signalList << getSignal(RowsAboutToBeInserted, indexFinder, startRow, startRow + rowsInserted - 1);
    signalList << getSignal(RowsInserted, indexFinder, startRow, startRow + rowsInserted - 1);

    indexFinder = IndexFinder(m_proxyModel, QList<int>() << 0 << 5 << 5);
    signalList << getSignal(RowsAboutToBeInserted, indexFinder, startRow, startRow + rowsInserted - 1);
    signalList << getSignal(RowsInserted, indexFinder, startRow, startRow + rowsInserted - 1);

    indexFinder = IndexFinder(m_proxyModel, QList<int>() << 0 << 5 << 5 << 5);
    signalList << getSignal(RowsAboutToBeInserted, indexFinder, startRow, startRow + rowsInserted - 1);
    signalList << getSignal(RowsInserted, indexFinder, startRow, startRow + rowsInserted - 1);

    indexFinder = IndexFinder(m_proxyModel, QList<int>() << 0 << 6);
    signalList << getSignal(RowsAboutToBeInserted, indexFinder, startRow, startRow + rowsInserted - 1);
    signalList << getSignal(RowsInserted, indexFinder, startRow, startRow + rowsInserted - 1);

    indexFinder = IndexFinder(m_proxyModel, QList<int>() << 0 << 7);
    signalList << getSignal(RowsAboutToBeInserted, indexFinder, startRow, startRow + rowsInserted - 1);
    signalList << getSignal(RowsInserted, indexFinder, startRow, startRow + rowsInserted - 1);

    setExpected("insert08", signalList, persistentList);
    signalList.clear();
    persistentList.clear();

    indexFinder = IndexFinder(m_proxyModel, QList<int>() << 0);

    int rowsRemoved = 1;
    rowCount = 40;
    signalRemoval("remove01", indexFinder, startRow, rowsRemoved, rowCount);

    startRow = 6;
    rowCount = 39;
    signalRemoval("remove02", indexFinder, startRow, rowsRemoved, rowCount);

    indexFinder = IndexFinder(m_proxyModel, QList<int>() << 0 << 5);
    startRow = 0;
    rowCount = 10;
    signalRemoval("remove03", indexFinder, startRow, rowsRemoved, rowCount);

    startRow = 8;
    rowCount = 9;
    signalRemoval("remove04", indexFinder, startRow, rowsRemoved, rowCount);

    startRow = 3;
    rowCount = 8;
    signalRemoval("remove05", indexFinder, startRow, rowsRemoved, rowCount);


    indexFinder = IndexFinder(m_proxyModel, QList<int>() << 0 << 5);
    startRow = 0;
    rowCount = 7;
    rowsRemoved = 7;
    signalRemoval("remove06", indexFinder, startRow, rowsRemoved, rowCount);

    indexFinder = IndexFinder(m_proxyModel, QList<int>() << 0);

    startRow = 4;
    rowCount = 38;
    rowsRemoved = 1;
    signalRemoval("remove07", indexFinder, startRow, rowsRemoved, rowCount);

  }

private:
  QItemSelectionModel *m_selectionModel;
  SelectionProxyModel *m_proxyModel;
};


QTEST_KDEMAIN(SelectionProxyModelTest, GUI)
#include "selectionproxymodeltest.moc"

