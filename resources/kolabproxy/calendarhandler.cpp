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

#include "calendarhandler.h"
#include "event.h"

#include <kdebug.h>

#include <QBuffer>


CalendarHandler::CalendarHandler()
{
}


CalendarHandler::~CalendarHandler()
{
}

Akonadi::Item::List CalendarHandler::translateItems(const Akonadi::Item::List & items)
{
  Akonadi::Item::List newItems;
  Q_FOREACH(Akonadi::Item item, items)
  {
//     kDebug() << item.id();
    MessagePtr payload = item.payload<MessagePtr>();
    EventPtr event(new KCal::Event);
    if (calendarFromKolab(payload, event)) {
      Akonadi::Item newItem("text/directory");
      newItem.setRemoteId(QString::number(item.id()));
      newItem.setPayload<EventPtr>(event);
      newItems << newItem;
    }
  }

  return newItems;
}

bool CalendarHandler::calendarFromKolab(MessagePtr data, EventPtr &calendarEvent)
{
  KMime::Content *xmlContent  = findContent(data, "application/x-vnd.kolab.contact");
  if (xmlContent) {
    QByteArray xmlData = xmlContent->decodedContent();
//     kDebug() << "xmlData " << xmlData;

    //FIXME: read the tz
    EventPtr e(Kolab::Event::xmlToEvent(QString::fromLatin1(xmlData), QString() ));
//     QString pictureAttachmentName = contact.pictureAttachmentName();
//     if (!pictureAttachmentName.isEmpty()) {
//       KMime::Content *imgContent = findContent(data, "image/png");
//       if (imgContent) {
//         QByteArray imgData = imgContent->decodedContent();
//         QBuffer buffer(&imgData);
//         buffer.open(QIODevice::ReadOnly);
//         QImage image;
//         image.load(&buffer, "PNG");
//         contact.setPicture(image);
//       }
//     }
    calendarEvent = e;
//     event->saveTo(&calendarEvent);
    return true;
  }
  return false;
}

KMime::Content* CalendarHandler::findContent(MessagePtr data, const QByteArray &type)
{
  KMime::Content::List list = data->contents();
  Q_FOREACH(KMime::Content *c, list)
  {
    if (c->contentType()->mimeType() ==  type)
      return c;
  }
  return 0L;

}

Akonadi::Item CalendarHandler::toKolabFormat(const Akonadi::Item& item)
{
/*  KABC::Addressee addressee = item.payload<KABC::Addressee>();
  Kolab::Contact contact(&addressee, 0);
  
  Akonadi::Item imapItem;
  imapItem.setMimeType( "message/rfc822" );

  MessagePtr message(new KMime::Message);
  QString header;
  header += "From: " + addressee.fullEmail() + "\n";
  header += "Subject: " + addressee.uid() + "\n";
//   header += "Date: " + QDateTime::currentDateTime().toString(Qt::ISODate) + "\n";
  header += "User-Agent: Akonadi Kolab Proxy Resource \n";
  header += "MIME-Version: 1.0\n";
  header += "X-Kolab-Type: application/x-vnd.kolab.contact\n\n\n";
  message->setContent(header.toLatin1());

  KMime::Content *content = new KMime::Content();
  QByteArray contentData = QByteArray("Content-Type: text/plain; charset=\"us-ascii\"\nContent-Transfer-Encoding: 7bit\n\n") +
  "This is a Kolab Groupware object.\n" +
  "To view this object you will need an email client that can understand the Kolab Groupware format.\n" +
  "For a list of such email clients please visit\n"
  "http://www.kolab.org/kolab2-clients.html\n";
  content->setContent(contentData);
  message->addContent(content);

  content = new KMime::Content();
  header = "Content-Type: application/x-vnd.kolab.contact; name=\"kolab.xml\"\n";
  header += "Content-Transfer-Encoding: 7bit\n"; //FIXME 7bit???
  header += "Content-Disposition: attachment; filename=\"kolab.xml\"";
  content->setHead(header.toLatin1());
  content->setBody(contact.saveXML().toUtf8());
  message->addContent(content);

  if (!contact.pictureAttachmentName().isEmpty()) {
    header = "Content-Type: image/png; name=\"kolab-picture.png\"\n";
    header += "Content-Transfer-Encoding: base64\n";
    header += "Content-Disposition: attachment; filename=\"kolab-picture.png\"";
    content = new KMime::Content();
    content->setHead(header.toLatin1());
    QByteArray pic;
    QBuffer buffer(&pic);
    buffer.open(QIODevice::WriteOnly);
    contact.picture().save(&buffer, "PNG");
    buffer.close();
    content->setBody(pic.toBase64());
    message->addContent(content);

  }

  imapItem.setPayload<MessagePtr>(message);

  return imapItem;*/
}