/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

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

#ifndef __MAILDIR_RESOURCE_H__
#define __MAILDIR_RESOURCE_H__

#include <resourcebase.h>

class QDBusMessage;

class MaildirResource : public Akonadi::ResourceBase
{
  Q_OBJECT

  public:
    MaildirResource( const QString &id );
    ~MaildirResource();

  public Q_SLOTS:
    virtual bool requestItemDelivery( const Akonadi::DataReference &ref, const QStringList &parts, const QDBusMessage &msg );
    virtual void configure();

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QStringList &parts );
    virtual void itemRemoved( const Akonadi::DataReference &ref );

    virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    virtual void collectionChanged( const Akonadi::Collection &collection );
    virtual void collectionRemoved( int id, const QString &remoteId );

    void retrieveCollections();
    void synchronizeCollection( const Akonadi::Collection &col );

  private:
    static QByteArray readHeader( const QString &fileName );
};

#endif
