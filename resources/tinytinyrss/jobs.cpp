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
#include <KRandom>
#include <KUrl>

#include <QUrl>

#include <Akonadi/EntityDisplayAttribute>

#include <krss/feedcollection.h>

#include <qjson/parser.h>
#include <qjson/serializer.h>

using Akonadi::Collection;

static QString mimeType() {
    return QLatin1String("application/rss+xml");
}

TransferJob::TransferJob( QObject* parent )
    : KJob( parent )
    , m_postDataFormat( Json )
{
}

Akonadi::Item TransferJob::item() const
{
    return m_item;
}

void TransferJob::setItem( const Akonadi::Item& item )
{
    m_item = item;
}

void TransferJob::setCollection( const Akonadi::Collection& c )
{
    m_collection = c;
}

Akonadi::Collection TransferJob::collection() const
{
    return m_collection;
}

void TransferJob::start()
{
    QMetaObject::invokeMethod( this, "doStart", Qt::QueuedConnection );
}

void TransferJob::setUrl( const KUrl& url )
{
    m_url = url;
}

KUrl TransferJob::url() const
{
    return m_url;
}

void TransferJob::setOperation( const QString& operation )
{
    insertData( QLatin1String("op"), operation );
}

void TransferJob::setSessionId( const QString& sessionId )
{
    insertData( QLatin1String("sid"), sessionId );
}

void TransferJob::setPostDataFormat( PostDataFormat format )
{
    m_postDataFormat = format;
}

void TransferJob::doStart()
{
    QByteArray postData;

    if ( m_postDataFormat == Json ) {
        QJson::Serializer serializer;
        postData = serializer.serialize( m_postData );
    } else {
        QUrl enc;
        QMap<QString, QVariant>::ConstIterator it = m_postData.constBegin();
        for ( ; it != m_postData.constEnd(); ++it )
            enc.addQueryItem( it.key(), it.value().toString() );
        postData = enc.encodedQuery();
    }

    KUrl url = m_url;
    url.addPath( QLatin1String("api/") );
    KIO::TransferJob* transfer = KIO::http_post( url, postData, KIO::HideProgressInfo );
    if ( m_postDataFormat == EncodedQuery )
        transfer->addMetaData( QLatin1String("content-type"), QLatin1String("Content-type: application/x-www-form-urlencoded") );

    connect( transfer, SIGNAL(data(KIO::Job*,QByteArray)), this, SLOT(dataReceived(KIO::Job*,QByteArray)) );
    connect( transfer, SIGNAL(result(KJob*)), this, SLOT(postFinished(KJob*)) );
}

void TransferJob::dataReceived( KIO::Job* job, const QByteArray& data )
{
    Q_UNUSED( job )
    m_responseData += data;
}

bool TransferJob::checkForError( const QVariant& jsonObject )
{
    const QVariantMap map = jsonObject.toMap();
    QVariantMap::ConstIterator statusIt = map.find( QLatin1String("status") );
    if ( statusIt == map.constEnd() ) {
        setError( UnexpectedJsonContentError );
        setErrorText( i18n("No status attribute found") );
        return true;
    }

    bool ok;
    const int status = map.value( QLatin1String("status") ).toInt( &ok );
    if ( !ok ) {
        setError( UnexpectedJsonContentError );
        setErrorText( i18n("Could not parse status from %1", map.value( QLatin1String("status") ).toString() ) );
        return true;
    }

    if ( status == 0 )
        return false;

    const QVariantMap content = map.value( QLatin1String("content") ).toMap();
    const QString errorEnum = content.value( QLatin1String("error") ).toString();

    if ( errorEnum == QLatin1String("NOT_LOGGED_IN") ) {
        setError( NotLoggedInError );
        setErrorText( i18n("Not logged in") );
    } else if ( errorEnum == QLatin1String("API_DISABLED") ) {
        setError( ApiDisabledError );
        setErrorText( i18n("The Tiny Tiny RSS installation has its external API disabled. Check the \"Enable external API\" option in Tiny Tiny RSS' preferences to enable it.") );
    } else if ( errorEnum == QLatin1String("LOGIN_ERROR") ) {
        setError( AuthenticationFailedError );
        setErrorText( i18n("Login failed. Ensure username and password are correct.") );
    } else if ( errorEnum == QLatin1String("INCORRECT_USAGE") ) {
        setError( OtherError );
        setErrorText( i18n("Incorrect usage") );
    } else {
        setError( OtherError );
        setErrorText( errorEnum );
    }

    return true;
}

void TransferJob::postFinished( KJob* j )
{
    KIO::TransferJob* job = qobject_cast<KIO::TransferJob*>( j );
    Q_ASSERT( job );

    if ( job->error() != KJob::NoError ) {
        setError( NetworkError );
        setErrorText( job->errorString() );
        emitResult();
        return;
    }

    if ( job->isErrorPage() ) {
        setError( NetworkError );
        setErrorText( i18n("HTTP error") ); //how to add more details?
        emitResult();
        return;
    }

    QJson::Parser parser;
    bool ok;
    const QVariant jsonObject = parser.parse( m_responseData, &ok );

    if ( !ok ) {
        setError( InvalidJsonError );
        setErrorText( i18n( "Invalid Json: [%1] %2", QString::number( parser.errorLine() ), parser.errorString() ) );
        emitResult();
        return;
    }

    if ( checkForError( jsonObject ) ) {
        emitResult();
        return;
    }

    m_responseJson = jsonObject;
    m_responseContent = jsonObject.toMap().value( QLatin1String("content") );

    emitResult();
}

void TransferJob::insertData( const QString& key, const QVariant& value )
{
    m_postData.insert( key, value );
}

QVariant TransferJob::responseJson() const
{
    return m_responseJson;
}

QVariant TransferJob::responseContent() const
{
    return m_responseContent;
}


RetrieveCollectionsJob::RetrieveCollectionsJob( QObject* parent )
    : KJob( parent )
    , m_nextCategoryToFetch( 0 )
{
}

void RetrieveCollectionsJob::setUrl( const KUrl& url )
{
    m_url = url;
}

void RetrieveCollectionsJob::setSessionId( const QString& sessionId )
{
    m_sessionId = sessionId;
}

void RetrieveCollectionsJob::start()
{
    TransferJob* job = new TransferJob( this );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(categoriesListed(KJob*)) );
    job->setUrl( m_url );
    job->setSessionId( m_sessionId );
    job->setOperation( QLatin1String("getCategories") );
    job->start();
}

void RetrieveCollectionsJob::fetchNextCategory()
{
    if ( m_nextCategoryToFetch >= m_categories.size() ) {
        emitResult();
        return;
    }

    const QString catId = m_categories[m_nextCategoryToFetch].id;
    TransferJob* job = new TransferJob( this );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(feedsListed(KJob*)) );
    job->setUrl( m_url );
    job->setSessionId( m_sessionId );
    job->setOperation( QLatin1String("getFeeds") );
    job->insertData( QLatin1String("cat_id"), catId );
    job->start();
}

void RetrieveCollectionsJob::feedsListed( KJob* j )
{
    TransferJob* job = qobject_cast<TransferJob*>( j );
    Q_ASSERT( job );
    if ( job->error() != KJob::NoError ) {
        setError( job->error() );
        setErrorText( job->errorString() );
        emitResult();
        return;
    }

    const QVariantList list = job->responseContent().toList();
    QVector<Feed> feeds;
    feeds.reserve( list.size() );

    Q_FOREACH( const QVariant& i, list ) {
        const QVariantMap map = i.toMap();
        Feed feed;
        feed.feedUrl = map.value( QLatin1String("feed_url") ).toString();
        feed.title = map.value( QLatin1String("title") ).toString();
        feed.id = map.value( QLatin1String("id") ).toString();
        feeds.append( feed );
    }

    m_categories[m_nextCategoryToFetch].feeds = feeds;

    m_nextCategoryToFetch++;
    fetchNextCategory();
}

void RetrieveCollectionsJob::categoriesListed( KJob* j )
{
    TransferJob* job = qobject_cast<TransferJob*>( j );
    Q_ASSERT( job );
    if ( job->error() != KJob::NoError ) {
        setError( job->error() );
        setErrorText( job->errorString() );
        emitResult();
        return;
    }

    const QVariantList content = job->responseContent().toList();
    QVariantList::ConstIterator it = content.constBegin();
    for ( ; it != content.constEnd(); ++it ) {
        const QVariantMap catMap = it->toMap();
        const QString id = catMap.value( QLatin1String("id") ).toString();
        const QString title = catMap.value( QLatin1String("title") ).toString();
        const QString order_id = catMap.value( QLatin1String("order_id") ).toString();
        if ( order_id.isEmpty() ) //Ignoring "special" categories not containing feeds
            continue;
        Category cat;
        cat.id = id;
        cat.title = title;
        m_categories.append( cat );
    }

    fetchNextCategory();
}

static Akonadi::Collection::List buildCollectionTree( const QVector<RetrieveCollectionsJob::Category>& categories )
{
    Akonadi::Collection::List collections;

    KRss::FeedCollection top;
    top.setParentCollection( Akonadi::Collection::root() );
    top.setContentMimeTypes( QStringList() << Collection::mimeType() << mimeType() );
    top.setName( i18n("Tiny Tiny RSS") );
    top.setRemoteId( QLatin1String("internal-root") );
    top.setRights( Collection::CanCreateCollection );
    top.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( i18n("Tiny Tiny RSS") );
    top.setAllowSubfolders( true );
    top.setIsFolder( true );

    collections.append( top );

    Q_FOREACH( const RetrieveCollectionsJob::Category& cat, categories ) {
        KRss::FeedCollection folder;
        folder.setRemoteId( cat.id );
        folder.setIsFolder( true );
        folder.setParentCollection( top );
        folder.setContentMimeTypes( QStringList() << Collection::mimeType() << mimeType() );
        folder.setName( cat.title + KRandom::randomString( 8 ) );
        folder.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( cat.title );
        folder.setAllowSubfolders( false );
        collections.append( folder );

        Q_FOREACH ( const RetrieveCollectionsJob::Feed& feed, cat.feeds ) {
            KRss::FeedCollection fc;
            fc.setParentCollection( folder );
            fc.setRemoteId( feed.id );
            fc.setContentMimeTypes( QStringList() << mimeType() );
            fc.setName( feed.title + KRandom::randomString( 8 ) );
            fc.setXmlUrl( feed.feedUrl );
//            feed.setImageUrl( node.icon );
            fc.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( feed.title );
            fc.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setIconName( QString("application-rss+xml") );
            collections.append( fc );
        }
    }

    return collections;
}

Akonadi::Collection::List RetrieveCollectionsJob::retrievedCollections() const
{
    return buildCollectionTree( m_categories );
}
