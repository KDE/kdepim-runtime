/*
    KRssLocalResource - an Akonadi resource to store subscriptions in an opml file
    Copyright (C) 2013    Frank Osterfeld <osterfeld@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rsscollectionattribute.h"

#include <QByteArray>

QByteArray RssCollectionAttribute::type() const
{
    return "krsslocalresourceCollectionAttribute";
}

RssCollectionAttribute* RssCollectionAttribute::clone() const
{
    return new RssCollectionAttribute( *this );
}

RssCollectionAttribute::RssCollectionAttribute()
    : m_fetchCounter( 0 )
{
}

void RssCollectionAttribute::setFetchCounter( int c )
{
    m_fetchCounter = c;
}

int RssCollectionAttribute::fetchCounter() const
{
    return m_fetchCounter;
}

QByteArray RssCollectionAttribute::serialized() const
{
    return QByteArray::number( m_fetchCounter );
}

void RssCollectionAttribute::deserialize( const QByteArray& data )
{
    bool ok;
    const int v = data.toInt( &ok );
    if ( ok )
        m_fetchCounter = v;
}


