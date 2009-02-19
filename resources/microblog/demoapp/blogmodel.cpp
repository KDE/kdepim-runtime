/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "blogmodel.h"
#include "../statusitem.h"

#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kio/job.h>

#include <QtCore/QDebug>

using namespace Akonadi;

class BlogModel::Private
{
public:
};

BlogModel::BlogModel( QObject *parent ) :
        ItemModel( parent ),
        d( new Private() )
{
    fetchScope().fetchFullPayload();
}

BlogModel::~BlogModel( )
{
    delete d;
}

int BlogModel::columnCount( const QModelIndex & parent ) const
{
    if ( !parent.isValid() )
        return 4; // keep in sync with the column type enum

    return 0;
}

QVariant BlogModel::data( const QModelIndex & index, int role ) const
{
    if ( role != Qt::DisplayRole )
        return QVariant();
    if ( !index.isValid() )
        return QVariant();
    if ( index.row() >= rowCount() )
        return QVariant();
    Item item = itemForIndex( index );
    if ( !item.hasPayload<StatusItem>() )
        return QVariant();
    StatusItem msg = item.payload<StatusItem>();
    Collection col = collection();
    if ( col.remoteId() == "home" || col.remoteId() == "replies" || 
         col.remoteId() == "favorites" ) {
       switch ( index.column() ) {
        case User:
            return msg.value( "user_screen_name" );
        case Text:
            return msg.text();
        case Date:
            return msg.date();
        case Picture:
            return msg.value( "user_-_profile_image_url" );
        default:
            return QVariant();
        }
    }
    if ( col.remoteId() == "inbox" ) {
       switch ( index.column() ) {
        case User:
            return msg.value( "sender_screen_name" );
        case Text:
            return msg.text();
        case Date:
            return msg.date();
        case Picture:
            return msg.value( "sender_-_profile_image_url" );
        default:
            return QVariant();
        }
    }
    if ( col.remoteId() == "outbox" ) {
       switch ( index.column() ) {
        case User:
            return msg.value( "recipient_screen_name" );
        case Text:
            return msg.text();
        case Date:
            return msg.date();
        case Picture:
            return msg.value( "recipient_-_profile_image_url" );
        default:
            return QVariant();
        }
    }
    return ItemModel::data( index, role );
}

QVariant BlogModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
        switch ( section ) {
        case User:
            return i18nc( "@title:column, message (e.g. email) subject", "User" );
        case Text:
            return i18nc( "@title:column, sender of message (e.g. email)", "Text" );
        case Date:
            return i18nc( "@title:column, message (e.g. email) timestamp", "Date" );
        case Picture:
            return i18nc( "@title:column, message (e.g. email) timestamp", "Picture" );
        default:
            return QString();
        }
    }
    return ItemModel::headerData( section, orientation, role );
}

#include "blogmodel.moc"
