/*
 * This file is part of the krss library
 *
 * Copyright (C) 2009 Frank Osterfeld <osterfeld@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "rssitemserializer.h"
#include "rssitem.h"
#include "category.h"
#include "enclosure.h"
#include "person.h"

#include <KDateTime>

#include <QDataStream>

using namespace KRss;

void RssItemSerializer::serialize( const RssItem& item, QByteArray& ba, ItemPart part ) {
    QDataStream stream( &ba, QIODevice::WriteOnly );
    const bool writeHeaders = ( part & Headers ) != 0;
    const bool writeContent = ( part & Content ) != 0;

    if ( writeHeaders ) {
        stream << static_cast<qint64>( item.hash() )
               << item.guidIsHash()
               << item.title()
               << item.datePublished().toString()
               << item.dateUpdated().toString()
               << item.guid();
        stream << static_cast<quint32>( item.authors().count() );
        Q_FOREACH( const Person& i, item.authors() )
            stream << i.email() << i.name() << i.uri();
    }
    if ( writeContent ) {
        stream << item.content()
               << item.description()
               << item.link()
                << item.language()
               << static_cast<qint32>( item.commentsCount() )
               << item.commentPostUri()
               << item.commentsFeed()
               << item.commentsLink();
        stream << static_cast<quint32>( item.enclosures().count() );
        Q_FOREACH( const Enclosure& i, item.enclosures() )
            stream << static_cast<qint32>( i.duration() ) << static_cast<qint32>( i.length() ) << i.title() << i.type() << i.url();
        stream << static_cast<quint32>( item.categories().count() );
        Q_FOREACH( const Category& i, item.categories() )
            stream << i.label() << i.scheme() << i.term();
        const QHash<QString, QString> ap = item.customProperties();
        stream << static_cast<quint32>( ap.size() );
        Q_FOREACH( const QString& key, ap.keys() )
            stream << key << ap.value( key );
    }
}

#define READSTRING(name) QString name; stream >> name; item.set##name( name );
#define READSTRING2(name,target) QString name; stream >> name; target.set##name( name );
#define READ(type,name) type name; stream >> name; item.set##name( name );
#define READ2(type,name,target) type name; stream >> name; target.set##name( name );
#define READDATE(name) QString name; stream >> name; item.set##name( KDateTime::fromString( name ) );

bool RssItemSerializer::deserialize( RssItem& itemOut, const QByteArray& ba, ItemPart part ) {
    const bool readHeaders = ( part & Headers ) != 0;
    const bool readContent = ( part & Content ) != 0;

    QDataStream stream( ba );
    RssItem item;
    if ( readHeaders ) {
        READ(qint64,Hash)
        READ(bool,GuidIsHash)
        READSTRING(Title)
        READDATE(DatePublished)
        READDATE(DateUpdated)
        READSTRING(Guid)
        quint32 authorCount;
        stream >> authorCount;
        QList<Person> authors;
        for ( quint32 i = 0; i < authorCount; ++i ) {
            Person auth;
            READSTRING2(Email, auth)
            READSTRING2(Name, auth)
            READSTRING2(Uri, auth)
            authors.append( auth );
        }
        item.setAuthors( authors );
    }
    if ( readContent ) {
        READSTRING(Content)
        READSTRING(Description)
        READSTRING(Link)

        READSTRING(Language)
        READ(qint32,CommentsCount)
        READSTRING(CommentPostUri)
        READSTRING(CommentsFeed)
        READSTRING(CommentsLink)
        quint32 encCount;
        stream >> encCount;
        QList<Enclosure> enclosures;
        for ( quint32 i = 0; i < encCount; ++i ) {
            Enclosure enc;
            READ2(qint32, Duration, enc)
            READ2(qint32, Length, enc)
            READSTRING2(Title, enc)
            READSTRING2(Type, enc)
            READSTRING2(Url, enc)
            enclosures.append( enc );
        }
        item.setEnclosures( enclosures );

        quint32 catCount = 0;
        stream >> catCount;
        QList<Category> categories;
        for ( quint32 i = 0; i < catCount; ++i ) {
            Category cat;
            READSTRING2(Label, cat)
            READSTRING2(Scheme, cat)
            READSTRING2(Term, cat)
            categories.append( cat );
        }
        item.setCategories( categories );


        quint32 apCount;
        stream >> apCount;
        for ( quint32 i = 0; i < apCount; ++i ) {
            QString key;
            stream >> key;
            QString value;
            stream >> value;
            item.setCustomProperty( key, value );
        }
    }
    itemOut = item;

    if ( readHeaders )
        item.setHeadersLoaded( true );

    if ( readContent )
        item.setContentLoaded( true );

    return true;
}

#undef READ
#undef READ2
#undef READSTRING
#undef READSTRING2
#undef READDATE
