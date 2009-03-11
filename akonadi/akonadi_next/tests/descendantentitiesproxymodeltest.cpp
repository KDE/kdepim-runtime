
#include <QtTest>
#include <QtCore>
#include <qtest_kde.h>
#include <qtestevent.h>

#include "dynamictreemodel.h"
#include "../modeltest.h"
#include "../descendantentitiesproxymodel.h"

#include <QTreeView>

#include <kdebug.h>

using namespace Akonadi;

class TestProxyModel : public QObject
{
  Q_OBJECT

private slots:
  void testInsertionChangeAndRemoval();

};

void TestProxyModel::testInsertionChangeAndRemoval()
{
  DynamicTreeModel *model = new DynamicTreeModel(this);

  DescendantEntitiesProxyModel *proxy = new DescendantEntitiesProxyModel(this);
  proxy->setSourceModel(model);

  new ModelTest(proxy, this);

  QList<int> ancestorRows;

  ModelInsertCommand *ins;
  int max_runs = 5;
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


  ModelRemoveCommand *rem;
  rem = new ModelRemoveCommand(model, this);
  rem->setStartRow(2);
  rem->setEndRow(2);
  rem->doCommand();

  // If we get this far, modeltest didn't assert.
  QVERIFY(true);
}


QTEST_KDEMAIN(TestProxyModel, GUI)
#include "descendantentitiesproxymodeltest.moc"
