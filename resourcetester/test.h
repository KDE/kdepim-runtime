#ifndef TESTAGENT_H
#define TESTAGENT_H

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/monitor.h>
//#include <libakonadi-xml.h>

using namespace Akonadi;

class TestAgent: public QObject
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
    void test();
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
