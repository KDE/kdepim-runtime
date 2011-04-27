/*
    Copyright 2011 Sebastian Kügler <sebas@kde.org>

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

#include "ispdbengine.h"

#include "../ispdb.h"

#define RESULT_LIMIT 10

class IspdbEnginePrivate
{
public:
    QString query;
    QSize previewSize;
    QHash<QString, QString> icons;
};


IspdbEngine::IspdbEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent)
{
    Q_UNUSED(args);
    d = new IspdbEnginePrivate;
    setMaxSourceCount(RESULT_LIMIT); // Guard against loading too many connections
    init();
}

void IspdbEngine::init()
{
}

IspdbEngine::~IspdbEngine()
{
    delete d;
}

QStringList IspdbEngine::sources() const
{
    return QStringList();
}

bool IspdbEngine::sourceRequestEvent(const QString &name)
{
    Ispdb *db = new Ispdb(this);

    connect(db, SIGNAL(finished(bool)), this, SLOT(onIspDbRequestFinished(bool)));

    db->setEmail(name);
    db->setProperty("__ispdb_engine_email", name);
    db->start();

    return true;
}

void IspdbEngine::onIspDbRequestFinished(bool result)
{
    kDebug() << "result:" << result;
    Ispdb *db = qobject_cast< Ispdb* >(sender());

    QStringList allServers;
    foreach (const server &s, db->imapServers()) {
        allServers.append(populateSource(s, "IMAP"));
    }

    foreach (const server &s, db->smtpServers()) {
        allServers.append(populateSource(s, "SMTP"));
    }

    foreach (const server &s, db->pop3Servers()) {
        allServers.append(populateSource(s, "POP3"));
    }

    QString email = db->property("__ispdb_engine_email").toString();

    setData(email, "successful", !allServers.isEmpty());
    if (!allServers.isEmpty()) {
        setData(email, "name", db->name(Ispdb::Short));
        setData(email, "longName", db->name(Ispdb::Long));
        setData(email, "relevantDomains", db->relevantDomains());
        setData(email, "allServers", allServers);
    }
    scheduleSourcesUpdated();
}

QString IspdbEngine::populateSource(server s, const QString& protocol)
{
    QString source = QString("%1@%2:%3").arg(s.username).arg(s.hostname).arg(s.port);
    QString socketType = QString("None");
    switch (s.socketType) {
        case Ispdb::SSL:
            socketType = QString("SSL");
            break;
        case Ispdb::StartTLS:
            socketType = QString("StartTLS");
            break;
        default:
            break;
    }

    QString authType = QString("Plain");
    switch (s.authentication) {
        case Ispdb::ClientIP:
            authType = QString("ClientIP");
            break;
        case Ispdb::CramMD5:
            authType = QString("CramMD5");
            break;
        case Ispdb::GSSAPI:
            authType = QString("GSSAPI");
            break;
        case Ispdb::NoAuth:
            authType = QString("NoAuth");
            break;
        case Ispdb::NTLM:
            authType = QString("NTLM");
            break;
        default:
            break;
    }

    setData(source, "hostname", s.hostname);
    setData(source, "username", s.username);
    setData(source, "port", s.port);
    setData(source, "protocol", protocol);
    setData(source, "socketType", socketType);
    setData(source, "authType", authType);

    return source;
}

#include "ispdbengine.moc"
