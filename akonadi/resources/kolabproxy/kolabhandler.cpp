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
#include "notehandler.h"

#include <akonadi/collection.h>
#include <kcal/kcalmimetypevisitor.h>
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
    return new NotesHandler();
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
  } else if ( contentMimeTypes.contains( "application/x-vnd.akonadi.note" ) ) {
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

KMime::Content* KolabHandler::findContentByType(const KMime::Message::Ptr &data, const QByteArray &type)
{
  const KMime::Content::List list = data->contents();
  Q_FOREACH(KMime::Content *c, list)
  {
    if (c->contentType()->mimeType() ==  type)
      return c;
  }
  return 0L;

}

KMime::Content* KolabHandler::findContentByName(const KMime::Message::Ptr &data, const QString &name, QByteArray &type)
{
  const KMime::Content::List list = data->contents();
  Q_FOREACH(KMime::Content *c, list)
  {
    if ( c->contentType()->name() == name ) {
      type = c->contentType()->mimeType();
      return c;
    }
  }
  return 0L;

}


KMime::Content* KolabHandler::createExplanationPart()
{
  KMime::Content *content = new KMime::Content();
  content->contentType()->setMimeType( "text/plain" );
  content->contentType()->setCharset( "us-ascii" );
  content->contentTransferEncoding()->setEncoding( KMime::Headers::CE7Bit );
  content->setBody( "This is a Kolab Groupware object.\n"
                    "To view this object you will need an email client that can understand the Kolab Groupware format.\n"
                    "For a list of such email clients please visit\n"
                    "http://www.kolab.org/kolab2-clients.html\n" );
  return content;
}


KMime::Message::Ptr KolabHandler::createMessage(const QString& mimeType)
{
  KMime::Message::Ptr message( new KMime::Message );
  message->date()->setDateTime( KDateTime::currentLocalDateTime() );
  KMime::Headers::Generic *h = new KMime::Headers::Generic( "X-Kolab-Type", message.get(), mimeType, "utf-8" );
  message->appendHeader( h );
  message->userAgent()->from7BitString( "Akonadi Kolab Proxy Resource" );
  message->contentType()->setMimeType( "multipart/mixed" );
  message->contentType()->setBoundary( KMime::multiPartBoundary() );

  message->addContent( createExplanationPart() );
  return message;
}


KMime::Content* KolabHandler::createMainPart(const QString& mimeType, const QByteArray& decodedContent)
{
  KMime::Content* content = new KMime::Content();
  content->contentType()->setMimeType( mimeType.toLatin1() );
  content->contentType()->setName( "kolab.xml", "us-ascii" );
  content->contentTransferEncoding()->setEncoding( KMime::Headers::CEquPr );
  content->contentDisposition()->setDisposition( KMime::Headers::CDattachment );
  content->contentDisposition()->setFilename( "kolab.xml" );
  content->setBody( decodedContent );
  return content;
}

KMime::Content* KolabHandler::createAttachmentPart(const QString& mimeType, const QString& fileName, const QByteArray& decodedContent)
{
  KMime::Content* content = new KMime::Content();
  content->contentType()->setMimeType( mimeType.toLatin1() );
  content->contentType()->setName( fileName, "us-ascii" );
  content->contentTransferEncoding()->setEncoding( KMime::Headers::CEbase64 );
  content->contentDisposition()->setDisposition( KMime::Headers::CDattachment );
  content->contentDisposition()->setFilename( fileName );
  content->setBody( decodedContent );
  return content;
}
