/*
    Copyright (c) 2010 Omat Holding B.V. <info@omat.nl>

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

#ifndef ISPDB_H
#define ISPDB_H

#include <QObject>

#include <kio/job.h>
#include <kmime/kmime_header_parsing.h>
#include <QUrl>

class QDomElement;

struct server;

/**
  This class will search in Mozilla's database for an xml file
  describing the isp data belonging to that address. This class
  searches and wraps the result for further usage. References:
    https://wiki.mozilla.org/Thunderbird:Autoconfiguration
    https://developer.mozilla.org/en/Thunderbird/Autoconfiguration
    https://ispdb.mozillamessaging.com/
*/
class Ispdb : public QObject
{
    Q_OBJECT
public:
    enum socketType { None = 0, SSL, StartTLS };

    /**
     Ispdb uses custom authtyps, hence the enum here.
     @see https://wiki.mozilla.org/Thunderbird:Autoconfiguration:ConfigFileFormat
     In particular, note that Ispdb's Plain represents both Cleartext and AUTH Plain
     We will always treat it as Cleartext
     */
    enum authType { Plain = 0, CramMD5, NTLM, GSSAPI, ClientIP, NoAuth };
    enum length { Long = 0, Short };

    /** Constructor */
    explicit Ispdb(QObject *parent = 0);

    /** Destructor */
    ~Ispdb();

    /** After finished() has been emitted you can
        retrieve the domains that are covered by these
        settings */
    QStringList relevantDomains() const;

    /** After finished() has been emitted you can
        get the name of the provider, you can get a long
        name and a short one */
    QString name(length) const;

    /** After finished() has been emitted you can
        get a list of imap servers available for this provider */
    QList< server > imapServers() const;

    /** After finished() has been emitted you can
        get a list of pop3 servers available for this provider */
    QList< server > pop3Servers() const;

    /** After finished() has been emitted you can
        get a list of smtp servers available for this provider */
    QList< server > smtpServers() const;

public slots:
    /** Sets the emailaddress you want to servers for */
    void setEmail(const QString &);

    /** Starts looking up the servers which belong to the e-mailaddress */
    void start();

private slots:
    void slotResult(KJob *);
    void dataArrived(KIO::Job *, const QByteArray &data);

signals:
    /** emitted when done. **/
    void finished(bool);
    void searchType(const QString &type);

private:
    enum searchServerType { IspAutoConfig = 0, IspWellKnow, DataBase };

    server createServer(const QDomElement &n);
    void lookupInDb();
    QString replacePlaceholders(const QString &);
    void startJob(const QUrl &url);

    KMime::Types::AddrSpec mAddr; // emailaddress
    QByteArray mData;             // storage of incoming data from kio

    // storage of the results
    QStringList mDomains;
    QString mDisplayName, mDisplayShortName;
    QList< server > mImapServers, mPop3Servers, mSmtpServers;
    Ispdb::searchServerType mServerType;
};

struct server {
    server()
    {
        port = -1;
        authentication = Ispdb::Plain;
        socketType = Ispdb::None;
    }
    bool isValid() const
    {
        return (port != -1);
    }
    QString hostname;
    int port;
    Ispdb::socketType socketType;
    QString username;
    Ispdb::authType authentication;
};

#endif
