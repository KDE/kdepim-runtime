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

#include <akonadi/collection.h>
#include <akonadi/kcal/kcalmimetypevisitor.h>
#include <kabc/addressee.h>

KolabHandler::KolabHandler()
{
}

KolabHandler *KolabHandler::createHandler(const QByteArray& type)
{
  if (type ==  "contact.default" || type ==  "contact") {
    return new AddressBookHandler();
  } else if (type ==  "event.default" || type ==  "event") {
    return new CalendarHandler();
  } else if (type ==  "task.default" || type ==  "task") {
    return new TasksHandler();
  } else if (type ==  "journal.default" || type ==  "journal") {
    return new JournalHandler();
  } else if (type ==  "note.default" || type ==  "note") {
    JournalHandler *handler =  new JournalHandler();
    handler->setMimeType("application/x-vnd.kolab.note");
    return handler;
  } else {
    return 0L;
  }
}


QByteArray KolabHandler::kolabTypeForCollection(const Akonadi::Collection& collection)
{
  const QStringList contentMimeTypes = collection.contentMimeTypes();
  if ( contentMimeTypes.contains( KABC::Addressee::mimeType() ) ) {
    return "contact";
  } else if ( contentMimeTypes.contains( Akonadi::KCalMimeTypeVisitor::eventMimeType() ) ) {
    return "event";
  } else if ( contentMimeTypes.contains( Akonadi::KCalMimeTypeVisitor::todoMimeType() ) ) {
    return "task";
  } else if ( contentMimeTypes.contains( Akonadi::KCalMimeTypeVisitor::journalMimeType() ) ) {
    return "journal";
  } else if ( contentMimeTypes.contains( "application/x-vnd.akonadi.notes" ) ) {
    return "note";
  }
  return QByteArray();
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

