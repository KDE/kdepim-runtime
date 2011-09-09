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

#include "opmlparser.h"

#include <krss/feedcollection.h>

#include <KLocale>
#include <KDebug>
#include <QtXml/QXmlStreamWriter>
#include <QtXml/QXmlStreamReader>
#include <QtXml/QXmlAttributes>
#include <memory>

using namespace boost;
using namespace KRssResource;

class ParsedNode::Private {
public:
    QString title;
    QHash<QString, QString> attributes;
    weak_ptr<ParsedFolder> parent;
};

ParsedNode::ParsedNode() : d( new Private ) {
}

weak_ptr<ParsedFolder> ParsedNode::parent() const {
    return d->parent;
}

void ParsedNode::setParent( const weak_ptr<ParsedFolder>& parent ) {
    d->parent = parent;
}

ParsedNode::~ParsedNode() {
    delete d;
}

QString ParsedNode::title() const {
    return d->title;
}

void ParsedNode::setTitle( const QString& title ) {
    d->title = title;
}

QString ParsedNode::attribute( const QString& key ) const {
    return d->attributes.value( key );
}

QHash<QString, QString> ParsedNode::attributes() const
{
    return d->attributes;
}

void ParsedNode::setAttribute( const QString& key, const QString& value )
{
    d->attributes.insert( key, value );
}

void ParsedNode::setAttributes( const QHash<QString, QString>& attributes )
{
    d->attributes = attributes;
}

QStringList ParsedNode::parentFolderTitles() const {
    QStringList labels;
    shared_ptr<ParsedNode> it = parent().lock();
    while ( it ) {
        labels.push_back( it->title() );
        it = it->parent().lock();
    }
    return labels;
}

class ParsedFeed::Private
{
public:
    Private() {}

    QString xmlUrl;
    QString htmlUrl;
    QString description;
    QString type;
};

ParsedFeed::ParsedFeed() : d( new Private )
{
}

ParsedFeed::~ParsedFeed()
{
    delete d;
}

bool ParsedFeed::isFolder() const
{
    return false;
}

QList<shared_ptr<const ParsedFeed> > ParsedFeed::feeds() const
{
  return QList<shared_ptr<const ParsedFeed> >() << static_pointer_cast<const ParsedFeed>( shared_from_this() );
}

QList<shared_ptr<const ParsedFolder> > ParsedFeed::folders() const
{
  return QList<shared_ptr<const ParsedFolder> >();
}


QString ParsedFeed::xmlUrl() const
{
    return d->xmlUrl;
}

void ParsedFeed::setXmlUrl( const QString& xmlUrl )
{
    d->xmlUrl = xmlUrl;
}

QString ParsedFeed::htmlUrl() const
{
    return d->htmlUrl;
}

void ParsedFeed::setHtmlUrl( const QString& htmlUrl )
{
    d->htmlUrl = htmlUrl;
}

QString ParsedFeed::description() const
{
    return d->description;
}

void ParsedFeed::setDescription( const QString& description )
{
    d->description = description;
}

QString ParsedFeed::type() const
{
    return d->type;
}

void ParsedFeed::setType( const QString& type )
{
    d->type = type;
}


Akonadi::Collection ParsedFeed::toAkonadiCollection() const
{
    KRss::FeedCollection feed;
    feed.setRemoteId( d->xmlUrl );
    feed.setTitle( title() );
    feed.setXmlUrl( d->xmlUrl );
    feed.setHtmlUrl( d->htmlUrl );
    feed.setDescription( d->description );
    feed.setFeedType( d->type );
    feed.setName( title() );
    feed.setContentMimeTypes( QStringList( QLatin1String("application/rss+xml") ) );
    return feed;
}

shared_ptr<ParsedFeed> ParsedFeed::fromAkonadiCollection( const Akonadi::Collection& collection )
{
    const KRss::FeedCollection feedCollection = collection;
    shared_ptr<ParsedFeed> parsedFeed( new ParsedFeed );
    parsedFeed->setTitle( collection.name() );
    parsedFeed->d->xmlUrl = feedCollection.xmlUrl();
    parsedFeed->d->htmlUrl = feedCollection.htmlUrl();
    parsedFeed->d->description = feedCollection.description();
    parsedFeed->d->type = feedCollection.feedType();
    parsedFeed->setAttribute( QLatin1String("remoteid"), feedCollection.remoteId() );
    return parsedFeed;
}

class ParsedFolder::Private {
public:
    QList<shared_ptr<const ParsedNode> > children;
};

ParsedFolder::ParsedFolder() : d( new Private ) {

}

ParsedFolder::~ParsedFolder() {
    delete d;
}

bool ParsedFolder::isFolder() const {
    return true;
}

QList<shared_ptr<const ParsedFeed> > ParsedFolder::feeds() const {
  QList<shared_ptr<const ParsedFeed> > f;
  Q_FOREACH( const shared_ptr<const ParsedNode>& i, d->children )
      f << i->feeds();
  return f;
}

QList<shared_ptr<const ParsedFolder> > ParsedFolder::folders() const {
  QList<shared_ptr<const ParsedFolder> > f;
  f << static_pointer_cast<const ParsedFolder>( shared_from_this() );
  Q_FOREACH( const shared_ptr<const ParsedNode>& i, d->children )
      f << i->folders();
  return f;
}

QList<shared_ptr<const ParsedNode> > ParsedFolder::children() const {
    return d->children;
}

void ParsedFolder::setChildren( const QList<shared_ptr<const ParsedNode> >& children ) {
    d->children = children;
}

void ParsedFolder::addChild( const shared_ptr<const ParsedNode>& child ) {
    d->children.push_back( child );
}

class OpmlReader::Private
{
public:
    explicit Private() {}

    QStringRef attributeValue( const QXmlStreamAttributes& attributes, const QString& name );
    void readBody( QXmlStreamReader& reader );
    void readOutline( QXmlStreamReader& reader, const shared_ptr<ParsedFolder> &parent );
    void readUnknownElement( QXmlStreamReader& reader );

    QList<shared_ptr<ParsedNode> > m_topNodes;
};

QStringRef OpmlReader::Private::attributeValue( const QXmlStreamAttributes& attributes, const QString& name )
{
    Q_FOREACH( const QXmlStreamAttribute& attr, attributes ) {
        if ( attr.name().toString().toLower() == name ) {
            return attr.value();
        }
    }

    return QStringRef();
}

void OpmlReader::Private::readBody( QXmlStreamReader& reader )
{
    Q_ASSERT( reader.isStartElement() && reader.name().toString().toLower() == QLatin1String("body") );

    while ( !reader.atEnd() ) {
        reader.readNext();

        if ( reader.isEndElement() )
            break;

        if ( reader.isStartElement() ) {
            if ( reader.name().toString().toLower() == QLatin1String("outline") ) {
                readOutline( reader, shared_ptr<ParsedFolder>() );
            }
            else {
                readUnknownElement( reader );
            }
        }
    }
}

void OpmlReader::Private::readOutline( QXmlStreamReader& reader, const shared_ptr<ParsedFolder>& parent )
{
    Q_ASSERT( reader.isStartElement() && reader.name().toString().toLower() == QLatin1String("outline") );

    shared_ptr<ParsedNode> newNode;
    const QString xmlUrl = attributeValue( reader.attributes(), QLatin1String("xmlurl") ).toString();

    if ( xmlUrl.isEmpty() ) {
        const QStringRef textAttribute = attributeValue( reader.attributes(), QLatin1String("text") );
        if ( !textAttribute.isEmpty() ) {
            // this attribute seem to represent a folder
            shared_ptr<ParsedFolder> newFolder( new ParsedFolder );
            newNode = newFolder;
            const QXmlStreamAttributes attrs = reader.attributes();

            Q_FOREACH( const QXmlStreamAttribute& attr, attrs ) {
                if ( attr.name().toString().toLower() == QLatin1String("title") )
                    newFolder->setTitle( attr.value().toString() );
                else if ( newFolder->title().isEmpty() && attr.name().toString().toLower() == QLatin1String("text")  )
                    newFolder->setTitle( attr.value().toString() );
                else {
                    newFolder->setAttribute( attr.name().toString(), attr.value().toString() );
                }
            }
        }
        else {
            kDebug() << "Encountered an empty outline";
            const QXmlStreamAttributes attrs;
            Q_FOREACH( const QXmlStreamAttribute& attr, attrs ) {
                kDebug() << "Attribute name:" << attr.name() << ", value:" << attr.value();
            }
        }
    }
    else {
        // this is a feed
        kDebug() << "Feed:" << xmlUrl;
        shared_ptr<ParsedFeed> newFeed( new ParsedFeed );
        newNode = newFeed;
        const QXmlStreamAttributes attrs = reader.attributes();
        Q_FOREACH( const QXmlStreamAttribute& attr, attrs ) {
            if ( attr.name().toString().toLower() == QLatin1String("title") )
                newFeed->setTitle( attr.value().toString() );
            else if ( newFeed->title().isEmpty() && attr.name().toString().toLower() == QLatin1String("text")  )
                newFeed->setTitle( attr.value().toString() );
            else if ( attr.name().toString().toLower() == QLatin1String("htmlurl") )
                newFeed->setHtmlUrl( attr.value().toString() );
            else if ( attr.name().toString().toLower() == QLatin1String("xmlurl") )
                newFeed->setXmlUrl( attr.value().toString() );
            else if ( attr.name().toString().toLower() == QLatin1String("description") )
                newFeed->setDescription( attr.value().toString() );
            else if ( attr.name().toString().toLower() == QLatin1String("type") )
                newFeed->setType( attr.value().toString() );
            else if ( attr.name().toString().toLower() == QLatin1String("category") ) {
                const QStringList categories = attr.value().toString().split( QRegExp( QLatin1String("[,/]") ), QString::SkipEmptyParts );
#if 0
                Q_FOREACH( const QString& category, categories ) {
                    if ( !currentTags.contains( category ) )
                        currentTags.append( category );
                }
#endif
            }
            else {
                newFeed->setAttribute( attr.name().toString(), attr.value().toString() );
            }
        }

        if ( newFeed->title().isEmpty() )
            newFeed->setTitle( xmlUrl );

        if ( newFeed->type().isEmpty() )
            newFeed->setType( QLatin1String("rss") );
    }

    if ( parent ) {
        parent->addChild( newNode );
        newNode->setParent( parent );
    }
    else
        m_topNodes.append( newNode );

    while ( !reader.atEnd() ) {
        reader.readNext();

        if ( reader.isEndElement() )
            break;

        if ( reader.isStartElement() ) {
            if ( reader.name().toString().toLower() == QLatin1String("outline") && newNode->isFolder() )
                readOutline( reader, static_pointer_cast<ParsedFolder>( newNode ) );
            else
                readUnknownElement( reader );
        }
    }
}

void OpmlReader::Private::readUnknownElement( QXmlStreamReader& reader )
{
    Q_ASSERT( reader.isStartElement() );

    while ( !reader.atEnd() ) {
        reader.readNext();

        if ( reader.isEndElement() )
            break;

        if ( reader.isStartElement() )
            readUnknownElement( reader );
    }
}

OpmlReader::OpmlReader()
    : d( new Private )
{
}

OpmlReader::~OpmlReader()
{
    delete d;
}

QList<shared_ptr<const ParsedFeed> > OpmlReader::feeds() const
{
    QList<shared_ptr<const ParsedFeed> > f;
    Q_FOREACH ( const shared_ptr<const ParsedNode>& i, d->m_topNodes )
        f << i->feeds();
    return f;
}

QList<shared_ptr<const ParsedNode> > OpmlReader::topLevelNodes() const
{
    QList<shared_ptr<const ParsedNode> > l;
    Q_FOREACH( const shared_ptr<ParsedNode>& i, d->m_topNodes )
        l << i;
    return l;
}

QStringList OpmlReader::tags() const
{
    QStringList titles;
    QList<shared_ptr<const ParsedFolder> > f;
    Q_FOREACH ( const shared_ptr<const ParsedNode>& i, d->m_topNodes )
        f << i->folders();
    Q_FOREACH ( const shared_ptr<const ParsedFolder>& i, f )
            titles << i->title();
    return titles;
}

void OpmlReader::readOpml( QXmlStreamReader& reader )
{
    Q_ASSERT( reader.isStartElement() && reader.name().toString().toLower() == QLatin1String("opml") );

    while ( !reader.atEnd() ) {
        reader.readNext();

        if ( reader.isEndElement() )
            break;

        if ( reader.isStartElement() ) {
            if ( reader.name().toString().toLower() == QLatin1String("head") ) {
                d->readUnknownElement( reader );
            }
            else if ( reader.name().toString().toLower() == QLatin1String("body") ) {
                d->readBody( reader );
            }
            else {
                d->readUnknownElement( reader );
            }
        }
    }
}

void OpmlWriter::writeOpml( QXmlStreamWriter& writer, const QList<shared_ptr<const ParsedNode> >& nodes, const QString& title)
{
    writer.writeStartElement( QLatin1String("opml") );
    writer.writeAttribute( QLatin1String("version"), QLatin1String("2.0") );
    writer.writeStartElement( QLatin1String("head") );
    writer.writeTextElement( QLatin1String("title"), title );
    writer.writeEndElement(); // head
    writer.writeStartElement( QLatin1String("body") );
    writeOutlineNodes( writer, nodes );
    writer.writeEndElement(); // body
    writer.writeEndElement(); // opml
}

void OpmlWriter::writeOutlineNodes( QXmlStreamWriter& writer, const QList<shared_ptr<const ParsedNode> >& nodes )
{
    Q_FOREACH( const shared_ptr<const ParsedNode>& node, nodes ) {
	if (node->isFolder()) {
	    const shared_ptr<const ParsedFolder> folder = (static_pointer_cast<const ParsedFolder>( node ));
	    writer.writeStartElement( QLatin1String("outline") );
	    writer.writeAttribute( QLatin1String("text"), folder->title() );
	    writer.writeAttribute( QLatin1String("title"), folder->title() );
	    writeOutlineNodes( writer, folder->children());
	    writer.writeEndElement();
	}
	else {
	    writeOutlineFeed( writer, static_pointer_cast<const ParsedFeed>( node ) );
	}
    }
}

void OpmlWriter::writeOutlineFeed( QXmlStreamWriter& writer, const shared_ptr<const ParsedFeed>& feed )
{
    writer.writeStartElement( QLatin1String("outline") );
    writer.writeAttribute( QLatin1String("text"), feed->title() );
    writer.writeAttribute( QLatin1String("title"), feed->title() );
//    writer.writeAttribute( QLatin1String("description"), feed->description() );
    writer.writeAttribute( QLatin1String("htmlUrl"), feed->htmlUrl() );
    writer.writeAttribute( QLatin1String("xmlUrl"), feed->xmlUrl() );
    writer.writeAttribute( QLatin1String("type"), feed->type());
/*    
    QHashIterator<QString, QString> it( feed->attributes() );
    while ( it.hasNext() ) {
        it.next();
        writer.writeAttribute( it.key(), it.value() );
    }
*/
    writer.writeEndElement();   // outline
}
