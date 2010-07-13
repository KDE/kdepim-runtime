/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef COMPACTCHANGEHELPER_H
#define COMPACTCHANGEHELPER_H

#include <QObject>

template <typename T> class QList;

namespace Akonadi {
  class Collection;
  class Item;

  typedef QList<Item> ItemList;
}

class CompactChangeHelper : public QObject
{
  Q_OBJECT

  public:
    explicit CompactChangeHelper( const QByteArray &sessionId, QObject *parent = 0 );

    ~CompactChangeHelper();

    void addChangedItems( const Akonadi::ItemList &items );

    QString currentRemoteId( const Akonadi::Item &item ) const;

    void checkCollectionChanged( const Akonadi::Collection &collection );

  private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void processNextBatch() )
    Q_PRIVATE_SLOT( d, void processNextItem() )
    Q_PRIVATE_SLOT( d, void itemFetchResult( KJob* ) )
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
