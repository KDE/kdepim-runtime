/*
    Copyright (c) 2010 Omat Holding B.V. <info@omat.nl>
    Copyright (c) 2014  Sandro Knauß <knauss@kolabsys.com>

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

class QDomElement;
class QDomDocument;

struct server;
struct identity;

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
    enum authType { Plain = 0, CramMD5, NTLM, GSSAPI, ClientIP, NoAuth, Basic };
    enum length { Long = 0, Short };

    /** Constructor */
    explicit Ispdb( QObject *parent = 0 );

    /** Destructor */
    ~Ispdb();

    /** After finished() has been emitted you can
        retrieve the domains that are covered by these
        settings */
    QStringList relevantDomains() const;

    /** After finished() has been emitted you can
        get the name of the provider, you can get a long
        name and a short one */
    QString name( length ) const;

    /** After finished() has been emitted you can
        get a list of imap servers available for this provider */
    QList< server > imapServers() const;

    /** After finished() has been emitted you can
        get a list of pop3 servers available for this provider */
    QList< server > pop3Servers() const;

    /** After finished() has been emitted you can
        get a list of smtp servers available for this provider */
    QList< server > smtpServers() const;

    QList< identity > identities() const;

    int defaultIdentity() const;

public slots:
    /** Sets the emailaddress you want to servers for */
    void setEmail( const QString& );

    /** Sets the password for login */
    void setPassword( const QString& );

    /** Starts looking up the servers which belong to the e-mailaddress */
    void start();

private slots:
    void slotResult( KJob* );

signals:
    /** emitted when done. **/
    void finished( bool );
    void searchType( const QString &type );

protected:
    /** search types, where to search for autoconfig
        @see lookupUrl to geneerate a url base on this type
     */
    enum searchServerType {
      IspAutoConfig = 0,                  /**< http://autoconfig.example.com/mail/config-v1.1.xml */
      IspWellKnow,                        /**< http://example.com/.well-known/autoconfig/mail/config-v1.1.xml */
      DataBase                            /**< https://autoconfig.thunderbird.net/v1.1/example.com */
    };

    /** let's request the autoconfig server */
    virtual void startJob( const KUrl &url );

    /** generate url and start job afterwards */
    virtual void lookupInDb( bool auth, bool crypt );

    /** an valid xml document is available, parse it and create all the objects
        should run createServer, createIdentity, ...
     */
    virtual void parseResult( const QDomDocument &document );

    /** create a server object out of an element */
    virtual server createServer( const QDomElement& n );

    /** create a identity object out of an element */
    virtual identity createIdentity( const QDomElement& n );

    /** get standard urls for autoconfig
        @return the standard url for autoconfig depends on serverType
        @param type of request ( ex. "mail" )
        @param version of the file ( example for mail "1.1" )
        @param auth use authentification with username & password to access autoconfig file
                    ( username is the emailaddress )
        @param crypt use https
     */
    KUrl lookupUrl( const QString& type, const QString& version, bool auth, bool crypt );

    /** setter for serverType */
    void setServerType( Ispdb::searchServerType type );

    /** getter for serverType */
    Ispdb::searchServerType serverType() const;

    /** replaces %EMAILLOCALPART%, %EMAILADDRESS% and %EMAILDOMAIN% with the
        parts of the emailaddress */
    QString replacePlaceholders( const QString& );

    QByteArray mData;             /** storage of incoming data from kio */
protected slots:

    /** slot for TransferJob to dump data */
    void dataArrived( KIO::Job*, const QByteArray& data );

private:
    KMime::Types::AddrSpec mAddr; // emailaddress
    QString mPassword;

    // storage of the results
    QStringList mDomains;
    QString mDisplayName, mDisplayShortName;
    QList< server > mImapServers, mPop3Servers, mSmtpServers;
    QList< identity > mIdentities;

    int mDefaultIdentity;
    Ispdb::searchServerType mServerType;
    bool mStart;
};

struct server {
    server() {
       port = -1;
       authentication = Ispdb::Plain;
       socketType = Ispdb::None;
    }
    bool isValid() const {
       return (port != -1);
    }
    QString hostname;
    int port;
    Ispdb::socketType socketType;
    QString username;
    Ispdb::authType authentication;
};

struct identity {
    identity()
         : mDefault(false)
    {
    }

    bool isValid() const {
        return !name.isEmpty();
    }

    bool isDefault() const {
        return mDefault;
    }

    bool mDefault;
    QString email;
    QString name;
    QString organization;
    QString signature;
};


#endif
