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
#include <QProcess>

#include <qtest_kde.h>

#include <KDebug>

#include "../ispdb.h"
#include "../autoconfigkolabmail.h"

class TAutoconfMail : public AutoconfigKolabMail
{
public:
    void startJob(const KUrl &url)
    {
        QCOMPARE(url, expectedUrls.takeFirst());
        if (replace.contains(url)) {
            AutoconfigKolabMail::startJob(replace[url]);
        } else {
            AutoconfigKolabMail::startJob(url);
        }
    }

    QMap<KUrl, KUrl> replace;
    QList<KUrl> expectedUrls;
};

class AutoconfMailTest : public QObject
{
    Q_OBJECT
public:
    TAutoconfMail *getAutoconf()
    {
        return new TAutoconfMail();
    }

    AutoconfigKolabMail *execIspdb(const QString &file)
    {
        QDir dir(QLatin1String(AUTOCONFIG_DATA_DIR));
        QString furl = QLatin1String("file://%1/%2");

        QEventLoop loop;
        TAutoconfMail *ispdb = getAutoconf();

        loop.connect(ispdb, SIGNAL(finished(bool)), SLOT(quit()));

        KUrl url(furl.arg(dir.path()).arg(file));
        ispdb->setEmail(QLatin1String("john.doe@example.com"));
        ispdb->expectedUrls.append(url);
        ispdb->startJob(url);

        loop.exec();
        return ispdb;
    }

private slots:
    void testCompleteFail()
    {
        QEventLoop loop;
        TAutoconfMail *ispdb = getAutoconf();

        loop.connect(ispdb, SIGNAL(finished(bool)), SLOT(quit()));
        connect(ispdb, SIGNAL(finished(bool)), SLOT(expectedReturn(bool)));

        KUrl expected0(QLatin1String("https://autoconfig.thunderbird.net/v1.1/example.com"));
        KUrl expected1(QLatin1String("http://autoconfig.example.com/mail/config-v1.1.xml"));
        KUrl expected2(QLatin1String("http://example.com/.well-known/autoconfig/mail/config-v1.1.xml"));
        mReturn = false;
        ispdb->setEmail(QLatin1String("john.doe@example.com"));
        ispdb->expectedUrls.append(expected0);
        ispdb->expectedUrls.append(expected1);
        ispdb->expectedUrls.append(expected2);
        ispdb->replace[expected0] = QLatin1String("http://localhost:8000/500");
        ispdb->replace[expected1] = QLatin1String("http://localhost:8000/500");
        ispdb->replace[expected2] = QLatin1String("http://localhost:8000/404");
        ispdb->start();
        loop.exec();
        QCOMPARE(ispdb->expectedUrls.count(), 0);
    }

    void testLogin()
    {
        QDir dir(QLatin1String(AUTOCONFIG_DATA_DIR));
        QString furl = QLatin1String("file://%1/autoconfig.xml");

        QEventLoop loop;
        TAutoconfMail *ispdb = getAutoconf();

        loop.connect(ispdb, SIGNAL(finished(bool)), SLOT(quit()));
        connect(ispdb, SIGNAL(finished(bool)), SLOT(expectedReturn(bool)));

        KUrl expected0(QLatin1String("https://autoconfig.thunderbird.net/v1.1/example.com"));
        KUrl expected1(QLatin1String("http://autoconfig.example.com/mail/config-v1.1.xml"));
        KUrl expected2(QLatin1String("https://john.doe%40example.com:xxx@autoconfig.example.com/mail/config-v1.1.xml"));
        KUrl expected3(QLatin1String("http://example.com/.well-known/autoconfig/mail/config-v1.1.xml"));

        mReturn = true;
        ispdb->setEmail(QLatin1String("john.doe@example.com"));
        ispdb->setPassword(QLatin1String("xxx"));
        ispdb->expectedUrls.append(expected0);
        ispdb->expectedUrls.append(expected1);
        ispdb->expectedUrls.append(expected2);
        ispdb->expectedUrls.append(expected3);
        ispdb->replace[expected0] = QLatin1String("http://localhost:8000/500");
        ispdb->replace[expected1] = QLatin1String("http://localhost:8000/401");
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

QTEST_KDEMAIN(AutoconfMailTest, NoGUI)

#include "autoconfigkolabmailtest.moc"
