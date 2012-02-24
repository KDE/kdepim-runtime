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

#include "krss.pb.h"

#include <KDateTime>

#include <QString>

#include <string>

using namespace KRss;
//TODO: in the plugin, do at init time:
// GOOGLE_PROTOBUF_VERIFY_VERSION;
//on plugin destruction, do
//google::protobuf::ShutdownProtobufLibrary();


static Protobuf::Person writePerson( const Person& o ) {
    Protobuf::Person r;
    r.set_name( o.name().toUtf8().constData() );
    r.set_email(  o.email().toUtf8().constData() );
    r.set_uri( o.uri().toUtf8().constData() );
    return r;
}

static Protobuf::Enclosure writeEnclosure( const Enclosure& o ) {
    Protobuf::Enclosure r;
    r.set_url( o.url().toUtf8().constData() );
    r.set_title( o.title().toUtf8().constData() );
    r.set_type( o.type().toUtf8().constData() );
    r.set_duration( o.duration() );
    r.set_length( o.length() );
    return r;
}

static Protobuf::Category writeCategory( const Category& o ) {
    Protobuf::Category r;
    r.set_label( o.label().toUtf8().constData() );
    r.set_scheme(  o.scheme().toUtf8().constData() );
    r.set_term( o.term().toUtf8().constData() );
    return r;
}

void RssItemSerializer::serialize( const KRss::RssItem& item, QByteArray& array, ItemPart part ) const
{
    const bool writeHeaders = part == Full || part == Headers;
    const bool writeContent = part == Full || part == Content;

    if ( writeHeaders ) {
        Protobuf::ItemHeader header;

        header.set_hash( item.hash() );
        header.set_guidishash( item.guidIsHash() );
        header.set_guid( item.guid().toUtf8().constData() );
        header.set_title( item.title().toUtf8().constData() );
        header.set_link( item.link().toUtf8().constData() );
        header.set_datepublished( item.datePublished().toTime_t() );
        header.set_dateupdated( item.dateUpdated().toTime_t() );
        header.set_language( item.language().toUtf8().constData() );
        Q_FOREACH( const Person& i, item.authors() )
            *header.add_authors() = writePerson( i );
        Q_FOREACH( const Enclosure& i, item.enclosures() )
            *header.add_enclosures() = writeEnclosure( i );
        Q_FOREACH( const Category& i, item.categories() )
            *header.add_categories() = writeCategory( i );
        header.set_commentscount( item.commentsCount() );
        header.set_commentslink( item.commentsLink().toUtf8().constData() );
        header.set_commentsfeed( item.commentsFeed().toUtf8().constData() );
        header.set_commentposturi( item.commentPostUri().toUtf8().constData() );

        std::string out;
        header.SerializeToString( &out );
        array.append( out.c_str(), out.size() );
    }

    if ( writeContent ) {
        Protobuf::ItemContent content;

        content.set_description( item.description().toUtf8().constData() );
        content.set_content( item.content().toUtf8().constData() );

        std::string out;
        content.SerializeToString( &out );
        array.append( out.c_str(), out.size() );
    }
}

static Enclosure readEnclosure( const Protobuf::Enclosure& pe ) {
    Enclosure enc;
    enc.setUrl( QString::fromUtf8( pe.url().c_str() ) );
    enc.setTitle( QString::fromUtf8( pe.title().c_str() ) );
    enc.setType( QString::fromUtf8( pe.type().c_str() ) );
    enc.setDuration( pe.duration() );
    enc.setLength( pe.length() );
    return enc;
}

static Category readCategory( const Protobuf::Category& pc ) {
    Category cat;
    cat.setLabel( QString::fromUtf8( pc.label().c_str() ) );
    cat.setScheme( QString::fromUtf8( pc.scheme().c_str() ) );
    cat.setTerm( QString::fromUtf8( pc.term().c_str() ) );
    return cat;
}


static Person readPerson( const Protobuf::Person& pp ) {
    Person p;
    p.setName( QString::fromUtf8( pp.name().c_str() ) );
    p.setEmail( QString::fromUtf8( pp.email().c_str() ) );
    p.setUri( QString::fromUtf8( pp.uri().c_str() ) );
    return p;
}

bool KRss::RssItemSerializer::deserialize( KRss::RssItem& item, const QByteArray& array, ItemPart part ) const
{
    const bool readHeaders = part == Full || part == Headers;
    const bool readContent = part == Full || part == Content;

    //TODO: these ifs are wrong, the Full case must be handled differently (FullItem message)

    if ( readHeaders ) {
        Protobuf::ItemHeader header;
        const bool parsed = header.ParsePartialFromArray( array.constData(), array.size() );
        if ( !parsed )
            return false;
        item.setHash( header.hash() );
        item.setGuidIsHash( header.guidishash() );
        item.setGuid( QString::fromUtf8( header.guid().c_str() ) );
        item.setTitle( QString::fromUtf8( header.title().c_str() ) );
        item.setLink( QString::fromUtf8( header.link().c_str() ) );
        KDateTime published;
        published.setTime_t( header.datepublished() );
        item.setDatePublished( published );
        KDateTime updated;
        updated.setTime_t( header.dateupdated() );
        item.setDateUpdated( updated );

        QList<Enclosure> enclosures;
        for ( int i = 0; i < header.enclosures_size(); ++i )
            enclosures.append( readEnclosure( header.enclosures( i ) ) );
        item.setEnclosures( enclosures );

       QList<Category> categories;
        for ( int i = 0; i < header.categories_size(); ++i )
            categories.append( readCategory( header.categories( i ) ) );
        item.setCategories( categories );

        QList<Person> authors;
        for ( int i = 0; i < header.authors_size(); ++i )
            authors.append( readPerson( header.authors( i ) ) );
        item.setAuthors( authors );

        item.setCommentsCount( header.commentscount() );
        item.setCommentsLink( QString::fromUtf8( header.commentslink().c_str() ) );
        item.setCommentsFeed( QString::fromUtf8( header.commentsfeed().c_str() ) );
        item.setCommentPostUri( QString::fromUtf8( header.commentposturi().c_str() ) );
    }

    item.setHeadersLoaded( readHeaders );

    if ( readContent ) {
        Protobuf::ItemContent content;
        const bool parsed = content.ParsePartialFromArray( array.constData(), array.size() );
        if ( !parsed )
            return false;
        item.setDescription( QString::fromUtf8( content.description().c_str() ) );
        item.setContent( QString::fromUtf8( content.content().c_str() ) );
    }

    item.setContentLoaded( readContent );

    return true;
}

