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

#ifndef OWNCLOUD_JOBS_H
#define OWNCLOUD_JOBS_H

#include <KJob>
#include <KUrl>

#include <KIO/JobClasses>

#include <QXmlStreamReader>

#include <Akonadi/Collection>

#include <stdexcept>

class ParseException : public std::runtime_error {
public:
    explicit ParseException( const QString& message );
    ~ParseException() throw();
    QString message() const;
private:
    QString m_message;
};

class Job : public KJob {

public:
    explicit Job( QObject* parent=0 );

    enum Error {
        IOError=KJob::UserDefinedError,
        XmlError,
        ParseError,
        OwncloudUserDefinedError
    };

    KUrl url() const;
    void setUrl( const KUrl& );

    QString username() const;
    void setUsername( const QString& username );

    QString password() const;
    void setPassword( const QString& password );

    Akonadi::Collection collection() const;
    void setCollection( const Akonadi::Collection& collection );

    void start();

protected:
    void setPath( const QString& );
    QString path() const;

private:
    Q_INVOKABLE virtual void doStart() = 0;

private:
    KUrl m_url;
    QString m_path;
    QString m_username;
    QString m_password;
    Akonadi::Collection m_collection;
};

class GetJob : public Job {
    Q_OBJECT
public:
    explicit GetJob( QObject* parent );

    /**
     * @throws ParseException
     */
    virtual void parse( QXmlStreamReader* reader ) = 0;

private:
    Q_INVOKABLE void doStart();
    KUrl assembleUrl( const QString& relpath ) const;

private Q_SLOTS:
    void data( KIO::Job* job, const QByteArray& data );
    void jobFinished( KJob* job );

private:
    QByteArray m_buffer;
};


class PostJob : public Job {
    Q_OBJECT
public:
    explicit PostJob( const QString& path, QObject* parent=0 );

    void insertData( const QString& key, const QString& value );

private:
     Q_INVOKABLE void doStart();
     KUrl assembleUrl( const QString& path ) const;

private Q_SLOTS:
     void jobFinished( KJob* job );

private:
     QMap<QString,QString> m_postData;
};

class ListNodeJob : public GetJob {
    Q_OBJECT
public:
    enum Type {
        Folders,
        Feeds
    };

    explicit ListNodeJob( Type type, QObject* parent=0 );

    struct Node {
        QString title;
        QString id;
        QString parentId;
        QString icon;
        QString link;
    };

    struct Meta {
        QString status;
        int statuscode;
        QString message;
    };

    QVector<Node> children() const;

protected:
    void parse( QXmlStreamReader* reader );

private:
    QVector<Node> m_children;
    Type m_type;
};

#endif
