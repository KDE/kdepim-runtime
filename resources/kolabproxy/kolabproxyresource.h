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

#ifndef KOLABPROXYRESOURCE_H
#define KOLABPROXYRESOURCE_H

#include <akonadi/resourcebase.h>
#include <QStringList>

namespace Akonadi {
  class Monitor;
}

namespace KABC{
  class Addressee;
}

class KolabProxyResource : public Akonadi::ResourceBase,
                           public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    KolabProxyResource( const QString &id );
    ~KolabProxyResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    void imapItemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection);
    void imapItemRemoved(const Akonadi::Item &item);
    void createAddressEntry(const Akonadi::Item::List & addrs);
    void itemCreatedDone(KJob *job);
    void collectionFetchDone(KJob *job);

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );
    bool addresseFromKolab(const QByteArray &data, KABC::Addressee &addressee);

    Akonadi::Monitor *m_monitor;
    QStringList m_managedCollections;
    QString m_id;
    Akonadi::Item m_item;
};

#endif
