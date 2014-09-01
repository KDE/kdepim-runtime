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

#include "ispdb.h"
#include <qdebug.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <KLocalizedString>

#include <kmime/kmime_header_parsing.h>
#include <QDomDocument>

Ispdb::Ispdb(QObject *parent)
    : QObject(parent), mServerType(DataBase)
{
}

Ispdb::~Ispdb()
{
}

void Ispdb::setEmail(const QString &address)
{
    KMime::Types::Mailbox box;
    box.fromUnicodeString(address);
    mAddr = box.addrSpec();
}

void Ispdb::start()
{
    qDebug() << mAddr.asString();
    // we should do different things in here. But lets focus in the db first.
    lookupInDb();
}

void Ispdb::startJob(const QUrl &url)
{
    QMap< QString, QVariant > map;
    map[QLatin1String("errorPage")] = false;

    KIO::TransferJob *job = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);
    job->setMetaData(map);
    connect(job, &KIO::TransferJob::result, this, &Ispdb::slotResult);
    connect(job, &KIO::TransferJob::data, this, &Ispdb::dataArrived);
}

void Ispdb::lookupInDb()
{
    QUrl url;
    switch (mServerType) {
    case IspAutoConfig: {
        url = QUrl(QLatin1String("http://autoconfig.") + mAddr.domain.toLower() + QLatin1String("/mail/config-v1.1.xml?emailaddress=") + mAddr.asString().toLower());
        Q_EMIT searchType(i18n("Lookup configuration: Email provider"));
        break;
    }
    case IspWellKnow: {
        url = QUrl(QLatin1String("http://") + mAddr.domain.toLower() + QLatin1String("/.well-known/autoconfig/mail/config-v1.1.xml"));
        Q_EMIT searchType(i18n("Lookup configuration: Trying common server name"));
        break;
    }
    case DataBase: {
        url = QUrl(QLatin1String("https://autoconfig.thunderbird.net/v1.1/") + mAddr.domain.toLower());
        Q_EMIT searchType(i18n("Lookup configuration: Mozilla database"));
        break;
    }
    }
    startJob(url);
}

void Ispdb::slotResult(KJob *job)
{
    if (job->error()) {
        qDebug() << "Fetching failed" << job->errorString();
        bool lookupFinished = false;

        switch (mServerType) {
        case IspAutoConfig: {
            mServerType = IspWellKnow;
            break;
        }
        case IspWellKnow: {
            lookupFinished = true;
            break;
        }
        case DataBase: {
            mServerType = IspAutoConfig;
            break;
        }
        }

        if (lookupFinished) {
            emit finished(false);
            return;
        }
        lookupInDb();
        return;
    }

    //qDebug() << mData;
    QDomDocument document;
    bool ok = document.setContent(mData);
    if (!ok) {
        qDebug() << "Could not parse xml" << mData;
        emit finished(false);
        return;
    }

    QDomElement docElem = document.documentElement();
    QDomNode m = docElem.firstChild(); // emailprovider
    QDomNode n = m.firstChild(); // emailprovider

    while (!n.isNull()) {
        QDomElement e = n.toElement();
        if (!e.isNull()) {
            //qDebug()  << qPrintable( e.tagName() );
            const QString tagName(e.tagName());
            if (tagName == QLatin1String("domain")) {
                mDomains << e.text();
            } else if (tagName == QLatin1String("displayName")) {
                mDisplayName = e.text();
            } else if (tagName == QLatin1String("displayShortName")) {
                mDisplayShortName = e.text();
            } else if (tagName == QLatin1String("incomingServer")
                       && e.attribute(QLatin1String("type")) == QLatin1String("imap")) {
                server s = createServer(e);
                if (s.isValid()) {
                    mImapServers.append(s);
                }
            } else if (tagName == QLatin1String("incomingServer")
                       && e.attribute(QLatin1String("type")) == QLatin1String("pop3")) {
                server s = createServer(e);
                if (s.isValid()) {
                    mPop3Servers.append(s);
                }
            } else if (tagName == QLatin1String("outgoingServer")
                       && e.attribute(QLatin1String("type")) == QLatin1String("smtp")) {
                server s = createServer(e);
                if (s.isValid()) {
                    mSmtpServers.append(s);
                }
            }
        }
        n = n.nextSibling();
    }

    // comment this section out when you are tired of it...
    qDebug() << "------------------ summary --------------";
    qDebug() << "Domains" << mDomains;
    qDebug() << "Name" << mDisplayName << "(" << mDisplayShortName << ")";
    qDebug() << "Imap servers:";
    foreach (const server &s, mImapServers) {
        qDebug() << s.hostname << s.port << s.socketType << s.username << s.authentication;
    }
    qDebug() << "pop3 servers:";
    foreach (const server &s, mPop3Servers) {
        qDebug() << s.hostname << s.port << s.socketType << s.username << s.authentication;
    }
    qDebug() << "smtp servers:";
    foreach (const server &s, mSmtpServers) {
        qDebug() << s.hostname << s.port << s.socketType << s.username << s.authentication;
    }
    // end section.

    emit finished(true);
}

server Ispdb::createServer(const QDomElement &n)
{
    QDomNode o = n.firstChild();
    server s;
    while (!o.isNull()) {
        QDomElement f = o.toElement();
        if (!f.isNull()) {
            const QString tagName(f.tagName());
            if (tagName == QLatin1String("hostname")) {
                s.hostname = replacePlaceholders(f.text());
            } else if (tagName == QLatin1String("port")) {
                s.port = f.text().toInt();
            } else if (tagName == QLatin1String("socketType")) {
                const QString type(f.text());
                if (type == QLatin1String("plain")) {
                    s.socketType = None;
                } else if (type == QLatin1String("SSL")) {
                    s.socketType = SSL;
                }
                if (type == QLatin1String("STARTTLS")) {
                    s.socketType = StartTLS;
                }
            } else if (tagName == QLatin1String("username")) {
                s.username = replacePlaceholders(f.text());
            } else if (tagName == QLatin1String("authentication")) {
                const QString type(f.text());
                if (type == QLatin1String("password-cleartext")
                        || type == QLatin1String("plain")) {
                    s.authentication = Plain;
                } else if (type == QLatin1String("password-encrypted")
                           || type == QLatin1String("secure")) {
                    s.authentication = CramMD5;
                } else if (type == QLatin1String("NTLM")) {
                    s.authentication = NTLM;
                } else if (type == QLatin1String("GSSAPI")) {
                    s.authentication = GSSAPI;
                } else if (type == QLatin1String("client-ip-based")) {
                    s.authentication = ClientIP;
                } else if (type == QLatin1String("none")) {
                    s.authentication = NoAuth;
                }
            }
        }
        o = o.nextSibling();
    }
    return s;
}

QString Ispdb::replacePlaceholders(const QString &in)
{
    QString out(in);
    out.replace(QLatin1String("%EMAILLOCALPART%"), mAddr.localPart);
    out.replace(QLatin1String("%EMAILADDRESS%"), mAddr.asString());
    out.replace(QLatin1String("%EMAILDOMAIN%"), mAddr.domain);
    return out;
}

void Ispdb::dataArrived(KIO::Job *, const QByteArray &data)
{
    mData.append(data);
}

// The getters

QStringList Ispdb::relevantDomains() const
{
    return mDomains;
}

QString Ispdb::name(length l) const
{
    if (l == Long) {
        return mDisplayName;
    } else if (l == Short) {
        return  mDisplayShortName;
    } else {
        return QString();    //make compiler happy. Not me.
    }
}

QList< server > Ispdb::imapServers() const
{
    return mImapServers;
}

QList< server > Ispdb::pop3Servers() const
{
    return mPop3Servers;
}

QList< server > Ispdb::smtpServers() const
{
    return mSmtpServers;
}

