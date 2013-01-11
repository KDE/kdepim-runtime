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
#include <QTime>
#include <KDE/KJob>
#include <Soprano/QueryResultIterator>
#include <Soprano/Model>
#include <nepomuk2/simpleresourcegraph.h>
#include <nepomuk2/datamanagement.h>
#include <nepomuk2/resourcemanager.h>
#include <KDebug>
#include <aneo.h>
#include <findunindexeditemsjob.h>
#include <akonadi/qtest_akonadi.h>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/Collection>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/RecursiveItemFetchJob>
#include "nepomukfeeder-config.h"

/**
 */
class QueryTest: public QObject
{
    Q_OBJECT
    
private slots:

    void testNepomukQueryByType()
    {
        QTime time;
        time.start();
        Soprano::QueryResultIterator result = Nepomuk2::ResourceManager::instance()->mainModel()->executeQuery(QString::fromLatin1("SELECT ?r WHERE { ?r a %1 }")
                .arg(Soprano::Node::resourceToN3(Vocabulary::ANEO::AkonadiDataObject())),
                Soprano::Query::QueryLanguageSparql);
        kDebug() << "Query took(ms): " << time.elapsed();
        time.start();
        int i = 0;
        while (result.next()) {
            i++;
        }
        kDebug() << "total " << i << " results";
        kDebug() << "Query took(ms): " << time.elapsed();
    }
    
    void testNepomukQueryByTypeAndVersion()
    {
        QTime time;
        time.start();
        Soprano::QueryResultIterator result = Nepomuk2::ResourceManager::instance()->mainModel()->executeQuery(QString::fromLatin1("SELECT ?r WHERE { ?r a %1 . ?r %2 %3 }")
                .arg(Soprano::Node::resourceToN3(Vocabulary::ANEO::AkonadiDataObject()),
                Soprano::Node::resourceToN3(Vocabulary::ANEO::akonadiIndexCompatLevel()),
                Soprano::Node::literalToN3(3)
                ),
                Soprano::Query::QueryLanguageSparql);
        kDebug() << "Query took(ms): " << time.elapsed();
        time.start();
        int i = 0;
        while (result.next()) {
            i++;
        }
        kDebug() << "total " << i << " results";
        kDebug() << "Query took(ms): " << time.elapsed();
    }

    void testAkonadiQuery()
    {
        QTime time;
        time.start();
        Akonadi::RecursiveItemFetchJob *itemFetchJob = new Akonadi::RecursiveItemFetchJob(Akonadi::Collection::root(), QStringList(), this);
        itemFetchJob->fetchScope().fetchAllAttributes(false);
        itemFetchJob->fetchScope().fetchFullPayload(false);
        itemFetchJob->exec();
        kDebug() << "Query took(ms): " << time.elapsed();
        kDebug() << "items " << itemFetchJob->items().size();
    }

    void testFindJob()
    {
        QTime time;
        time.start();
        FindUnindexedItemsJob *job = new FindUnindexedItemsJob(NEPOMUK_FEEDER_INDEX_COMPAT_LEVEL, this);
        job->exec();
        kDebug() << job->getUnindexed().size();
        kDebug() << "Query took(ms): " << time.elapsed();
    }
};


QTEST_AKONADIMAIN( QueryTest, NoGUI )

#include "querytest.moc"