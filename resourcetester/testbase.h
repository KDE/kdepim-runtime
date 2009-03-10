#ifndef TESTBASE_H
#define TESTBASE_H

#include <akonadi/entity.h>


class AkonadiTestBase
{
  private:
    Akonadi::Entity mEntity;

  protected:
    void compareEntity(const Akonadi::Entity &entity);
    //void checkEntity();
};

#endif
