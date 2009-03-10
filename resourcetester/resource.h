#ifndef RESOURCE_H
#define RESOURCE_H

#include <akonadi/resourcebase.h>

class Resource: public Q_Object
{
  QOBJECT

  private:
    QString mResourceName;
    QString mPath;

  public Q_SLOTS:

    bool load();
    bool load(const QString &resourceName, const QString &path);

    void setName(const QString &resourceName);
    void setPath(const QString &path); 
};   

#endif
