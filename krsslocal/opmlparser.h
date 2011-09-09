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

#ifndef KRSSRESOURCE_OPMLPARSER_H
#define KRSSRESOURCE_OPMLPARSER_H

#include <krssresource/krssresource_export.h>

#include <Akonadi/Collection>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QXmlStreamReader>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <boost/weak_ptr.hpp>

class QXmlStreamWriter;
class QXmlStreamAttributes;

namespace KRssResource {

class ParsedFeed;
class ParsedFolder;

class KRSSRESOURCE_EXPORT ParsedNode : public boost::enable_shared_from_this<ParsedNode>
{
public:
    ParsedNode();
    virtual ~ParsedNode();

    boost::weak_ptr<ParsedFolder> parent() const;
    void setParent( const boost::weak_ptr<ParsedFolder>& parent );

    virtual bool isFolder() const = 0;
    virtual QList<boost::shared_ptr<const ParsedFeed> > feeds() const = 0;
    virtual QList<boost::shared_ptr<const ParsedFolder> > folders() const = 0;
    QString title() const;
    void setTitle( const QString& title );

    QString attribute( const QString& key ) const;
    QHash<QString, QString> attributes() const;
    void setAttribute( const QString& key, const QString& value );
    void setAttributes( const QHash<QString, QString>& attributes );

    QStringList parentFolderTitles() const;

private:
    Q_DISABLE_COPY(ParsedNode)
    class Private;
    Private* const d;
};

class KRSSRESOURCE_EXPORT ParsedFeed : public ParsedNode
{
public:
    ParsedFeed();
    ~ParsedFeed();

    /* reimp */ bool isFolder() const;
    /* reimp */ QList<boost::shared_ptr<const ParsedFeed> > feeds() const;
    /* reimp */ QList<boost::shared_ptr<const ParsedFolder> > folders() const;

    QString xmlUrl() const;
    void setXmlUrl( const QString& xmlUrl );
    QString htmlUrl() const;
    void setHtmlUrl( const QString& htmlUrl );
    QString description() const;
    void setDescription( const QString& description );
    QString type() const;
    void setType( const QString& type );

    Akonadi::Collection toAkonadiCollection() const;
    static boost::shared_ptr<ParsedFeed> fromAkonadiCollection( const Akonadi::Collection& collection );

private:
    Q_DISABLE_COPY(ParsedFeed)
    class Private;
    Private* const d;
};

class KRSSRESOURCE_EXPORT ParsedFolder : public ParsedNode
{
public:
    ParsedFolder();
    ~ParsedFolder();

    /* reimp */ bool isFolder() const;
    /* reimp */ QList<boost::shared_ptr<const ParsedFeed> > feeds() const;
    /* reimp */ QList<boost::shared_ptr<const ParsedFolder> > folders() const;

    QList<boost::shared_ptr<const ParsedNode> > children() const;
    void setChildren( const QList<boost::shared_ptr<const ParsedNode> >& children );
    void addChild( const boost::shared_ptr<const ParsedNode>& child );

private:
    class Private;
    Private* const d;
};

class KRSSRESOURCE_EXPORT OpmlReader
{
public:
    OpmlReader();
    ~OpmlReader();

    void readOpml( QXmlStreamReader& reader );

    QList<boost::shared_ptr<const ParsedFeed> > feeds() const;
    QList<boost::shared_ptr<const ParsedNode> > topLevelNodes() const;

    QStringList tags() const;

private:
    class Private;
    Private * const d;
    Q_DISABLE_COPY( OpmlReader )
};

class KRSSRESOURCE_EXPORT OpmlWriter
{
public:
    static void writeOpml( QXmlStreamWriter& writer, const QList<boost::shared_ptr< const ParsedNode> >& nodes,
                           const QString& title = QLatin1String("") );
    static void writeOutlineFeed( QXmlStreamWriter& writer, const boost::shared_ptr<const ParsedFeed>& feed );
    static void writeOutlineNodes( QXmlStreamWriter& writer, const QList<boost::shared_ptr< const ParsedNode> >& nodes );

};

} //namespace KRssResource

#endif // KRSSRESOURCE_OPMLPARSER_H
