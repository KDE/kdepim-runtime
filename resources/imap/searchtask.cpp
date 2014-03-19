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

static KIMAP::Term::Relation mapRelation(Akonadi::SearchTerm::Relation relation) {
    if (relation == Akonadi::SearchTerm::RelAnd){
        return KIMAP::Term::And;
    }
    return KIMAP::Term::Or;
}

static KIMAP::Term recursiveEmailTermMapping(const Akonadi::SearchTerm &term)
{
    if (!term.subTerms().isEmpty()) {
        QVector<KIMAP::Term> subterms;
        Q_FOREACH (const Akonadi::SearchTerm &subterm, term.subTerms()) {
            const KIMAP::Term newTerm = recursiveEmailTermMapping(subterm);
            if (!newTerm.isNull()) {
                subterms << newTerm;
            }
        }
        return KIMAP::Term(mapRelation(term.relation()), subterms);
    } else {
        const Akonadi::EmailSearchTerm::EmailSearchField field = Akonadi::EmailSearchTerm::fromKey(term.key());
        switch (field) {
            case Akonadi::EmailSearchTerm::Message:
                return KIMAP::Term(KIMAP::Term::Text, term.value().toString()).setNegated(term.isNegated());
            case Akonadi::EmailSearchTerm::Body:
                return KIMAP::Term(KIMAP::Term::Body, term.value().toString()).setNegated(term.isNegated());
            case Akonadi::EmailSearchTerm::Headers:
                //FIXME
//                 return KIMAP::Term(KIMAP::Term::Header, term.value()).setNegated(term.isNegated());
                break;
            case Akonadi::EmailSearchTerm::ByteSize: {
                int value = term.value().toInt();
                switch (term.condition()) {
                    case Akonadi::SearchTerm::CondGreaterOrEqual:
                        value--;
                    case Akonadi::SearchTerm::CondGreaterThan:
                        return KIMAP::Term(KIMAP::Term::Larger, value).setNegated(term.isNegated());
                    case Akonadi::SearchTerm::CondLessOrEqual:
                        value++;
                    case Akonadi::SearchTerm::CondLessThan:
                        return KIMAP::Term(KIMAP::Term::Smaller, value).setNegated(term.isNegated());
                    case Akonadi::SearchTerm::CondEqual:
                        return KIMAP::Term(KIMAP::Term::And, QVector<KIMAP::Term>() << KIMAP::Term(KIMAP::Term::Smaller, value + 1) << KIMAP::Term(KIMAP::Term::Larger, value + 1)).setNegated(term.isNegated());
                    case Akonadi::SearchTerm::CondContains:
                        kDebug()<<" invalid condition for ByteSize";
                        break;
                }
            }
                break;
            case Akonadi::EmailSearchTerm::HeaderOnlyDate:
            case Akonadi::EmailSearchTerm::HeaderDate: {
                QDate value = term.value().toDateTime().date();
                switch (term.condition()) {
                    case Akonadi::SearchTerm::CondGreaterOrEqual:
                        value = value.addDays(-1);
                    case Akonadi::SearchTerm::CondGreaterThan:
                        return KIMAP::Term(KIMAP::Term::SentSince, value).setNegated(term.isNegated());
                    case Akonadi::SearchTerm::CondLessOrEqual:
                        value = value.addDays(1);
                    case Akonadi::SearchTerm::CondLessThan:
                        return KIMAP::Term(KIMAP::Term::SentBefore, value).setNegated(term.isNegated());
                    case Akonadi::SearchTerm::CondEqual:
                        return KIMAP::Term(KIMAP::Term::SentOn, value).setNegated(term.isNegated());
                    case Akonadi::SearchTerm::CondContains:
                        kDebug()<<" invalid condition for Date";
                        break;
                }
            }
            case Akonadi::EmailSearchTerm::Subject:
                return KIMAP::Term(KIMAP::Term::Subject, term.value().toString()).setNegated(term.isNegated());
            case Akonadi::EmailSearchTerm::HeaderFrom:
                return KIMAP::Term(KIMAP::Term::From, term.value().toString()).setNegated(term.isNegated());
            case Akonadi::EmailSearchTerm::HeaderTo:
                return KIMAP::Term(KIMAP::Term::To, term.value().toString()).setNegated(term.isNegated());
            case Akonadi::EmailSearchTerm::HeaderCC:
                return KIMAP::Term(KIMAP::Term::Cc, term.value().toString()).setNegated(term.isNegated());
            case Akonadi::EmailSearchTerm::HeaderBCC:
                return KIMAP::Term(KIMAP::Term::Bcc, term.value().toString()).setNegated(term.isNegated());
            case Akonadi::EmailSearchTerm::MessageStatus:
                if (term.value().toString() == QString::fromLatin1(Akonadi::MessageFlags::Flagged)) {
                    return KIMAP::Term(KIMAP::Term::Flagged).setNegated(term.isNegated());
                }
                if (term.value().toString() == QString::fromLatin1(Akonadi::MessageFlags::Deleted)) {
                    return KIMAP::Term(KIMAP::Term::Deleted).setNegated(term.isNegated());
                }
                if (term.value().toString() == QString::fromLatin1(Akonadi::MessageFlags::Replied)) {
                    return KIMAP::Term(KIMAP::Term::Answered).setNegated(term.isNegated());
                }
                if (term.value().toString() == QString::fromLatin1(Akonadi::MessageFlags::Seen)) {
                    return KIMAP::Term(KIMAP::Term::Seen).setNegated(term.isNegated());
                }
                break;
            case Akonadi::EmailSearchTerm::MessageTag:
                break;
            case Akonadi::EmailSearchTerm::HeaderReplyTo:
                break;
            case Akonadi::EmailSearchTerm::HeaderOrganization:
                break;
            case Akonadi::EmailSearchTerm::HeaderListId:
                break;
            case Akonadi::EmailSearchTerm::HeaderResentFrom:
                break;
            case Akonadi::EmailSearchTerm::HeaderXLoop:
                break;
            case Akonadi::EmailSearchTerm::HeaderXMailingList:
                break;
            case Akonadi::EmailSearchTerm::HeaderXSpamFlag:
                break;
            case Akonadi::EmailSearchTerm::Unknown:
            default:
                kWarning() << "unknown term " << term.key();
        }
    }
    return KIMAP::Term();
}

void SearchTask::doSearch( KIMAP::Session *session )
{
    kDebug() << m_query;

    Akonadi::SearchQuery query = Akonadi::SearchQuery::fromJSON( m_query.toLatin1() );
    KIMAP::SearchJob *searchJob = new KIMAP::SearchJob( session );
    searchJob->setUidBased( true );

    KIMAP::Term term = recursiveEmailTermMapping(query.term());
    if (term.isNull()) {
        kWarning() << "failed to translate query " << m_query;
        searchFinished( QVector<qint64>() );
        cancelTask( "Invalid search" );
        return;
    }
    searchJob->setTerm(term);

    connect( searchJob, SIGNAL(finished(KJob*)),
             this, SLOT(onSearchDone(KJob*)) );
    searchJob->start();
}

void SearchTask::onSearchDone( KJob* job )
{
    if ( job->error() ) {
        kWarning() << "Failed to execute search " << job->errorString();
        kDebug() << m_query;
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
