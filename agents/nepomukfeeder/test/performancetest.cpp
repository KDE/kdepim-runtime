/*
    Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <QtCore/QObject>
#include <KDE/KJob>
#include <nepomuk2/simpleresourcegraph.h>
#include <nepomuk2/datamanagement.h>
#include <nepomuk2/storeresourcesjob.h>
#include <nepomuk2/simpleresource.h>
#include <akonadi/qtest_akonadi.h>
#include <akonadi/item.h>
#include <kmime/kmime_message.h>
#include <limits>
#include "../nepomukhelpers.h"
#include <propertycache.h>
#define TESTFILEDIR QString::fromLatin1(TEST_DATA_PATH "/testfiles/")

/**
 * This "test" should hopefully give a reproducible base to compare indexing speeds accross systems.
 * It's not an automated test which can fail, if we have some reliable numbers we could make sure it doesn't get worse than that.
 * Note that the code writes directly to the real nepomuk db, but with an akonadi item id which is never going to exist.
 */
class PerformanceTest: public QObject
{
    Q_OBJECT

    static KMime::Message::Ptr readMimeFile( const QString &fileName)
    {
        //   qDebug() << fileName;
        QFile file( fileName );
        if (!file.open( QFile::ReadOnly )) {
            kWarning() << "failed to open file: " << fileName;
            return KMime::Message::Ptr();
        }
        const QByteArray data = file.readAll();

        KMime::Message::Ptr msg = KMime::Message::Ptr(new KMime::Message);
        msg->setContent( data );
        msg->parse();
        return msg;
    }
    
private slots:

    void testMail_data()
    {
        QTest::addColumn<QString>("file");
        QTest::newRow("simpleMail") << TESTFILEDIR.append(QString::fromLatin1("simplemail.mime"));
//         QTest::newRow("realworldmail") << TESTFILEDIR.append(QString::fromLatin1("realworldmail.mime"));
//         QTest::newRow("mailWithCoupleOfCCs") << TESTFILEDIR.append(QString::fromLatin1("mailWithACoupleOfCCs.mime"));
        //FIXME That's to much so far for nepomuk
//         QTest::newRow("mailWithLoadsOfCCs") << TESTFILEDIR.append(QString::fromLatin1("mailWithLoadsOfCCs.mime"));
    }

    void jobResult(KJob *job)
    {
        if (job->error()) {
            kWarning() << "Error:";
            kWarning() << job->errorString();
        }
    }
    
    void testMail()
    {
        QTime time;
        time.start();
        Akonadi::Item item(std::numeric_limits < Akonadi::Entity::Id >::max());
        item.setModificationTime(QDateTime(QDate(2012, 01, 02)));
        QFETCH(QString, file);
        KMime::Message::Ptr  msg = readMimeFile(file);
        QVERIFY(msg);
        item.setPayload(msg);
        item.setMimeType(KMime::Message::mimeType());
        Nepomuk2::SimpleResourceGraph graph;
        NepomukHelpers::addItemToGraph( item, graph );
        KJob *addGraphJob = NepomukHelpers::addGraphToNepomuk( graph );
        connect(addGraphJob, SIGNAL(result(KJob*)), this, SLOT(jobResult(KJob*)));
        addGraphJob->exec();
        kDebug() << "Storing the graph took(ms): " << time.elapsed();
        time.start();
        KJob *removeJob = Nepomuk2::removeResources( QList <QUrl>() << item.url(), Nepomuk2::RemoveSubResoures );
        connect(removeJob, SIGNAL(result(KJob*)), this, SLOT(jobResult(KJob*)));
        removeJob->exec();
        kDebug() << "Removing the data took(ms): " << time.elapsed();
    }
    
    void testMailCached_data()
    {
        QTest::addColumn<QString>("file");
        QTest::newRow("simpleMail") << TESTFILEDIR.append(QString::fromLatin1("simplemail.mime"));
    }

        
    
    void testMailCached()
    {
        QTime time;
        time.start();
        Akonadi::Item item(std::numeric_limits < Akonadi::Entity::Id >::max());
        item.setModificationTime(QDateTime(QDate(2012, 01, 02)));
        QFETCH(QString, file);
        KMime::Message::Ptr  msg = readMimeFile(file);
        QVERIFY(msg);
        item.setPayload(msg);
        item.setMimeType(KMime::Message::mimeType());
        
        Nepomuk2::SimpleResourceGraph graph;
        NepomukHelpers::addItemToGraph( item, graph );
        
        KJob *addGraphJob = NepomukHelpers::addGraphToNepomuk( graph );
        connect(addGraphJob, SIGNAL(result(KJob*)), this, SLOT(jobResult(KJob*)));
        addGraphJob->exec();
        kDebug() << "Storing the graph took(ms): " << time.elapsed();

        Nepomuk2::StoreResourcesJob *storeJob = static_cast<Nepomuk2::StoreResourcesJob*>(addGraphJob);
        const QHash<QUrl, QUrl> mappings = storeJob->mappings();
        qDebug() << mappings;

        PropertyCache propertyCache;
        propertyCache.setCachedTypes(QList<QUrl>()
           << QUrl("http://www.semanticdesktop.org/ontologies/2007/08/15/nao#FreeDesktopIcon")
           << QUrl("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#EmailAddress")
           << QUrl("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#Contact")
        );
        propertyCache.fillCache(graph, mappings);
        
        time.start();
        KJob *removeJob = Nepomuk2::removeResources( QList <QUrl>() << item.url(), Nepomuk2::RemoveSubResoures );
        connect(removeJob, SIGNAL(result(KJob*)), this, SLOT(jobResult(KJob*)));
        removeJob->exec();
        kDebug() << "Removing the data took(ms): " << time.elapsed();
        
        Nepomuk2::SimpleResourceGraph graph2;
        NepomukHelpers::addItemToGraph( item, graph2 );
        
        const Nepomuk2::SimpleResourceGraph resultGraph = propertyCache.applyCache(graph2);

        addGraphJob = NepomukHelpers::addGraphToNepomuk( resultGraph );
        connect(addGraphJob, SIGNAL(result(KJob*)), this, SLOT(jobResult(KJob*)));
        addGraphJob->exec();
        kDebug() << "Storing the graph again took(ms): " << time.elapsed();

        time.start();
        removeJob = Nepomuk2::removeResources( QList <QUrl>() << item.url(), Nepomuk2::RemoveSubResoures );
        connect(removeJob, SIGNAL(result(KJob*)), this, SLOT(jobResult(KJob*)));
        removeJob->exec();
        kDebug() << "Removing the data took(ms): " << time.elapsed();
    }


};


QTEST_AKONADIMAIN( PerformanceTest, NoGUI )

#include "performancetest.moc"