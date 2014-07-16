/*
    Copyright (c) 2014 Sandro Knauß <knauss@kolabsys.com>

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

#include <QObject>
#include <QDir>
#include <qtest_kde.h>

#include <KDebug>

#include "../ispdb.h"

class TIspdb : public Ispdb
{
public:
    void start(const KUrl &url)
    {
        setServerType(Ispdb::IspWellKnow);
        startJob(url);
    }
};

class IspdbTest : public QObject
{
    Q_OBJECT
public:
    Ispdb *execIspdb(const QString &file)
    {
        QDir dir(QLatin1String(AUTOCONFIG_DATA_DIR));
        QString furl = QLatin1String("file://%1/%2");

        QEventLoop loop;
        TIspdb *ispdb = new TIspdb();

        loop.connect(ispdb, SIGNAL(finished(bool)), SLOT(quit()));

        KUrl url(furl.arg(dir.path()).arg(file));
        ispdb->setEmail(QLatin1String("john.doe@example.com"));
        ispdb->start(url);

        loop.exec();
        return ispdb;
    }

    void testServer(const server &test, const server &expected) const
    {
        QVERIFY(test.isValid());
        QCOMPARE(test.hostname, expected.hostname);
        QCOMPARE(test.port, expected.port);
        QCOMPARE(test.socketType, expected.socketType);
        QCOMPARE(test.username, expected.username);
        QCOMPARE(test.authentication, expected.authentication);
    }

    void testIdendity(const identity &test, const identity &expected) const
    {
        QVERIFY(test.isValid());
        QCOMPARE(test.name, expected.name);
        QCOMPARE(test.email, expected.email);
        QCOMPARE(test.organization, expected.organization);
        QCOMPARE(test.signature, expected.signature);
        QCOMPARE(test.isDefault(), expected.isDefault());
    }

private slots:
    void testParsing()
    {
        Ispdb *ispdb = execIspdb(QLatin1String("autoconfig.xml"));

        server s;
        identity i;

        s.hostname = QLatin1String("imap.example.com");
        s.port = 993;
        s.socketType = Ispdb::SSL;
        s.authentication = Ispdb::CramMD5;
        s.username = QLatin1String("john.doe@example.com");

        QCOMPARE(ispdb->imapServers().count(), 1);
        testServer(ispdb->imapServers().first(), s);

        s.hostname = QLatin1String("smtp.example.com");
        s.port = 25;
        s.socketType = Ispdb::None;
        s.authentication = Ispdb::Plain;
        s.username = QLatin1String("john.doe");
        QCOMPARE(ispdb->smtpServers().count(), 1);
        testServer(ispdb->smtpServers().first(), s);

        s.hostname = QLatin1String("pop.example.com");
        s.port = 995;
        s.socketType = Ispdb::StartTLS;
        s.authentication = Ispdb::NTLM;
        s.username = QLatin1String("example.com");
        QCOMPARE(ispdb->pop3Servers().count(), 1);
        testServer(ispdb->pop3Servers().first(), s);

        i.mDefault = true;
        i.name = QLatin1String("John Doe");
        i.email = QLatin1String("john.doe@example.com");
        i.organization = QLatin1String("Example AG");
        i.signature = QLatin1String("John Doe\n\
Head of World\n\
\n\
Example AG\n\
\n\
w: <a href=\"http://example.com\">http://example.com</a>");

        QCOMPARE(ispdb->identities().count(), 1);
        testIdendity(ispdb->identities().first(), i);
        QCOMPARE(ispdb->defaultIdentity(), 0);
    }
};

QTEST_KDEMAIN(IspdbTest, NoGUI)

#include "ispdbtest.moc"
