/*
 * Copyright (c) 2009  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "test.h"
#include <QTest>

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
