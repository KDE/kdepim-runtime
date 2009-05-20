/*
    Copyright (c) 2009 Andras Mantia <amantia@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/
#include "kolabhandler.h"
#include "addressbookhandler.h"
#include "calendarhandler.h"
#include "taskshandler.h"
#include "journalhandler.h"

KolabHandler::KolabHandler(const QString& timezoneId)
{
  m_timezoneId = timezoneId;
}

KolabHandler *KolabHandler::createHandler(const QByteArray& type, const QString& timezoneId)
{
  if (type ==  "contact.default" || type ==  "contact") {
    return new AddressBookHandler(timezoneId);
  } else if (type ==  "event.default" || type ==  "event") {
    return new CalendarHandler(timezoneId);
  } else if (type ==  "task.default" || type ==  "task") {
    return new TasksHandler(timezoneId);
  } else if (type ==  "journal.default" || type ==  "journal") {
    return new JournalHandler(timezoneId);
  } else if (type ==  "note.default" || type ==  "note") {
    JournalHandler *handler =  new JournalHandler(timezoneId);
    handler->setMimeType("application/x-vnd.kolab.note");
    return handler;
  } else {
    return 0L;
  }
}


KolabHandler::~KolabHandler()
{
}

QByteArray KolabHandler::mimeType() const
{
  return m_mimeType;
}

void KolabHandler::setMimeType(const QByteArray &type)
{
  m_mimeType = type;
}


KMime::Content* KolabHandler::findContentByType(MessagePtr data, const QByteArray &type)
{
  KMime::Content::List list = data->contents();
  Q_FOREACH(KMime::Content *c, list)
  {
    if (c->contentType()->mimeType() ==  type)
      return c;
  }
  return 0L;

}

KMime::Content* KolabHandler::findContentByName(MessagePtr data, const QString &name, QByteArray &type)
{
  KMime::Content::List list = data->contents();
  Q_FOREACH(KMime::Content *c, list)
  {
    if (c->contentType()->name() == name)
      type = QByteArray(c->contentType()->type());
    return c;
  }
  return 0L;

}

