/*
    Copyright (c) 2012 Frank Osterfeld <osterfeld@kde.org>

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

#include "jobs.h"

#include <KIO/Scheduler>

#include <KDebug>
#include <KLocalizedString>

ParseException::ParseException( const QString& message )
    : std::runtime_error( message.toStdString() )
    , m_message( message )
{}

ParseException::~ParseException() throw() {
}

QString ParseException::message() const {
    return m_message;
}

Job::Job( QObject* parent )
    : KJob( parent )
{
}

void Job::start()
{
    QMetaObject::invokeMethod( this, "doStart", Qt::QueuedConnection );
}

void Job::setUrl( const KUrl& url )
{
    m_url = url;
}

KUrl Job::url() const
{
    return m_url;
}

void Job::setUsername( const QString& username )
{
    m_username = username;
}

QString Job::username() const
{
    return m_username;
}

void Job::setPassword( const QString& password )
{
    m_password = password;
}

QString Job::password() const
{
    return m_password;
}

static const QString prefix = QLatin1String("ocs/v1.php/news/");

KUrl Job::assembleUrl( const QString& path ) const
{
    KUrl u = m_url;
    QString basepath = m_url.path();
    if ( !basepath.endsWith( QLatin1Char('/') ) )
        basepath += QLatin1Char('/');
    u.setPath( basepath + prefix + path );
    u.setQuery( QLatin1String("?format=xml") );
    return u;
}

void Job::doStart()
{
    const KUrl url = assembleUrl( m_path );
    KIO::TransferJob* transfer = KIO::get( url, KIO::Reload, KIO::HideProgressInfo );
    transfer->addMetaData ( QLatin1String("customHTTPHeader"), "Authorization: Basic " + QString( m_username + QLatin1Char(':') + m_password ).toUtf8().toBase64() );
    connect( transfer, SIGNAL(data(KIO::Job*,QByteArray)),
             this, SLOT(data(KIO::Job*,QByteArray)) );
    connect( transfer, SIGNAL(result(KJob*)), this, SLOT(jobFinished(KJob*)) );
}

void Job::setPath( const QString& path )
{
    m_path = path;
}

void Job::data( KIO::Job *job, const QByteArray &data )
{
    m_buffer.append( data );
}

void Job::jobFinished( KJob *j )
{
    KIO::TransferJob* job = qobject_cast<KIO::TransferJob*>( j );

    if ( job->error() != KJob::NoError ) {
        setError( IOError );
        setErrorText( job->errorText() );
        emitResult();
        return;
    }

    if ( job->isErrorPage() ) {
        setError( IOError );
        setErrorText( i18n("Server returned error") ); //TODO how to get details?
        emitResult();
        return;
    }

    QXmlStreamReader reader( m_buffer );
    reader.setNamespaceProcessing( true );

    try {
        parse( &reader );
    } catch ( const ParseException& e ) {
        setError( ParseError );
        setErrorText( e.message() );
        emitResult();
        return;
    }

    if ( reader.hasError() ) {
        const QString errorString = i18n("Error parsing XML [%1:%2]: %3", QString::number( reader.lineNumber() ), QString::number( reader.columnNumber() ), reader.errorString() );
        setError( XmlError );
        setErrorText( errorString );
        emitResult();
        return;
    }

    emitResult();
}

ListNodeJob::ListNodeJob( Type type, QObject* parent )
    : Job( parent )
    , m_type( type )
{
    setPath( type == Feeds ? QLatin1String("feeds") : QLatin1String("folders") );
}

static ListNodeJob::Node readElement( QXmlStreamReader* reader ) {
    ListNodeJob::Node node;
    while ( !reader->atEnd() && !reader->hasError() ) {
        reader->readNext();
        if ( reader->isEndElement() ) {
            return node;
        }

        if ( reader->isStartElement() ) {
            if ( reader->name() == QLatin1String("id") ) {
                node.id = reader->readElementText();
            } else if ( reader->name() == QLatin1String("title") ) {
                node.title = reader->readElementText();
            } else if ( reader->name() == QLatin1String("name") ) {
                node.title = reader->readElementText();
            } else if ( reader->name() == QLatin1String("folderId") ) {
                node.parentId = reader->readElementText();
            } else if ( reader->name() == QLatin1String("parentId") ) {
                node.parentId = reader->readElementText();
            } else if ( reader->name() == QLatin1String("icon") ) {
                node.icon = reader->readElementText();
            } else if ( reader->name() == QLatin1String("url") ) {
                node.link = reader->readElementText();
            } else if ( reader->name() == QLatin1String("opened") ) {
                //Ignore?
            } else {
                reader->raiseError( i18n("Unexpected element %1 in <element>", reader->name().toString() ) );
            }
        }
    }
    return ListNodeJob::Node();
}

static ListNodeJob::Meta readMeta( QXmlStreamReader* reader ) {
    ListNodeJob::Meta meta;
    meta.statuscode = 0;
    while ( !reader->atEnd() && !reader->hasError() ) {
        reader->readNext();
        if ( reader->isEndElement() ) {
            return meta;
        }

        if ( reader->isStartElement() ) {
            if ( reader->name() == QLatin1String("status") ) {
                meta.status = reader->readElementText();
            } else if ( reader->name() == QLatin1String("statuscode") ) {
                bool ok = false;
                meta.statuscode = reader->readElementText().toInt( &ok );
                if ( !ok )
                    reader->raiseError( i18n("Could not parse status code") );
            } else if ( reader->name() == QLatin1String("message") ) {
                meta.message = reader->readElementText();
            } else {
                reader->raiseError( i18n("Unexpected element %1 in <meta>", reader->name().toString() ) );
            }
        }
    }
    return ListNodeJob::Meta();

}

void ListNodeJob::parse( QXmlStreamReader* reader )
{
    while ( !reader->atEnd() && !reader->hasError() ) {
        reader->readNext();
        if ( reader->isStartElement() ) {
            if ( reader->name() == QLatin1String("element") ) {
                const Node n = readElement( reader );
                m_children += n;
            } else if ( reader->name() == QLatin1String("meta") ) {
                const Meta meta = readMeta( reader );
                if ( meta.statuscode != 100 )
                    throw ParseException( meta.message );
            }
        }
    }
}

QVector<ListNodeJob::Node> ListNodeJob::children() const
{
    return m_children;
}


#include "jobs.moc"
