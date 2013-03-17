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

#ifndef TINYTINYRSS_JOBS_H
#define TINYTINYRSS_JOBS_H

#include <KJob>
#include <KUrl>

#include <KIO/JobClasses>

#include <Akonadi/Collection>
#include <Akonadi/Item>

#include <QVariantMap>
#include <stdexcept>


class TransferJob : public KJob {
    Q_OBJECT
public:
    explicit TransferJob( QObject* parent=0 );

    enum Error {
        NetworkError=KJob::UserDefinedError,
        NotLoggedInError,
        AuthenticationFailedError,
        ApiDisabledError,
        InvalidJsonError,
        UnexpectedJsonContentError,
        OtherError
    };

    enum PostDataFormat {
        Json,
        EncodedQuery
    };

    void setPostDataFormat( PostDataFormat format );

    KUrl url() const;
    void setUrl( const KUrl& );

    Akonadi::Collection collection() const;
    void setCollection( const Akonadi::Collection& collection );
    Akonadi::Item item() const;
    void setItem( const Akonadi::Item& item );

    void setOperation( const QString& operation );
    void setSessionId( const QString& sessionId );

    void insertData( const QString& key, const QVariant& value );

    void start();

    QVariant responseContent() const;
    QVariant responseJson() const;

private:
    bool checkForError( const QVariant& jsonObject );
    Q_INVOKABLE void doStart();

private Q_SLOTS:
    void postFinished( KJob* job );
    void dataReceived( KIO::Job* job, const QByteArray& data );

private:
    PostDataFormat m_postDataFormat;
    KUrl m_url;
    Akonadi::Collection m_collection;
    Akonadi::Item m_item;
    QVariantMap m_postData;
    QVariant m_responseJson;
    QVariant m_responseContent;
    QByteArray m_responseData;
};

class RetrieveCollectionsJob : public KJob {
    Q_OBJECT
public:

    struct Feed {
        QString id;
        QString title;
        QString feedUrl;
    };

    struct Category {
        QString id;
        QString title;
        QVector<Feed> feeds;
    };

    explicit RetrieveCollectionsJob( QObject* parent=0 );

    void setUrl( const KUrl& url );
    void setSessionId( const QString& sessionId );

    void start();

    Akonadi::Collection::List retrievedCollections() const;

private Q_SLOTS:
    void categoriesListed( KJob* );
    void feedsListed( KJob* );

private:
    void fetchNextCategory();

private:
    KUrl m_url;
    QString m_sessionId;
    QVector<Category> m_categories;
    int m_nextCategoryToFetch;
};

#endif
