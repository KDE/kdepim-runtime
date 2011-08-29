/*
    Copyright (C) 2009    Dmitry Ivanov <vonami@gmail.com>

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

#include "util.h"

#include <krss/item.h>
#include <krss/rssitem.h>
#include <krss/person.h>
#include <krss/category.h>
#include <krss/enclosure.h>
//#include <krss/feedcollection.h>

#include <Akonadi/Collection>
#include <Syndication/Person>
#include <Syndication/Enclosure>
#include <Syndication/Category>
#include <KDateTime>
#include <QtCore/QString>
#include <QtCore/QByteArray>

#include <KDebug>
#include <QtCore/QMultiMap>
#include <QtCore/QMapIterator>
#include <QtXml/QDomElement>

namespace Util
{
// from akregator/src/utils.cpp
static inline int calcHash( const QString& str )
{
    const QByteArray array = str.toAscii();
    return qChecksum( array.constData(), array.size() );
}

static KRss::RssItem KRssResource::fromSyndicationItem( const Syndication::ItemPtr& syndItem )
{
    KRss::RssItem rssItem;
    rssItem.setHeadersLoaded( true );
    rssItem.setContentLoaded( true );

    rssItem.setGuidIsHash( syndItem->id().startsWith( QLatin1String("hash:") ) );
    //rssItem.setSourceFeedId(currentCollection().remoteId().toInt());
    rssItem.setTitle( syndItem->title() );
    rssItem.setLink( syndItem->link() );
    rssItem.setDescription( syndItem->description() );
    rssItem.setContent( syndItem->content() );
    KDateTime dt;
    if ( syndItem->datePublished() > 0 ) {
        dt.setTime_t( syndItem->datePublished() );
        rssItem.setDatePublished( dt );
    }
    else {
        rssItem.setDatePublished( KDateTime::currentLocalDateTime() );
    }
    dt.setTime_t( syndItem->dateUpdated() );
    rssItem.setDateUpdated( dt );
    rssItem.setGuid( syndItem->id() );
    rssItem.setLanguage( syndItem->language() );
    rssItem.setCommentsCount( syndItem->commentsCount() );
    rssItem.setCommentsLink( syndItem->commentsLink() );
    rssItem.setCommentsFeed( syndItem->commentsFeed() );
    rssItem.setCommentPostUri( syndItem->commentPostUri() );

    // Authors
    QList<KRss::Person> authors;
    const QList<Syndication::PersonPtr> syndAuthors = syndItem->authors();
    Q_FOREACH( const Syndication::PersonPtr& syndAuthor, syndAuthors ) {
        if ( !syndAuthor->isNull() ) {
            KRss::Person author;
            author.setName( syndAuthor->name() );
            author.setUri( syndAuthor->uri() );
            author.setEmail( syndAuthor->email() );
            authors.append( author );
        }
    }
    rssItem.setAuthors( authors );

    // Enclosures
    QList<KRss::Enclosure> enclosures;
    const QList<Syndication::EnclosurePtr> syndEnclosures = syndItem->enclosures();
    Q_FOREACH( const Syndication::EnclosurePtr& syncEnclosure, syndEnclosures ) {
        if ( !syncEnclosure->isNull() ) {
            KRss::Enclosure enclosure;
            enclosure.setUrl( syncEnclosure->url() );
            enclosure.setTitle( syncEnclosure->title());
            enclosure.setType( syncEnclosure->type() );
            enclosure.setLength( syncEnclosure->length() );
            enclosure.setDuration( syncEnclosure->duration() );
            enclosures.append( enclosure );
        }
    }
    rssItem.setEnclosures( enclosures );

    // Categories
    QList<KRss::Category> categories;
    const QList<Syndication::CategoryPtr> syndCategories = syndItem->categories();
    Q_FOREACH( const Syndication::CategoryPtr& syndCategory, syndCategories ) {
        if ( !syndCategory->isNull() ) {
            KRss::Category category;
            category.setTerm( syndCategory->term() );
            category.setScheme( syndCategory->scheme() );
            category.setLabel( syndCategory->label() );
            categories.append( category );
        }
    }
    rssItem.setCategories( categories );

    // Additional properties
    // it's a multi map but we store only the most recently inserted value
    const QMultiMap<QString, QDomElement> syndProperties = syndItem->additionalProperties();
    QMapIterator<QString, QDomElement> it( syndProperties );
    while ( it.hasNext() ) {
        it.next();
        rssItem.setCustomProperty( it.key(), it.value().text() );
    }

    rssItem.setHash( calcHash( syndItem->title() + syndItem->description() +
                               syndItem->content() + syndItem->link() ) );
    return rssItem;
}

/*
QString KRssResource::generateCollectionName( const Akonadi::Collection& collection )
{
    if ( static_cast<KRss::FeedCollection>( collection ).title().isEmpty() ) {
        return collection.remoteId();
    }

    return static_cast<KRss::FeedCollection>( collection ).title() + QLatin1Char( '-' ) + collection.remoteId();
}*/

}