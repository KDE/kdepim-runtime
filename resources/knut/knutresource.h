/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef KNUTRESOURCE_H
#define KNUTRESOURCE_H

#include <akonadi/resourcebase.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QDomDocument>

#include <xml/xmldocument.h>

#include "settings.h"

class QFileSystemWatcher;

class KnutResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    KnutResource( const QString &id );
    ~KnutResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &collection );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

    void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    void collectionChanged( const Akonadi::Collection &collection );
    void collectionRemoved( const Akonadi::Collection &collection );

    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    void itemRemoved( const Akonadi::Item &ref );


  private:
    QDomElement findElementByRid( const QString &rid ) const;

  private slots:
    void load();
    void save();

  private:
    Akonadi::XmlDocument mDocument;
    QFileSystemWatcher *mWatcher;
    KnutSettings *mSettings;
};

#endif
