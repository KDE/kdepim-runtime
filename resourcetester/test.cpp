#include "test.h"
#include <QTest>
#include <QDebug>

void TestAgent::test()
{
  qDebug()<<"teste";
}

void TestAgent::check(Item item, Collection col, const char *signal, const char *slot)
{
  mItem = item;
  mCol = col;  

  connect(&monitorResource, signal, this, slot);
}

void TestAgent::entityCompare(const Entity &entity1, const Entity &entity2)
{
  QCOMPARE(entity1.id(), entity2.id());
  QCOMPARE(entity1.remoteId(), entity2.remoteId());
}

void TestAgent::itemCompare(const Item &item, const Collection &col)
{
  QCOMPARE(item.mimeType(), mItem.mimeType());
}

void TestAgent::agentMonitored(const QString &agentName)
{
  monitorResource.setResourceMonitored(agentName.toUtf8());
}

void TestAgent::checkAddedItem(const Item &item, const Collection &col)
{ 
  check(item, col, SIGNAL(itemAdded (const Akonadi::Item &, const Akonadi::Collection &)), 
                  SLOT(itemCompare(const Item &, const Collection &)));
}
