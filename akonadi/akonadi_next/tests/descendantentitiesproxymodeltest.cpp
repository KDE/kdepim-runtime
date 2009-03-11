
#include <QtTest>
#include <QtCore>
#include <qtest_kde.h>
#include <qtestevent.h>

#include <kdebug.h>

class TestProxyModel : public QObject
{
  Q_OBJECT

private slots:
  void testInsertion();

};

void TestProxyModel::testInsertion()
{
  QVERIFY(true);
}


QTEST_KDEMAIN(TestProxyModel, GUI)
#include "descendantentitiesproxymodeltest.moc"
