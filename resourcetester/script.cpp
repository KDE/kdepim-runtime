#include "script.h"

Script::Script()
{
  action = new Kross::Action(this, "ResourceTester");
}  

void Script::configure(const QString &path, QHash<QString, QObject * > hash)
{
  action->setFile(path);
 
  QHashIterator<QString, QObject * > i(hash);

  while( i.hasNext()) {
    i.next();
    action->addObject( i.value(), i.key());
  }
}

void Script::configure(const QString &path)
{
  action->setFile(path);
}

void Script::insertObject(QObject *object, const QString &objectName)
{
  action->addObject(object, objectName);
}

void Script::start()
{
  action->trigger();
}
