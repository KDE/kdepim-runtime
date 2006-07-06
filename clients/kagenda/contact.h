#ifndef CONTACT_H
#define CONTACT_H

#include <QList>
#include <QString>

struct Contact
{
  typedef QList<Contact> List;

  QString name;
  QString email;
  QString phone;
};

#endif
