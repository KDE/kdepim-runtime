#ifndef EVENT_H
#define EVENT_H

#include <QList>
#include <QDateTime>
#include <QString>

struct Event
{
  typedef QList<Event> List;

  QDateTime start;
  QDateTime end;

  QString title;
};

#endif
