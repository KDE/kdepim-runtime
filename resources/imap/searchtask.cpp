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
#include <Akonadi/SearchQuery>
#include <Akonadi/KMime/MessageFlags>
#include <KDateTime>

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

    Akonadi::SearchQuery query = Akonadi::SearchQuery::fromJSON( m_query.toLatin1() );
    KIMAP::SearchJob *searchJob = new KIMAP::SearchJob( session );
    searchJob->setUidBased( true );

    Akonadi::SearchTerm term = query.term();
    if ( term.relation() == Akonadi::SearchTerm::RelAnd ) {
        searchJob->setSearchLogic( KIMAP::SearchJob::And );
    } else {
        searchJob->setSearchLogic( KIMAP::SearchJob::Or );
    }

    Q_FOREACH (const Akonadi::SearchTerm &termsGroup, term.subTerms()) {
        Q_FOREACH (const Akonadi::SearchTerm &subterm, termsGroup.subTerms()) {
            kDebug() << subterm.key() << subterm.value();
            const Akonadi::EmailSearchTerm::EmailSearchField field = Akonadi::EmailSearchTerm::fromKey(subterm.key());
            switch (field) {
                case Akonadi::EmailSearchTerm::Message:
                case Akonadi::EmailSearchTerm::Body:
                    //FIXME
                    //todo somehow search the body (not possible yet)
                    searchJob->addSearchCriteria( KIMAP::SearchJob::Body, subterm.value().toByteArray() );
                    break;
                case Akonadi::EmailSearchTerm::Headers:
                    //FIXME
                    //search all headers
                    searchJob->addSearchCriteria( KIMAP::SearchJob::Header, subterm.value().toByteArray() );
                    break;
                case Akonadi::EmailSearchTerm::ByteSize:
                    searchJob->addSearchCriteria( KIMAP::SearchJob::Larger, subterm.value().toInt( ) );
                    searchJob->addSearchCriteria( KIMAP::SearchJob::Smaller, subterm.value().toInt( ) );
                    break;
                case Akonadi::EmailSearchTerm::HeaderDate: {
                    const KDateTime dt = KDateTime::fromString(subterm.value().toString(), KDateTime::ISODate);
                    searchJob->addSearchCriteria( KIMAP::SearchJob::On, dt.date() );
                }
                    break;
                case Akonadi::EmailSearchTerm::Subject:
                    searchJob->addSearchCriteria( KIMAP::SearchJob::Subject, subterm.value().toByteArray() );
                    break;
                case Akonadi::EmailSearchTerm::HeaderFrom:
                    searchJob->addSearchCriteria( KIMAP::SearchJob::From, subterm.value().toByteArray() );
                    break;
                case Akonadi::EmailSearchTerm::HeaderTo:
                    searchJob->addSearchCriteria( KIMAP::SearchJob::To, subterm.value().toByteArray() );
                    break;
                case Akonadi::EmailSearchTerm::HeaderCC:
                    searchJob->addSearchCriteria( KIMAP::SearchJob::CC, subterm.value().toByteArray() );
                    break;
                case Akonadi::EmailSearchTerm::HeaderBCC:
                    searchJob->addSearchCriteria( KIMAP::SearchJob::BCC, subterm.value().toByteArray() );
                    break;
                case Akonadi::EmailSearchTerm::MessageStatus:
                    if ( subterm.value().toString() == QString::fromLatin1( Akonadi::MessageFlags::Flagged ) ) {
                        searchJob->addSearchCriteria( KIMAP::SearchJob::Flagged );
                    }
                    //TODO remaining flags
                    break;
                case Akonadi::EmailSearchTerm::MessageTag:
                    //search directly in akonadi? or index tags.
                    break;
                case Akonadi::EmailSearchTerm::HeaderReplyTo:
                    // TODO
                    //t.addSubTerm(getTerm(subterm, "replyto"));
                    break;
                case Akonadi::EmailSearchTerm::HeaderOrganization:
                    // TODO
                    //t.addSubTerm(getTerm(subterm, "organization"));
                    break;
                case Akonadi::EmailSearchTerm::HeaderListId:
    //                     t.addSubTerm(getTerm(subterm, "listid"));
                    break;
                case Akonadi::EmailSearchTerm::HeaderResentFrom:
    //                     t.addSubTerm(getTerm(subterm, "resentfrom"));
                    break;
                case Akonadi::EmailSearchTerm::HeaderXLoop:
                    break;
                case Akonadi::EmailSearchTerm::HeaderXMailingList:
                    break;
                case Akonadi::EmailSearchTerm::HeaderXSpamFlag:
                    break;
                case Akonadi::EmailSearchTerm::Unknown:
                default:
                    kWarning() << "unknown term " << subterm.key();
            }
        }
    }

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
