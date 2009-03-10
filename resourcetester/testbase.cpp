#include "testbase.h"
#include <QTest>

void AkonadiTestBase::compareEntity(const Akonadi::Entity &entity)
{
  QCOMPARE(entity.id(), mEntity.id());
  QCOMPARE(entity.remoteId(), mEntity.remoteId());

}  
/*
void TestBase::checkEntity()
{
  mItem = item;

  connect(&monitorResource, SIGNAL(itemAdded( const Akonadi::Item &, const Akonadi::Collection &)), this, SLOT(ItemCompare(Item &,Collection &)));
}*/
