/*
 *   Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>
 * 
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtCore/qcoreapplication.h>
#include <feederqueue.h>
#include <nepomukhelpers.h>
#include <nie.h>
#include <aneo.h>
#include <QtCore/QStringList>
#include <akonadi/item.h>
#include <nepomuk/datamanagement.h>
#include <KJob>
#include <KUrl>
#include <Nepomuk/ResourceManager>
#include <Soprano/Node>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>

class Tester: public QObject
{
    Q_OBJECT
public:
    Tester(QObject *o): QObject(o){};
    ~Tester(){};
public Q_SLOTS:
    void progress(int p) {
        kDebug() << "progress " << p;
    }
    
    void running(const QString &p) {
        kDebug() << "running " << p;
    }
    
    void removalComplete(KJob *job) {
        kDebug();
        if (job->error())
            kDebug() << job->errorString();
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    if (app.arguments().size() != 3) {
        kWarning() << "not enough arguments, check the source";
    }
    Tester *tester = new Tester(&app);
    Akonadi::Entity::Id id = app.arguments().at(2).toInt();
    if (app.arguments().at(1) == QString::fromLatin1("item")) {
        ItemQueue *queue = new ItemQueue(1, 1, &app);
        kDebug() << "indexing item: " << id;
        queue->addItem(Akonadi::Item(id));
        queue->processItem();
        QObject::connect( queue, SIGNAL(finished()), &app, SLOT(quit()));
    } else if (app.arguments().at(1) == QString::fromLatin1("rm-item")) {
        kDebug() << "removing item: " << Akonadi::Item(id).url().url();
        KJob *job = Nepomuk::removeDataByApplication( QList<QUrl>() << Akonadi::Item(id).url().url(), Nepomuk::RemoveSubResoures, KGlobal::mainComponent() );
        QObject::connect( job, SIGNAL( finished( KJob* ) ), tester, SLOT( removalComplete( KJob* ) ) );
    } else if (app.arguments().at(1) == QString::fromLatin1("collection")) {
        FeederQueue *feederq = new FeederQueue(&app);
        kDebug() << "indexing collection: " << id;
        feederq->setReindexing(true);
        feederq->setOnline(true);
        feederq->addCollection(Akonadi::Collection(id));

        QObject::connect(feederq, SIGNAL(fullyIndexed()), &app, SLOT(quit()));
        QObject::connect(feederq, SIGNAL(progress(int)), tester, SLOT(progress(int)));
        QObject::connect(feederq, SIGNAL(running(QString)), tester, SLOT(running(QString)));
    } else if (app.arguments().at(1) == QString::fromLatin1("check-collection")) {
        int indexerLevel = 3;
        kDebug() << "Already indexed: " << Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(QString::fromLatin1("ask where { ?r %1 %2 ; %3 %4 . }")
        .arg(Soprano::Node::resourceToN3(Vocabulary::NIE::url()),
             Soprano::Node::resourceToN3(Akonadi::Collection(id).url()),
             Soprano::Node::resourceToN3(Vocabulary::ANEO::akonadiIndexCompatLevel()),
             Soprano::Node::literalToN3(indexerLevel)),
            Soprano::Query::QueryLanguageSparql).boolValue();
        app.quit();
    } else if (app.arguments().at(1) == QString::fromLatin1("mark-collection")) {
        KJob *job = NepomukHelpers::markCollectionAsIndexed(Akonadi::Collection(id));
        QObject::connect( job, SIGNAL( finished( KJob* ) ), tester, SLOT( removalComplete( KJob* ) ) );
    }
    
    return app.exec();
}

#include "akonadinepomukfeeder_indexer.moc"