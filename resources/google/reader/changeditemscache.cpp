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

#include <KRss/Item>

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
            QString id;
            bool isRead;
            while ( !cache.atEnd() ) {
                cache >> id;
                cache >> isRead;

                if (isRead) {
                    m_readItems << id;
                } else {
                    m_unreadItems << id;
                }
            }
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
    if ( KRss::Item::isRead( item ) ) {
        m_readItems << item.remoteId();
    } else if ( KRss::Item::isUnread( item ) ) {
        m_unreadItems << item.remoteId();
    }
}

void ChangedItemsCache::clear()
{
    m_readItems.clear();
    m_unreadItems.clear();
}

QStringList ChangedItemsCache::readItems() const
{
    return m_readItems;
}

QStringList ChangedItemsCache::unreadItems() const
{
    return m_unreadItems;
}

bool ChangedItemsCache::isEmpty() const
{
    return m_readItems.isEmpty() && m_unreadItems.isEmpty();
}

void ChangedItemsCache::write()
{
    QFile cacheFile( m_cacheFile );

    if ( cacheFile.open( QIODevice::WriteOnly ) ) {
        QDataStream cache( &cacheFile );
        cache.setVersion( QDataStream::Qt_4_7 );
        Q_FOREACH( const QString &id, m_readItems ) {
            cache << m_readItems;
            cache << true;
        }
        Q_FOREACH( const QString &id, m_unreadItems ) {
            cache << m_unreadItems;
            cache << false;
        }
        cacheFile.close();
    }
}
