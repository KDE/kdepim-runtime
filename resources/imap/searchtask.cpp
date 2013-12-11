/*
 * Copyright 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "searchtask.h"
#include <kimap/searchjob.h>
#include <kimap/session.h>
#include <kimap/selectjob.h>

Q_DECLARE_METATYPE( KIMAP::Session* )

SearchTask::SearchTask( ResourceStateInterface::Ptr state,  const QString &query, QObject *parent)
 : ResourceTask( ResourceTask::DeferIfNoSession, state, parent)
 , m_query( query )
{
}

SearchTask::~SearchTask()
{
}

void SearchTask::doStart( KIMAP::Session *session )
{
    kDebug() << collection().remoteId();

    const QString mailbox = mailBoxForCollection( collection() );
    if ( session->selectedMailBox() == mailbox ) {
        doSearch( session );
        return;
    }

    KIMAP::SelectJob *select = new KIMAP::SelectJob( session );
    select->setMailBox( mailbox );
    connect( select, SIGNAL(finished(KJob*)),
             this, SLOT(onSelectDone(KJob*)) );
    select->start();
}

void SearchTask::onSelectDone( KJob *job )
{
    if ( job->error() ) {
        searchFinished( QVector<qint64>() );
        cancelTask( job->errorText() );
        return;
    }

    doSearch( qobject_cast<KIMAP::SelectJob*>( job )->session() );
}

void SearchTask::doSearch( KIMAP::Session *session )
{
    kDebug();

    KIMAP::SearchJob *searchJob = new KIMAP::SearchJob( session );
    searchJob->setUidBased( true );
    searchJob->addSearchCriteria( KIMAP::SearchJob::To, "dvratil@redhat.com" );
    connect( searchJob, SIGNAL(finished(KJob*)),
             this, SLOT(onSearchDone(KJob*)) );
    searchJob->start();
}

void SearchTask::onSearchDone( KJob* job )
{
    if ( job->error() ) {
        searchFinished( QVector<qint64>() );
        cancelTask( job->errorString() );
        return;
    }

    KIMAP::SearchJob *searchJob = qobject_cast<KIMAP::SearchJob*>( job );
    const QList<qint64> result = searchJob->results();
    kDebug() << result.count() << "matches";

    searchFinished( result.toVector() );
    taskDone();
}
