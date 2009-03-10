#ifndef TESTAGENT_H
#define TESTAGENT_H

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/monitor.h>
//#include <libakonadi-xml.h>
#include "testbase.h"

using namespace Akonadi;

class TestAgent: public AkonadiTestBase
{
  Q_OBJECT
  
  private:
    Monitor monitorResource;
    Item mItem;
    //void openXML();

  private Q_SLOTS:
    void itemCompare(const Item &item);
    //void CollectionCompare(const Collection &c1,const Collection &c2);

  public Q_SLOTS:

    //Collection collectionFromXML(const QString &path);
    //Item itemFromXML(const QString &path);
     
    void agentMonitored(const QString &agentName);
/*
    void checkRemovedItem(const Item &item);
    void checkChangedItem(const Item &item);
*/
    void checkAddedItem(const Item &item, const Collection &col);
/*
    void checkMovedItem(const Item &item);
    void checkCollectionChanged(const Collection &col);
    void checkCollectionRemoved(const Collection &col); 
*/
};

#endif   
