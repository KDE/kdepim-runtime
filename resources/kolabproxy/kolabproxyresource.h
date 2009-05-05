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
#include <boost/shared_ptr.hpp>
#include <kmime/kmime_message.h>
typedef boost::shared_ptr<KMime::Message> MessagePtr;


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
    void imapCollectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent);
    void imapCollectionRemoved(const Akonadi::Collection &collection);
    void imapCollectionChanged(const Akonadi::Collection &collection);
    void createAddressEntry(const Akonadi::Item::List & addrs);
    void itemCreatedDone(KJob *job);
    void collectionFetchDone(KJob *job);

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );
    bool addresseFromKolab(MessagePtr data, KABC::Addressee &addressee);
    Akonadi::Collection createCollection(const Akonadi::Collection& imapCollection);
    KMime::Content *findContent(MessagePtr data, const QByteArray &type);

    Akonadi::Monitor *m_monitor;
    Akonadi::Monitor *m_colectionMonitor;
    QStringList m_managedCollections;
    QList<Akonadi::Item::Id> m_monitoredCollections;
    QMap<KJob*, QString> m_ids;
    QMap<KJob*, Akonadi::Item> m_items;
    QList<Akonadi::Item::Id> m_excludeAppend;
};

#endif
