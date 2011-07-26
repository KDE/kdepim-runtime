/*
    Copyright (C) 2008    Dmitry Ivanov <vonami@gmail.com>

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


#include "akonadi_serializer_rss.h"

#include <krss/item.h>
#include <krss/rssitem.h>

#include <akonadi/item.h>

#include <QtCore/qplugin.h>

using namespace KRss;

bool SerializerPluginRss::deserialize( Akonadi::Item& item, const QByteArray& label, QIODevice& data, int version )
{
    Q_UNUSED( version )

    if ( label != Akonadi::Item::FullPayload && label != KRss::Item::HeadersPart &&
         label != KRss::Item::ContentPart )
        return false;

    RssItem rssItem;
    if ( item.hasPayload<RssItem>() ) {
        rssItem = item.payload<RssItem>();
    }

    if ( label == Akonadi::Item::FullPayload ) {
        m_serializer.deserialize( rssItem, data.readAll(), RssItemSerializer::Full );
    }
    else if ( label == KRss::Item::HeadersPart ) {
        m_serializer.deserialize( rssItem, data.readAll(), RssItemSerializer::Headers );
    }
    else if ( label == KRss::Item::ContentPart ) {
        m_serializer.deserialize( rssItem, data.readAll(), RssItemSerializer::Content );
    }

    item.setPayload<RssItem>( rssItem );
    return true;
}

void SerializerPluginRss::serialize( const Akonadi::Item& item, const QByteArray& label, QIODevice& data, int &version )
{
    Q_UNUSED( version )

    if ( label != Akonadi::Item::FullPayload && label != KRss::Item::HeadersPart &&
         label != KRss::Item::ContentPart )
        return;

    if ( item.mimeType() != Item::mimeType() || !item.hasPayload<RssItem>() )
        return;

    const RssItem rssItem = item.payload<RssItem>();
    QByteArray array;

    if ( label == Akonadi::Item::FullPayload ) {
        m_serializer.serialize( rssItem, array, RssItemSerializer::Full );
    }
    else if ( label == KRss::Item::HeadersPart ) {
        m_serializer.serialize( rssItem, array, RssItemSerializer::Headers );
    }
    else if ( label == KRss::Item::ContentPart ) {
        m_serializer.serialize( rssItem, array, RssItemSerializer::Content );
    }

    data.write( array );
}

QSet<QByteArray> SerializerPluginRss::parts( const Akonadi::Item& item ) const
{
    QSet<QByteArray> partIdentifiers;

    if ( item.hasPayload<RssItem>() ) {
        const RssItem rssItem = item.payload<RssItem>();

        if ( rssItem.headersLoaded() ) {
            partIdentifiers << KRss::Item::HeadersPart ;
        }

        if ( rssItem.contentLoaded() ) {
            partIdentifiers << KRss::Item::ContentPart;
        }

        if ( rssItem.headersLoaded() && rssItem.contentLoaded() ) {
            partIdentifiers << Akonadi::Item::FullPayload;
        }
    }

    return partIdentifiers;
}

Q_EXPORT_PLUGIN2( akonadi_serializer_rss, SerializerPluginRss )

#include "akonadi_serializer_rss.moc"
