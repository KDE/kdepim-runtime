/*
    Akonadi Google - Reader Resource
    Copyright (C) 2012  Dan Vratil <dan@progdan.cz>

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


#include "changeditemscache.h"

#include <KDE/KApplication>
#include <KDE/KStandardDirs>

#include <QtCore/QFile>
#include <Akonadi/Collection>

ChangedItemsCache::ChangedItemsCache( const QString &resourceName, QObject *parent ):
    QObject(parent)
{
    QString cacheBase = QString( "%1_changeCache.dat" ).arg( resourceName );
    m_cacheFile = KStandardDirs::locateLocal( "data", cacheBase );
    QFile cacheFile( m_cacheFile );

    if ( cacheFile.exists() ) {
        if (cacheFile.open( QIODevice::ReadOnly ) ) {
            QDataStream cache( &cacheFile );
            cache >> m_cache;
            cacheFile.close();
        }
    }
}

ChangedItemsCache::~ChangedItemsCache()
{
    write();
}

void ChangedItemsCache::addItem( const Akonadi::Item &item )
{
    CacheItem cacheItem = {
        item.id(),
        item.remoteId(),
        item.parentCollection().remoteId()
    };

    m_cache.insert( cacheItem.id, cacheItem );
}

void ChangedItemsCache::addItem( const ChangedItemsCache::CacheItem& item )
{
    m_cache.insert( item.id, item );
}


void ChangedItemsCache::removeItem( const Akonadi::Entity::Id &id )
{
    m_cache.remove( id );
}

void ChangedItemsCache::clear()
{
    m_cache.clear();
}


bool ChangedItemsCache::isEmpty() const
{
    return m_cache.isEmpty();
}

ChangedItemsCache::CacheItem::List ChangedItemsCache::items() const
{
    return m_cache.values();
}

void ChangedItemsCache::write()
{
    QFile cacheFile( m_cacheFile );

    if ( cacheFile.open( QIODevice::WriteOnly ) ) {
        QDataStream cache( &cacheFile );
        cache.setVersion( QDataStream::Qt_4_7 );
        cache << m_cache;
        cacheFile.close();
    }
}

QDataStream &operator<<( QDataStream &out, const ChangedItemsCache::CacheItem &item )
{
    out << item.id;
    out << item.itemId;
    out << item.feedId;

    return out;
}

QDataStream &operator>>( QDataStream &in, ChangedItemsCache::CacheItem &item )
{
    in >> item.id;
    in >> item.itemId;
    in >> item.feedId;

    return in;
}



