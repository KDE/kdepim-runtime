/*
    Copyright (c) 2009 Andras Mantia <amantia@kde.org>
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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
#include "distributionlist.h"

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <kdebug.h>
#include <kmime/kmime_codecs.h>

#include <QBuffer>


AddressBookHandler::AddressBookHandler( const Akonadi::Collection &imapCollection ) : KolabHandler( imapCollection )
{
  m_mimeType = "application/x-vnd.kolab.contact";
}


AddressBookHandler::~AddressBookHandler()
{
}


Akonadi::Item::List AddressBookHandler::translateItems(const Akonadi::Item::List & items)
{
  Akonadi::Item::List newItems;
  Q_FOREACH(const Akonadi::Item &item, items)
  {
//     kDebug() << item.id();
    if (!item.hasPayload<KMime::Message::Ptr>()) {
      kWarning() << "Payload is not a MessagePtr!";
      continue;
    }
    const KMime::Message::Ptr payload = item.payload<KMime::Message::Ptr>();
    KABC::Addressee addressee;
    KABC::ContactGroup contactGroup;
    if (addresseFromKolab(payload, addressee)) {
      Akonadi::Item newItem(KABC::Addressee::mimeType());
      newItem.setRemoteId(QString::number(item.id()));
      newItem.setPayload(addressee);
      newItems << newItem;
    } else if (contactGroupFromKolab(payload, contactGroup)) {
      Akonadi::Item newItem(KABC::ContactGroup::mimeType());
      newItem.setRemoteId(QString::number(item.id()));
      newItem.setPayload(contactGroup);
      newItems << newItem;
    }
  }

  return newItems;
}

bool AddressBookHandler::addresseFromKolab( const KMime::Message::Ptr &data, KABC::Addressee &addressee)
{
  KMime::Content *xmlContent  = findContentByType(data, m_mimeType);
  if (xmlContent) {
    QByteArray xmlData = xmlContent->decodedContent();
//     kDebug() << "xmlData " << xmlData;
    Kolab::Contact contact(QString::fromUtf8(xmlData));
    const QString pictureAttachmentName = contact.pictureAttachmentName();
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

bool AddressBookHandler::contactGroupFromKolab(const KMime::Message::Ptr &data, KABC::ContactGroup &contactGroup)
{
  KMime::Content *xmlContent  = findContentByType(data, m_mimeType + ".distlist");
  if (xmlContent) {
    QByteArray xmlData = xmlContent->decodedContent();
//     kDebug() << "xmlData " << xmlData;
    Kolab::DistributionList distList(QString::fromUtf8(xmlData));
    distList.saveTo(&contactGroup);
    return true;
  }
  return false;
}

void AddressBookHandler::toKolabFormat(const Akonadi::Item& item, Akonadi::Item &imapItem)
{
  if (item.hasPayload<KABC::Addressee>()) {
    KABC::Addressee addressee = item.payload<KABC::Addressee>();
    Kolab::Contact contact(&addressee);

    contactToKolabFormat(contact, imapItem);
  } else if (item.hasPayload<KABC::ContactGroup>()) {
    KABC::ContactGroup contactGroup = item.payload<KABC::ContactGroup>();
    Kolab::DistributionList distList(&contactGroup);

    distListToKolabFormat(distList, imapItem);
  } else {
    kWarning() << "Payload is neither a KABC::Addressee nor KABC::ContactGroup!";
    return;
  }
}

void AddressBookHandler::contactToKolabFormat(const Kolab::Contact& contact, Akonadi::Item &imapItem)
{
  imapItem.setMimeType( "message/rfc822" );

  KMime::Message::Ptr message = createMessage( m_mimeType );
  message->subject()->fromUnicodeString( contact.uid(), "utf-8" );
  message->from()->fromUnicodeString( contact.fullEmail(), "utf-8" );

  KMime::Content* content = createMainPart( m_mimeType, contact.saveXML().toUtf8() );
  message->addContent( content );

  if ( !contact.picture().isNull() ) {
    QByteArray pic;
    QBuffer buffer(&pic);
    buffer.open(QIODevice::WriteOnly);
    contact.picture().save(&buffer, "PNG");
    buffer.close();

    content = createAttachmentPart( "image/png", "kolab-picture.png", pic );
    message->addContent(content);
  }

  if ( !contact.logo().isNull() ) {
    QByteArray pic;
    QBuffer buffer(&pic);
    buffer.open(QIODevice::WriteOnly);
    contact.logo().save(&buffer, "PNG");
    buffer.close();

    content = createAttachmentPart( "image/png", "kolab-logo.png", pic );
    message->addContent(content);
  }


  if ( !contact.sound().isEmpty() ) {
    content = createAttachmentPart( "audio/unknown", "sound", contact.sound() );
    message->addContent(content);
  }

  message->assemble();
  imapItem.setPayload(message);
}

void AddressBookHandler::distListToKolabFormat(const Kolab::DistributionList& distList, Akonadi::Item &imapItem)
{
  imapItem.setMimeType( "message/rfc822" );

  KMime::Message::Ptr message = createMessage( m_mimeType + ".distlist" );
  message->subject()->fromUnicodeString( distList.uid(), "utf-8" );
  message->from()->fromUnicodeString( distList.uid(), "utf-8" );

  KMime::Content* content = createMainPart( m_mimeType + ".distlist", distList.saveXML().toUtf8() );
  message->addContent( content );

  message->assemble();
  imapItem.setPayload(message);
}

QStringList AddressBookHandler::contentMimeTypes()
{
  return QStringList() << KABC::Addressee::mimeType()
                       << KABC::ContactGroup::mimeType();
}

QString AddressBookHandler::iconName() const
{
  return QString::fromLatin1( "view-pim-contacts" );
}
