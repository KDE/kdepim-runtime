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

#include <microblog/statusitem.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kio/job.h>

#include <QtCore/QDebug>

using namespace Akonadi;
using namespace Microblog;

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
        return 1;

    return 0;
}

QVariant BlogModel::data( const QModelIndex & index, int role ) const
{
    if ( role != Qt::DisplayRole && role != Qt::EditRole && role < Qt::UserRole )
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

    if ( role == Qt::EditRole ) {
        return msg.date();
    }

    if ( role == Qt::DisplayRole )
        return msg.id();

    switch ( role ) {
    case Date:
        return msg.date().toString();
    case User:
        if ( role == Qt::UserRole+1 ) {
            if ( col.remoteId() == "home" || col.remoteId() == "replies" ||
                    col.remoteId() == "favorites" )
                return msg.value( "user_-_screen_name" );
            else if ( col.remoteId() == "inbox" )
                return msg.value( "sender_screen_name" );
            else if ( col.remoteId() == "outbox" )
                return msg.value( "recipient_screen_name" );
            else
                return QVariant();
        }
    case Text:
        return msg.text();
    case Picture:
        if ( role == Qt::UserRole+3 ) {
            if ( col.remoteId() == "home" || col.remoteId() == "replies" ||
                    col.remoteId() == "favorites" )
                return msg.value( "user_-_profile_image_url" );
            else if ( col.remoteId() == "inbox" )
                return msg.value( "sender_-_profile_image_url" );
            else if ( col.remoteId() == "outbox" )
                return msg.value( "recipient_-_profile_image_url" );
            else
                return QVariant();
        }
    default:
        return QVariant();
    }

    return ItemModel::data( index, role );
}

QVariant BlogModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
        return i18nc( "@title:column, item id", "Blogs by date" );
    }
    return ItemModel::headerData( section, orientation, role );
}

#include "blogmodel.moc"
