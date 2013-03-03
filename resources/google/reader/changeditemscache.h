/*
    Copyright (C) 2013  Daniel Vrátil <dan@progdan.cz>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef CHANGEDITEMSCACHE_H
#define CHANGEDITEMSCACHE_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <KDE/Akonadi/Item>

class ChangedItemsCache : public QObject
{
  Q_OBJECT
  public:
    explicit ChangedItemsCache( const QString &resourceName, QObject *parent = 0 );
    virtual ~ChangedItemsCache();

    void addItem( const Akonadi::Item &item );
    void clear();

    bool isEmpty() const;
    QStringList readItems() const;
    QStringList unreadItems() const;

  private:
    void write();

    QStringList m_readItems;
    QStringList m_unreadItems;
    QString m_cacheFile;
};

#endif // CHANGEDITEMSCACHE_H
