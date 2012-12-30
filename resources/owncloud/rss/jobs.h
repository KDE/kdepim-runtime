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
    Q_OBJECT
public:
    explicit Job( QObject* parent );

    void start();

    KUrl url() const;
    void setUrl( const KUrl& );

    QString username() const;
    void setUsername( const QString& username );

    QString password() const;
    void setPassword( const QString& password );

    enum Error {
        IOError=KJob::UserDefinedError,
        XmlError,
        ParseError,
        OwncloudUserDefinedError
    };

protected:
    void setPath( const QString& );
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
    KUrl m_url;
    QString m_path;
    QString m_username;
    QString m_password;
    QByteArray m_buffer;
};


class ListNodeJob : public Job {
    Q_OBJECT
public:
    explicit ListNodeJob( QObject* parent );

    void setNodePath( const QString& nodePath );

    struct Node {
        QString title;
        QString id;
    };

    QVector<Node> children() const;

protected:
    void parse( QXmlStreamReader* reader );

private:
    QString m_nodePath;
    QVector<Node> m_children;
};

#endif
