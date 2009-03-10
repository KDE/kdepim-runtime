#include "test.h"
#include <QTest>

void TestAgent::itemCompare(const Item &item)
{
  compareEntity(item);
  QCOMPARE(item.mimeType(), mItem.mimeType());
}

void TestAgent::agentMonitored(const QString &agentName)
{
  monitorResource.setResourceMonitored(agentName.toUtf8());
}

void TestAgent::checkAddedItem(const Item &item, const Collection &col)
{
  mItem = item;

  connect(&monitorResource, SIGNAL(itemAdded( const Akonadi::Item &, const Akonadi::Collection &)), this, SLOT(ItemCompare(Item &,Collection &)));
}
