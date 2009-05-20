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
//
#ifndef KOLABHANDLER_H
#define KOLABHANDLER_H

#include <akonadi/item.h>
#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KMime::Message> MessagePtr;


/**
	@author Andras Mantia <amantia@kde.org>
*/
class KolabHandler{
public:
  static KolabHandler *createHandler(const QByteArray& type, const QString& timezoneId);

  virtual ~KolabHandler();

  /**
   * Translates Kolab items into the items supported by the handler.
   * @param addrs
   * @return the translated items
   */
  virtual Akonadi::Item::List translateItems(const Akonadi::Item::List & addrs) = 0;

  /**
   * Translates an item into Kolab format.
   * @param item the item to be translated
   * @param imapItem the item that will hold the Kolab format payload data.
   */
  virtual void toKolabFormat(const Akonadi::Item &item, Akonadi::Item &imapItem) = 0;

  /**
   * Return the mimetypes for the collections managed by the handler.
   */
  virtual QStringList contentMimeTypes() = 0;

  virtual QByteArray mimeType() const;
  virtual void setMimeType(const QByteArray& type);

protected:
  KolabHandler(const QString& timezoneId);
  KMime::Content *findContentByType(MessagePtr data, const QByteArray &type);

  QByteArray m_mimeType;
  QString m_timezoneId;
};

#endif
