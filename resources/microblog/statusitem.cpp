/*
    Copyright (C) 2009 Omat Holding B.V. <info@omat.nl>

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

#include "statusitem.h"
#include <kdebug.h>

#include <QDomElement>

class StatusItem::Private : public QSharedData
{
public:
    Private() {}
    Private( const Private& other ) : QSharedData( other ) {
        data = other.data;
        status = other.status;
    }

public:
    void init();
    QByteArray data;
    QHash<QString,QString> status;
};

void StatusItem::Private::init()
{
    QDomDocument document;
    document.setContent( data );
    QDomElement root = document.documentElement();
    QDomNode node = root.firstChild();
    while ( !node.isNull() ) {
        const QString key = node.toElement().tagName();
        if ( key == "user" || key == "sender" || key == "recipient" ) {
            QDomNode node2 = node.firstChild();
            while ( !node2.isNull() ) {
                const QString key2 = node2.toElement().tagName();
                const QString val2 = node2.toElement().text();
                status[ key + "_-_" + key2 ] = val2;
                node2 = node2.nextSibling();
            }
        } else {
            const QString value = node.toElement().text();
            status[key] = value;
        }
        node = node.nextSibling();
    }
    //kDebug() << status;
}

StatusItem::StatusItem()  :  d( new Private )
{
}

StatusItem::StatusItem( const QByteArray &data ) : d( new Private )
{
    d->data = data;
    d->init();
}

StatusItem::StatusItem( const StatusItem& other ) : d( other.d )
{
}

StatusItem::~StatusItem()
{
}

StatusItem StatusItem::operator=( const StatusItem &other )
{
    if ( &other != this )
        d = other.d;

    return *this;
}

void StatusItem::setData( const QByteArray &data )
{
    d->data = data;
    d->init();
}


qlonglong StatusItem::id() const
{
    return d->status.value( "id" ).toLongLong();
}

QByteArray StatusItem::data() const
{
    return d->data;
}

QString StatusItem::value( const QString& value ) const
{
    return d->status.value( value );
}

QString StatusItem::text() const
{
    //linklocater
    return d->status.value( "text" );
}

QString StatusItem::date() const
{
    //return QDateTime
    return d->status.value( "created_at" );
}
