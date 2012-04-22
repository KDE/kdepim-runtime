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
#include <akonadi/collection.h>
#include <kmime/kmime_message.h>
#include <kolab/kolabobject.h>
#include "kolabdefs.h"

/**
	@author Andras Mantia <amantia@kde.org>
*/
class KolabHandler : public QObject{
  Q_OBJECT
public:
  typedef QSharedPointer<KolabHandler>  Ptr;
  static Ptr createHandler( const QByteArray& type, const Akonadi::Collection &imapCollection );
  static Ptr createHandler( const KolabV2::FolderType& type, const Akonadi::Collection &imapCollection );

  /**
    Returns the Kolab folder type for the given collection.
  */
  static QByteArray kolabTypeForMimeType( const QStringList &mimetypes );

  /**
   * Returns all mime types supported by Kolab.
   */
  static QStringList allSupportedMimeTypes();

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

  /**
   * Returns the default icon for this folder type.
   */
  virtual QString iconName() const = 0;

  virtual QByteArray mimeType() const;

  virtual void itemDeleted(const Akonadi::Item &item) { Q_UNUSED( item ); }
  virtual void itemAdded(const Akonadi::Item &item) { Q_UNUSED( item ); }
  virtual void reset() {}
  
  void setKolabFormatVersion(Kolab::Version);
  
  /**
   * Returns true if the current operation should be aborted and false if everything is ok.
   * 
   * Error handling strategy:
   * If an error happend, either:
   * - completely skip item => will be redownloaded (or will it be deleted from the server? (it shouldn0t))
   * - mark item as corrup => readonly, will not be written back and will be redownloaded
   * 
   * @param affectedItem The item which is currently being processed.
   */
  bool checkForErrors(Akonadi::Item::Id affectedItem);
Q_SIGNALS:
    void deleteItemFromImap(const Akonadi::Item& item);
    void addItemToImap(const Akonadi::Item& item, Akonadi::Entity::Id collectionId);

protected:
  explicit KolabHandler( const Akonadi::Collection &imapCollection );

  QByteArray m_mimeType;
  Akonadi::Collection m_imapCollection;
  Kolab::Version m_formatVersion;
};

#endif
