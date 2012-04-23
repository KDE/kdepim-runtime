/*
    Copyright (c) 2009 Andras Mantia <amantia@kde.org>
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>
    Copyright (c) 2012 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <kdebug.h>
#include <kmime/kmime_codecs.h>
#include <kolab/kolabobject.h>

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
    Kolab::KolabObjectReader reader(payload);
    if (reader.getType() == Kolab::ContactObject) {
      Akonadi::Item newItem(KABC::Addressee::mimeType());
      newItem.setRemoteId(QString::number(item.id()));
      newItem.setPayload(reader.getContact());
      newItems << newItem;
    } else if (reader.getType() == Kolab::DistlistObject) {
      Akonadi::Item newItem(KABC::ContactGroup::mimeType());
      newItem.setRemoteId(QString::number(item.id()));
      newItem.setPayload(reader.getDistlist());
      newItems << newItem;
    }
    if (checkForErrors(item.id())) {
      newItems.removeLast(); //TODO what are the implications of this?
    }
  }

  return newItems;
}

void AddressBookHandler::toKolabFormat(const Akonadi::Item& item, Akonadi::Item &imapItem)
{
  if (item.hasPayload<KABC::Addressee>()) {
    const KABC::Addressee &addressee = item.payload<KABC::Addressee>();
    
    const KMime::Message::Ptr &message = Kolab::KolabObjectWriter::writeContact(addressee, m_formatVersion);
    if (checkForErrors(item.id())) {
      return;
    }
    imapItem.setMimeType( "message/rfc822" );
    imapItem.setPayload(message);
  } else if (item.hasPayload<KABC::ContactGroup>()) {
    KABC::ContactGroup contactGroup = item.payload<KABC::ContactGroup>();
    
    const KMime::Message::Ptr &message = Kolab::KolabObjectWriter::writeDistlist(contactGroup, m_formatVersion);
    if (checkForErrors(item.id())) {
      return;
    }
    imapItem.setMimeType( "message/rfc822" );
    imapItem.setPayload(message);
  } else {
    kWarning() << "Payload is neither a KABC::Addressee nor KABC::ContactGroup!";
    return;
  }
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
