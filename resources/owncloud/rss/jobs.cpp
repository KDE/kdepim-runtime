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

static const QString prefix = QLatin1String("ocs/v1.php/");

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
    m_reader.addData( data );
    try {
        parseChunk( &m_reader );
    } catch ( const ParseException& e ) {
        job->disconnect( this );
        setError( ParseError );
        setErrorText( e.message() );
        emitResult();
        return;
    }

    if ( m_reader.hasError() && m_reader.error() != QXmlStreamReader::PrematureEndOfDocumentError ) {
        const QString errorString = i18n("Error parsing XML [%1:%2]: %3", QString::number( m_reader.lineNumber() ), QString::number( m_reader.columnNumber() ), m_reader.errorString() );
        job->disconnect( this );
        setError( XmlError );
        setErrorText( errorString );
        emitResult();
        return;
    }
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

    emitResult();
}

ListNodeJob::ListNodeJob( QObject* parent )
    : Job( parent )
{
    setPath( QLatin1String("cloud/users") );
}

void ListNodeJob::parseChunk( QXmlStreamReader* reader )
{
    while ( !reader->atEnd() && !reader->hasError() ) {
        reader->readNext();
        if ( reader->isStartElement() ) {
            if ( reader->name() == QLatin1String("element") ) {
                const QString name = reader->readElementText( QXmlStreamReader::ErrorOnUnexpectedElement );
                Node n;
                n.id = m_nodePath + QLatin1Char('/') + name;
                n.title = name;
                m_children += n;
            }
        }
    }
}

QVector<ListNodeJob::Node> ListNodeJob::children() const
{
    return m_children;
}

void ListNodeJob::setNodePath( const QString& nodePath )
{
    m_nodePath = nodePath;
}

#include "jobs.moc"
