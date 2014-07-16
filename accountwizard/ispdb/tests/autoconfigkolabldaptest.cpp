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

#include "../autoconfigkolabldap.h"

class TAutoconfLdap : public AutoconfigKolabLdap
{
public:
    void startJob(const KUrl &url)
    {
        QCOMPARE(url, expectedUrls.takeFirst());
        if (replace.contains(url)) {
            AutoconfigKolabLdap::startJob(replace[url]);
        } else {
            AutoconfigKolabLdap::startJob(url);
        }
    }

    QMap<KUrl, KUrl> replace;
    QList<KUrl> expectedUrls;
};

class AutoconfLdapTest : public QObject
{
    Q_OBJECT
public:
    AutoconfigKolabLdap *execIspdb(const QString &file)
    {
        QDir dir(QLatin1String(AUTOCONFIG_DATA_DIR));
        QString furl = QLatin1String("file://%1/%2");

        QEventLoop loop;
        TAutoconfLdap *ispdb = new TAutoconfLdap();

        loop.connect(ispdb, SIGNAL(finished(bool)), SLOT(quit()));

        KUrl url(furl.arg(dir.path()).arg(file));
        ispdb->setEmail(QLatin1String("john.doe@example.com"));
        ispdb->expectedUrls.append(url);
        ispdb->startJob(url);

        loop.exec();
        return ispdb;
    }

    void testLdapServer(const ldapServer &test, const ldapServer &expected) const {
        QCOMPARE(test.hostname, expected.hostname);
        QCOMPARE(test.port, expected.port);
        QCOMPARE(test.socketType, expected.socketType);
        QCOMPARE(test.authentication, expected.authentication);
        QCOMPARE(test.bindDn, expected.bindDn);
        QCOMPARE(test.password, expected.password);
        QCOMPARE(test.saslMech, expected.saslMech);
        QCOMPARE(test.username, expected.username);
        QCOMPARE(test.realm, expected.realm);
        QCOMPARE(test.dn, expected.dn);
        QCOMPARE(test.ldapVersion, expected.ldapVersion);
        QCOMPARE(test.filter, expected.filter);
        QCOMPARE(test.pageSize, expected.pageSize);
        QCOMPARE(test.timeLimit, expected.timeLimit);
        QCOMPARE(test.sizeLimit, expected.sizeLimit);
    }

private slots:
    void testLdapParsing()
    {
        AutoconfigKolabLdap *ispdb = execIspdb(QLatin1String("ldap.xml"));

        ldapServer s;
        QCOMPARE(ispdb->ldapServers().count(), 2);

        s.hostname = QLatin1String("ldap.example.com");
        s.port = 389;
        s.socketType = KLDAP::LdapServer::None;
        s.authentication = KLDAP::LdapServer::Simple;
        s.bindDn = QLatin1String("cn=Directory Manager");
        s.password = QLatin1String("Welcome2KolabSystems");
        s.saslMech = QString();
        s.username = QString();
        s.realm = QString();
        s.dn = QLatin1String("dc=kolabsys,dc=com");
        s.ldapVersion = 3;
        s.filter = QString();
        s.pageSize = -1;
        s.timeLimit = -1;
        s.sizeLimit = -1;

        testLdapServer(ispdb->ldapServers()[QLatin1String("ldap.example.com")], s);

        s.hostname = QLatin1String("ldap2.example.com");
        s.port = 387;
        s.socketType = KLDAP::LdapServer::SSL;
        s.authentication = KLDAP::LdapServer::SASL;
        s.bindDn = QLatin1String("cn=Directory");
        s.password = QLatin1String("Welcome2KolabSystems");
        s.saslMech = QLatin1String("XXX");
        s.username = QLatin1String("john.doe");
        s.realm = QLatin1String("realm.example.com");
        s.dn = QLatin1String("dc=example,dc=com");
        s.ldapVersion = 3;
        s.filter = QString();
        s.pageSize = 10;
        s.timeLimit = -1;
        s.sizeLimit = 9999999;

        testLdapServer(ispdb->ldapServers()[QLatin1String("ldap2.example.com")], s);
    }

    void testLdapCompleteFail()
    {
        QEventLoop loop;
        TAutoconfLdap *ispdb = new TAutoconfLdap();

        loop.connect(ispdb, SIGNAL(finished(bool)), SLOT(quit()));
        connect(ispdb, SIGNAL(finished(bool)), SLOT(expectedReturn(bool)));

        KUrl expected(QLatin1String("http://autoconfig.example.com/ldap/config-v1.0.xml"));
        KUrl expected2(QLatin1String("http://example.com/.well-known/autoconfig/ldap/config-v1.0.xml"));

        mReturn = false;
        ispdb->setEmail(QLatin1String("john.doe@example.com"));
        ispdb->expectedUrls.append(expected);
        ispdb->expectedUrls.append(expected2);
        ispdb->replace[expected] = QLatin1String("http://localhost:8000/500");
        ispdb->replace[expected2] = QLatin1String("http://localhost:8000/404");
        ispdb->start();
        loop.exec();
        QCOMPARE(ispdb->expectedUrls.count(), 0);
    }

    void testLdapLogin()
    {
        QDir dir(QLatin1String(AUTOCONFIG_DATA_DIR));
        QString furl = QLatin1String("file://%1/ldap.xml");

        QEventLoop loop;
        TAutoconfLdap *ispdb = new TAutoconfLdap();

        loop.connect(ispdb, SIGNAL(finished(bool)), SLOT(quit()));
        connect(ispdb, SIGNAL(finished(bool)), SLOT(expectedReturn(bool)));

        KUrl expected(QLatin1String("http://autoconfig.example.com/ldap/config-v1.0.xml"));
        KUrl expected2(QLatin1String("https://john.doe%40example.com:xxx@autoconfig.example.com/ldap/config-v1.0.xml"));
        KUrl expected3(QLatin1String("http://example.com/.well-known/autoconfig/ldap/config-v1.0.xml"));

        mReturn = true;
        ispdb->setEmail(QLatin1String("john.doe@example.com"));
        ispdb->setPassword(QLatin1String("xxx"));
        ispdb->expectedUrls.append(expected);
        ispdb->expectedUrls.append(expected2);
        ispdb->expectedUrls.append(expected3);
        ispdb->replace[expected] = QLatin1String("http://localhost:8000/401");
        ispdb->replace[expected2] = QLatin1String("http://localhost:8000/500");
        ispdb->replace[expected3] = furl.arg(dir.path());
        ispdb->start();
        loop.exec();
        QCOMPARE(ispdb->expectedUrls.count(), 0);
    }

    void expectedReturn(bool ret)
    {
        QCOMPARE(ret, mReturn);
    }

    void initTestCase()
    {
        QDir dir(QLatin1String(CURRENT_SOURCE_DIR));
        QString furl = QLatin1String("%1/errorserver.py");
        process.start(QLatin1String("python"), QStringList() << furl.arg(dir.path()));
        process.waitForStarted();
        QCOMPARE(process.state(), QProcess::Running);
    }

    void cleanupTestCase()
    {
        process.terminate();
        process.waitForFinished();
    }

public:
    bool mReturn;
    QProcess process;
};

QTEST_KDEMAIN(AutoconfLdapTest, NoGUI)

#include "autoconfigkolabldaptest.moc"
