#ifndef SCRIPT_H
#define SCRIPT_H

#include <kross/core/action.h>
#include <QHash>

class Script:public QObject
{
  private:
    Kross::Action *action;

  public:
    Script();
    void configure(const QString &path, QHash<QString, QObject *> hash);
    void configure(const QString &path);
    void insertObject(QObject *object, const QString &objectName);
    void start();
};

#endif
