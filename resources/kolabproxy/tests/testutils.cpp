/*
    Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

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
#include "testutils.h"

#include <QTest>
#include <Akonadi/Item>
#include <Akonadi/AgentInstanceCreateJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <KMime/Message>
#include <collectionannotationsattribute.h>
#include <kolabdefinitions.h> //libkolab
#include <kolabobject.h>

#include "../kolabdefs.h"

using namespace Akonadi;

Akonadi::Item TestUtils::createImapItem(const KCalCore::Event::Ptr &event) {
    const KMime::Message::Ptr &message = Kolab::KolabObjectWriter::writeEvent(event, Kolab::KolabV3, "Proxytest", QLatin1String("UTC") );
    Q_ASSERT(message);
    Akonadi::Item imapItem1;
    imapItem1.setMimeType( QLatin1String("message/rfc822") );
    imapItem1.setPayload( message );
    return imapItem1;
}

TestUtils::MonitorPair TestUtils::monitor(Akonadi::Collection col, const char *signal) {
    QSharedPointer<Akonadi::Monitor> monitor(new Akonadi::Monitor);
    monitor->setCollectionMonitored(col);
    QSharedPointer<QSignalSpy> spy(new QSignalSpy(monitor.data(), signal));
    Q_ASSERT_X(spy->isValid(), "", signal);
    return qMakePair<QSharedPointer<QSignalSpy>, QSharedPointer<Akonadi::Monitor> >(spy, monitor);
}

bool TestUtils::wait(const MonitorPair &pair) {
    for (int i = 0; i < TIMEOUT/10 ; i++) {
        if (pair.first->count() >= 1) {
            kDebug() << pair.first->first();
            return true;
        }
        QTest::qWait(10);
    }
    return false;
}

bool TestUtils::ensure(Akonadi::Collection col, const char *signal, Akonadi::Job *job) {
    MonitorPair m = monitor(col, signal);
    if (!job->exec()) {
        return false;
    }
    return wait(m);
}

bool TestUtils::ensurePopulated(QString agentinstance, int count) {
    for (int i = 0; i < TIMEOUT/10 ; i++) {
        Akonadi::CollectionFetchJob *fetchJob = new Akonadi::CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive);
        fetchJob->fetchScope().setResource(agentinstance);
        if (!fetchJob->exec()) {
            return false;
        }
        if (fetchJob->collections().size() >= count) {
            return true;
        }
        QTest::qWait(10);
    }
    return false;
}

Akonadi::Collection TestUtils::findCollection(QString agentinstance, QString name) {
    for (int i = 0; i < 500 ; i++) {
        Akonadi::CollectionFetchJob *fetchJob = new Akonadi::CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive);
        fetchJob->fetchScope().setResource(agentinstance);
        if (!fetchJob->exec()) {
            return Akonadi::Collection();
        }
        foreach (const Collection &col, fetchJob->collections()) {
            if (col.name().contains(name)) {
                return col;
            }
        }
        QTest::qWait(10);
    }
    return Akonadi::Collection();
}
