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

#ifndef TESTAGENT_H
#define TESTAGENT_H

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/monitor.h>

using namespace Akonadi;

class TestAgent : public QObject
{
  Q_OBJECT
  
  private:
    Monitor monitorResource;
    Item mItem;
    Collection mCol;
    //void openXML();

  private:
    void entityCompare(const Entity &entity1, const Entity &entity2);
    void itemCompare(const Item &item, const Collection &col);
    //void CollectionCompare(const Collection &c1,const Collection &c2);
    void check(Item item, Collection col, const char *signal, const char *slot);
   // void check(Collection, const char *signal, const char *slot);

  public Q_SLOTS:
    void agentMonitored(const QString &agentName);
/*
    void checkRemovedItem(const Item &item, const Collection &col);
    void checkChangedItem(const Item &item, const Collection &col;
*/
    void checkAddedItem(const Item &item, const Collection &col);
/*
    void checkMovedItem(const Item &item);
    void checkCollectionChanged(const Collection &col);
    void checkCollectionRemoved(const Collection &col); 
*/
};

#endif   
