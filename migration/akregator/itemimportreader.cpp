/*
    Copyright (C) 2009    Frank Osterfeld <osterfeld@kde.org>

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

#include "itemimportreader.h"

#include <krss/category.h>
#include <krss/enclosure.h>
#include <krss/rssitem.h>
#include <krss/item.h>
#include <krss/person.h>

#include <syndication/atom/constants.h>
#include <syndication/constants.h>

#include <Akonadi/Item>
#include <KDateTime>

#include <QXmlStreamReader>
#include <QVariant>

namespace {

enum TextMode {
    PlainText,
    Html
};

static QString akregatorNamespace() {
    return QLatin1String("http://akregator.kde.org/StorageExporter#");
}

class Element
{
public:
    Element( const QString& ns_, const QString& name_, const QVariant& defaultValue_ = QVariant() ) : ns( ns_ ), name( name_ ), qualifiedName( ns + QLatin1Char(':') + name ), defaultValue( defaultValue_ )
    {
    }

    const QString ns;
    const QString name;
    const QString qualifiedName;
    const QVariant defaultValue;

    bool isNextIn( const QXmlStreamReader& reader ) const
    {
        return reader.isStartElement() && reader.name() == name && reader.namespaceUri() == ns;
    }
};

struct Elements
{
    Elements() : atomNS( Syndication::Atom::atom1Namespace() ),
                 akregatorNS( akregatorNamespace() ),
                 commentNS( Syndication::commentApiNamespace() ),
                 title( atomNS, QLatin1String("title"), QString() ),
                 summary( atomNS, QLatin1String("summary"), QString() ),
                 content( atomNS, QLatin1String("content"), QString() ),
                 link( atomNS, QLatin1String("link"), QString() ),
                 language( atomNS, QLatin1String("language"), QString() ),
                 guid( atomNS, QLatin1String("id"), QString() ),
                 published( atomNS, QLatin1String("published"), KDateTime().toString( KDateTime::ISODate ) ),
                 updated( atomNS, QLatin1String("updated"), KDateTime().toString( KDateTime::ISODate ) ),
                 commentsCount( Syndication::slashNamespace(), QLatin1String("comments"), -1 ),
                 commentsFeed( commentNS, QLatin1String("commentRss"), QString() ),
                 commentPostUri( commentNS, QLatin1String("comment"), QString() ),
                 commentsLink( akregatorNS, QLatin1String("commentsLink"), QString() ),
                 hash( akregatorNS, QLatin1String("hash"), 0 ),
                 guidIsHash( akregatorNS, QLatin1String("idIsHash"), false ),
                 sourceFeedId( akregatorNS, QLatin1String("sourceFeedId"), -1 ),
                 name( atomNS, QLatin1String("name"), QString() ),
                 uri( atomNS, QLatin1String("uri"), QString() ),
                 email( atomNS, QLatin1String("email"), QString() ),
                 author( atomNS, QLatin1String("author"), QString() ),
                 category( atomNS, QLatin1String("category"), QString() ),
                 entry( atomNS, QLatin1String("entry"), QString() ),
                 readStatus( akregatorNS, QLatin1String("readStatus") ),
                 deleted( akregatorNS, QLatin1String("deleted") ),
                 important( akregatorNS, QLatin1String("important") )
{}
    const QString atomNS;
    const QString akregatorNS;
    const QString commentNS;
    const Element title;
    const Element summary;
    const Element content;
    const Element link;
    const Element language;
    const Element guid;
    const Element published;
    const Element updated;
    const Element commentsCount;
    const Element commentsFeed;
    const Element commentPostUri;
    const Element commentsLink;
    const Element hash;
    const Element guidIsHash;
    const Element sourceFeedId;
    const Element name;
    const Element uri;
    const Element email;
    const Element author;
    const Element category;
    const Element entry;
    const Element readStatus;
    const Element deleted;
    const Element important;
    static const Elements instance;
};

const Elements Elements::instance;

static void readLink( KRss::RssItem& item, QXmlStreamReader& reader )
{
    const QXmlStreamAttributes attrs = reader.attributes();
    const QString rel = attrs.value( QString(), QLatin1String("rel") ).toString();
    if (  rel == QLatin1String("alternate") )
    {
        item.setLink( attrs.value( QString(), QLatin1String("href") ).toString() );
    }
    else if ( rel == QLatin1String("enclosure") )
    {
        KRss::Enclosure enc;
        enc.setUrl( attrs.value( QString(), QLatin1String("href") ).toString() );
        enc.setType( attrs.value( QString(), QLatin1String("type") ).toString() );
        enc.setTitle( attrs.value( QString(), QLatin1String("title") ).toString() );
        bool ok;
        const uint length = attrs.value( QString(), QLatin1String("length") ).toString().toUInt( &ok );
        if ( ok )
            enc.setLength( length );
        const uint duration = attrs.value( Syndication::itunesNamespace(), QLatin1String("duration") ).toString().toUInt( &ok );
        if ( ok )
            enc.setDuration( duration );
        QList<KRss::Enclosure> encs = item.enclosures();
        encs.append( enc );
        item.setEnclosures( encs );
    }
}

static void readAuthor( KRss::RssItem& item, QXmlStreamReader& reader )
{
    KRss::Person author;
    int depth = 1;
    while ( !reader.atEnd() && depth > 0 )
    {
        reader.readNext();
        if ( reader.isEndElement() )
            --depth;
        else if ( reader.isStartElement() )
        {
            if ( Elements::instance.name.isNextIn( reader ) )
                author.setName( reader.readElementText() );
            else if ( Elements::instance.uri.isNextIn( reader ) )
                author.setUri( reader.readElementText() );
            else if ( Elements::instance.email.isNextIn( reader ) )
                author.setEmail( reader.readElementText() );
        }

    }
    QList<KRss::Person> authors = item.authors();
    authors.append( author );
    item.setAuthors( authors );
}

static void readCategory( KRss::RssItem& item, QXmlStreamReader& reader )
{
    const QXmlStreamAttributes attrs = reader.attributes();
    KRss::Category cat;
    cat.setTerm( attrs.value( QString(), QLatin1String("term") ).toString() );
    cat.setScheme( attrs.value( QString(), QLatin1String("scheme") ).toString() );
    cat.setLabel( attrs.value( QString(), QLatin1String("label") ).toString() );
    QList<KRss::Category> cats = item.categories();
    cats.append( cat );
    item.setCategories( cats );
}

static bool readItem( Akonadi::Item& akonadiItem, QXmlStreamReader& reader ) {
    const Elements& el = Elements::instance;
    if ( !reader.atEnd() )
        reader.readNext();

    Akonadi::Item::Flags flags;
    KRss::RssItem item;
    item.setHeadersLoaded( true );
    item.setContentLoaded( true );

    while ( !reader.atEnd() && !el.entry.isNextIn( reader ) )
    {
        if ( reader.isStartElement() )
        {
            if ( el.title.isNextIn( reader ) )
                item.setTitle( reader.readElementText() );
            else if ( el.summary.isNextIn( reader ) )
                item.setDescription( reader.readElementText() );
            else if ( el.content.isNextIn( reader ) )
                item.setContent( reader.readElementText() );
            else if ( el.language.isNextIn( reader ) )
                item.setLanguage( reader.readElementText() );
            else if ( el.guid.isNextIn( reader ) )
                item.setGuid( reader.readElementText() );
            else if ( el.hash.isNextIn( reader ) )
                item.setHash( reader.readElementText().toInt() );
            else if ( el.guidIsHash.isNextIn( reader ) )
                item.setGuidIsHash( QVariant( reader.readElementText() ).toBool() );
            else if ( el.sourceFeedId.isNextIn( reader ) )
                item.setSourceFeedId( reader.readElementText().toInt() );
            else if ( el.commentsLink.isNextIn( reader ) )
                item.setCommentsLink( reader.readElementText() );
            else if ( el.commentPostUri.isNextIn( reader ) )
                item.setCommentPostUri( reader.readElementText() );
            else if ( el.commentsCount.isNextIn( reader ) )
                item.setCommentsCount( reader.readElementText().toInt() );
            else if ( el.commentsFeed.isNextIn( reader ) )
                item.setCommentsFeed( reader.readElementText() );
            else if ( el.link.isNextIn( reader ) )
                readLink( item, reader );
            else if ( el.author.isNextIn( reader ) )
                readAuthor( item, reader );
            else if ( el.category.isNextIn( reader ) )
                readCategory( item, reader );
            else if ( el.published.isNextIn( reader ) )
                item.setDatePublished( KDateTime::fromString( reader.readElementText(), KDateTime::ISODate ) );
            else if ( el.updated.isNextIn( reader ) )
                item.setDateUpdated( KDateTime::fromString( reader.readElementText(), KDateTime::ISODate ) );
            else if ( el.readStatus.isNextIn( reader ) ) {
                const QString statusStr = reader.readElementText();
                if ( statusStr != QLatin1String("new") && statusStr != QLatin1String("unread") )
                    flags.insert( KRss::RssItem::flagRead() );
            } else if ( el.important.isNextIn( reader ) ) {
                if ( reader.readElementText() == QLatin1String("true") )
                    flags.insert( KRss::RssItem::flagImportant() );
            } else if ( el.deleted.isNextIn( reader ) ) {
                if ( reader.readElementText() == QLatin1String("true") ) {
                    flags.insert( KRss::RssItem::flagDeleted() );
                    flags.insert( KRss::RssItem::flagRead() );
                }
            }

        }

        reader.readNext();
    }

    akonadiItem.setPayload<KRss::RssItem>( item );
    akonadiItem.setMimeType( KRss::Item::mimeType() );
    akonadiItem.setRemoteId( item.guid() );
    akonadiItem.setFlags( flags );
    return !reader.hasError();
}

} // namespace

ItemImportReader::ItemImportReader( QIODevice* dev ) : m_reader( new QXmlStreamReader( dev ) ) {
    m_reader->setNamespaceProcessing( true );
    m_firstItemFound = findFirstItem();
}

ItemImportReader::~ItemImportReader() {
    delete m_reader;
}

bool ItemImportReader::hasNext() const {
    return m_firstItemFound && !m_reader->atEnd();
}

Akonadi::Item ItemImportReader::nextItem() {
    const Elements el;
    if ( !m_firstItemFound )
        return Akonadi::Item();

    while ( !m_reader->atEnd() )
        if ( m_reader->isStartElement() &&
                el.entry.isNextIn( *m_reader ) ) {
                Akonadi::Item item;
                if ( readItem( item, *m_reader ) )
                    return item;
                else
                    return Akonadi::Item();
            }
    return Akonadi::Item();
}

bool ItemImportReader::findFirstItem() {
    const Elements el;
    bool itemFound = false;
    while ( !itemFound && !m_reader->atEnd() ) {
        m_reader->readNext();
        if ( m_reader->isStartElement() && el.entry.isNextIn( *m_reader ) )
            itemFound = true;
    }
    return itemFound;
}
