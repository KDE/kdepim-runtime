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

#include "addressbookhandler.h"
#include "contact.h"

#include <kabc/addressee.h>
#include <kdebug.h>
#include <kmime/kmime_codecs.h>

#include <QBuffer>


AddressBookHandler::AddressBookHandler(): KolabHandler()
{
  m_mimeType = "application/x-vnd.kolab.contact";
}


AddressBookHandler::~AddressBookHandler()
{
}


Akonadi::Item::List AddressBookHandler::translateItems(const Akonadi::Item::List & items)
{
  Akonadi::Item::List newItems;
  Q_FOREACH(Akonadi::Item item, items)
  {
//     kDebug() << item.id();
    if (!item.hasPayload<MessagePtr>()) {
      kWarning() << "Payload is not a MessagePtr!";
      continue;
    }
    MessagePtr payload = item.payload<MessagePtr>();
    KABC::Addressee addressee;
    if (addresseFromKolab(payload, addressee)) {
      Akonadi::Item newItem("text/directory");
      newItem.setRemoteId(QString::number(item.id()));
      newItem.setPayload(addressee);
      newItems << newItem;
    }
  }

  return newItems;
}

bool AddressBookHandler::addresseFromKolab(MessagePtr data, KABC::Addressee &addressee)
{
  KMime::Content *xmlContent  = findContentByType(data, m_mimeType);
  if (xmlContent) {
    QByteArray xmlData = xmlContent->decodedContent();
//     kDebug() << "xmlData " << xmlData;
    Kolab::Contact contact(QString::fromUtf8(xmlData));
    QString pictureAttachmentName = contact.pictureAttachmentName();
    if (!pictureAttachmentName.isEmpty()) {
      QByteArray type;
      KMime::Content *imgContent = findContentByName(data, "kolab-picture.png", type);
      if (imgContent) {
        QByteArray imgData = imgContent->decodedContent();
        QBuffer buffer(&imgData);
        buffer.open(QIODevice::ReadOnly);
        QImage image;
        image.load(&buffer, "PNG");
        contact.setPicture(image);
      }
    }

    QString logoAttachmentName = contact.logoAttachmentName();
    if (!logoAttachmentName.isEmpty()) {
      QByteArray type;
      KMime::Content *imgContent = findContentByName(data, "kolab-logo.png", type);
      if (imgContent) {
        QByteArray imgData = imgContent->decodedContent();
        QBuffer buffer(&imgData);
        buffer.open(QIODevice::ReadOnly);
        QImage image;
        image.load(&buffer, "PNG");
        contact.setLogo(image);
      }
    }

    QString soundAttachmentName = contact.soundAttachmentName();
    if (!soundAttachmentName.isEmpty()) {
      QByteArray type;
      KMime::Content *content = findContentByName(data, "sound", type);
      if (content) {
        QByteArray sData = content->decodedContent();
        contact.setSound(sData);
      }
    }
    contact.saveTo(&addressee);
    return true;
  }
  return false;
}


void AddressBookHandler::toKolabFormat(const Akonadi::Item& item, Akonadi::Item &imapItem)
{
  if (!item.hasPayload<KABC::Addressee>()) {
    kWarning() << "Payload is not a KABC::Addressee!";
    return;
  }
  KABC::Addressee addressee = item.payload<KABC::Addressee>();
  Kolab::Contact contact(&addressee, 0);

  imapItem.setMimeType( "message/rfc822" );

  MessagePtr message(new KMime::Message);
  QString header;
  header += "From: " + addressee.fullEmail() + "\n";
  header += "Subject: " + addressee.uid() + "\n";
  header += "Date: " + QDateTime::currentDateTime().toString(Qt::TextDate) + "\n";
  header += "User-Agent: Akonadi Kolab Proxy Resource \n";
  header += "MIME-Version: 1.0\n";
  header += "X-Kolab-Type: " + m_mimeType + "\n\n\n";
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
  header = "Content-Type: " + m_mimeType + "; name=\"kolab.xml\"\n";
  header += "Content-Transfer-Encoding: quoted-printable\n";
  header += "Content-Disposition: attachment; filename=\"kolab.xml\"";
  content->setHead(header.toLatin1());
  KMime::Codec *codec = KMime::Codec::codecForName( "quoted-printable" );
  content->setBody(codec->encode(contact.saveXML().toUtf8()));
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

  if (!contact.logoAttachmentName().isEmpty()) {
    header = "Content-Type: image/png; name=\"kolab-logo.png\"\n";
    header += "Content-Transfer-Encoding: base64\n";
    header += "Content-Disposition: attachment; filename=\"kolab-logo.png\"";
    content = new KMime::Content();
    content->setHead(header.toLatin1());
    QByteArray pic;
    QBuffer buffer(&pic);
    buffer.open(QIODevice::WriteOnly);
    contact.logo().save(&buffer, "PNG");
    buffer.close();
    content->setBody(pic.toBase64());
    message->addContent(content);
  }


  if (!contact.soundAttachmentName().isEmpty()) {
    header = "Content-Type: audio/unknown; name=\"sound\"\n";
    header += "Content-Transfer-Encoding: base64\n";
    header += "Content-Disposition: attachment; filename=\"sound\"";
    content = new KMime::Content();
    content->setHead(header.toLatin1());
    content->setBody(contact.sound().toBase64());
    message->addContent(content);
  }


  imapItem.setPayload(message);
}

QStringList AddressBookHandler::contentMimeTypes()
{
  return QStringList() << "text/directory";
}
