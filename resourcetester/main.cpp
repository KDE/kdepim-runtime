#include "test.h"
#include "script.h"
#include <akonadi/akonadi-xml.h>
int main()
{
  TestAgent *test = new TestAgent();
  Script *script = new Script();
  
  script->configure("/home/igor/test.js");
  script->insertObject(test, "TestResource");
  script->start();
  return 0;
}
