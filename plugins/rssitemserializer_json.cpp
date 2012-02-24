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
#include <QMap>
#include <QVariantMap>

#include <qjson/parser.h>
#include <qjson/serializer.h>

using namespace boost;
using namespace KRss;

static QVariantMap serialize( const Enclosure& e ) {
    QVariantMap h;
    h.insert( QLatin1String( "duration" ), e.duration() );
    h.insert( QLatin1String( "length" ), e.length() );
    h.insert( QLatin1String( "title" ), e.title() );
    h.insert( QLatin1String( "type" ), e.type() );
    h.insert( QLatin1String( "url" ), e.url() );
    return h;
}

static QVariantMap serialize( const Person& p ) {
    QVariantMap h;
    h.insert( QLatin1String( "email" ), p.email() );
    h.insert( QLatin1String( "name" ), p.name() );
    h.insert( QLatin1String( "uri" ), p.uri() );
    return h;
}

static QVariantMap serialize( const Category& c ) {
    QVariantMap h;
    h.insert( QLatin1String( "label"), c.label() );
    h.insert( QLatin1String( "scheme" ), c.scheme() );
    h.insert( QLatin1String( "term" ), c.term() );
    return h;
}

template <typename T>
static QVariantList doSerialize( const QList<T>& l ) {
    QVariantList list;
    Q_FOREACH( const T& i, l )
        list.append( serialize( i ) ); 
    return list;
}

static Category deserializeCategory( const QVariantMap& h ) {
    Category c;
    c.setLabel( h.value( QLatin1String( "label" ) ).toString() );
    c.setScheme( h.value( QLatin1String( "scheme" ) ).toString() );
    c.setTerm( h.value( QLatin1String( "term" ) ).toString() );
    return c;
}


static Enclosure deserializeEnclosure( const QVariantMap& h ) {
    Enclosure e;
    e.setDuration( h.value( QLatin1String( "duration" ) ).toInt() );
    e.setLength( h.value( QLatin1String( "length" ) ).toInt() );
    e.setTitle( h.value( QLatin1String( "title" ) ).toString() );
    e.setType( h.value( QLatin1String( "type" ) ).toString() );
    e.setUrl( h.value( QLatin1String( "url" ) ).toString() );
    return e;
}


static Person deserializePerson( const QVariantMap& h ) {
    Person p;
    p.setEmail( h.value( QLatin1String( "email" ) ).toString() );
    p.setName( h.value( QLatin1String( "name" ) ).toString() );
    p.setUri( h.value( QLatin1String( "uri" ) ).toString() );
    return p;
}

static QList<Person> deserializePersons( const QVariantList& h ) {
    QList<Person> l;
    Q_FOREACH( const QVariant& i, h )
        l.push_back( deserializePerson( i.toMap() ) );
    return l;
}

static QList<Category> deserializeCategories( const QVariantList& h ) {
    QList<Category> l;
    Q_FOREACH( const QVariant& i, h )
        l.push_back( deserializeCategory( i.toMap() ) );
    return l;
}

static QList<Enclosure> deserializeEnclosures( const QVariantList& h ) {
    QList<Enclosure> l;
    Q_FOREACH( const QVariant& i, h )
        l.push_back( deserializeEnclosure( i.toMap() ) );
    return l;
}

void RssItemSerializer::serialize( const RssItem& item, QByteArray& ba, ItemPart part ) {
    QVariantMap hash;
    //hash.insert( QLatin1String( "hash" ), QVariant::fromValue( item.hash() ) );
    hash.insert( QLatin1String( "guidIsHash" ), item.guidIsHash() );
    hash.insert( QLatin1String( "title" ), item.title() );
    hash.insert( QLatin1String( "link" ), item.link() );
    hash.insert( QLatin1String( "description" ), item.description() );
    hash.insert( QLatin1String( "content" ), item.content() );
    hash.insert( QLatin1String( "language" ), item.language() );
    hash.insert( QLatin1String( "datePublished" ), item.datePublished().toString() );
    hash.insert( QLatin1String( "dateUpdated" ), item.dateUpdated().toString() );
    hash.insert( QLatin1String( "guid" ), item.guid() );
    hash.insert( QLatin1String( "commentsCount" ), item.commentsCount() );
    hash.insert( QLatin1String( "commentPostUri" ), item.commentPostUri() );
    hash.insert( QLatin1String( "commentsFeed" ), item.commentsFeed() );
    hash.insert( QLatin1String( "commentsLink" ), item.commentsLink() );
    hash.insert( QLatin1String( "enclosures" ), QVariant( doSerialize( item.enclosures() ) ) );
    hash.insert( QLatin1String( "categories" ), doSerialize( item.categories() ) );
    hash.insert( QLatin1String( "authors" ), doSerialize( item.authors() ) );
#if 0    
    const QHash<QString, QString> ap = item.customProperties();
    stream << static_cast<quint32>( ap.size() );
    Q_FOREACH( const QString& key, ap.keys() )
            stream << key << ap.value( key );
#endif
    QJSon::Serializer serializer;
    ba = serializer.serialize( hash );
    qDebug() << ba;
}

bool RssItemSerializer::deserialize( RssItem& item, const QByteArray& ba, ItemPart part ) {
    QJSon::Parser parser;
    bool ok;
    QVariantMap hash = parser.parse( ba, &ok ).toMap();
    if ( !ok )
        return false;
    //item.setHash( hash.value( QLatin1String( "hash" ) ).value<qint64>() );
    item.setGuidIsHash( hash.value( QLatin1String( "guidIsHash" ) ).toBool() );
    item.setTitle( hash.value( QLatin1String( "title" ) ).toString() );
    item.setLink( hash.value( QLatin1String( "link" ) ).toString() );
    item.setDescription( hash.value( QLatin1String( "description" ) ).toString() );
    item.setContent( hash.value( QLatin1String( "content" ) ).toString() );
    item.setLanguage( hash.value( QLatin1String( "language" ) ).toString() );
    item.setDatePublished( KDateTime::fromString( hash.value( QLatin1String( "datePublished" ) ).toString() ) );
    item.setDateUpdated( KDateTime::fromString( hash.value( QLatin1String( "dateUpdated" ) ).toString() ) );
    item.setGuid( hash.value( QLatin1String( "guid" ) ).toString() );
    item.setCommentsCount( hash.value( QLatin1String( "commentsCount" ) ).toInt() );
    item.setCommentPostUri( hash.value( QLatin1String( "commentPostUri" ) ).toString() );
    item.setCommentsFeed( hash.value( QLatin1String( "commentsFeed" ) ).toString() );
    item.setCommentsLink( hash.value( QLatin1String( "commentsLink" ) ).toString() );
    item.setEnclosures( deserializeEnclosures( hash.value( QLatin1String( "enclosures" ) ).toList() ) );
    item.setCategories( deserializeCategories( hash.value( QLatin1String( "categories" ) ).toList() ) );
    item.setAuthors( deserializePersons( hash.value( QLatin1String( "authors" ) ).toList() ) );
    return true;
}

#undef READ
#undef READ2
#undef READSTRING
#undef READSTRING2
#undef READDATE
