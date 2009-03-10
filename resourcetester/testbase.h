#ifndef TESTBASE_H
#define TESTBASE_H

#include <akonadi/entity.h>


class AkonadiTestBase:public QObject
{
  private:
    Akonadi::Entity mEntity;

  protected:
    void compareEntity(const Akonadi::Entity &entity);
    //void checkEntity();
};

#endif
