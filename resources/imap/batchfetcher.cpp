/*
 * Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "batchfetcher.h"

#include <KIMAP/Session>

BatchFetcher::BatchFetcher(MessageHelper::Ptr messageHelper,
                           const KIMAP::ImapSet &set,
                           const KIMAP::FetchJob::FetchScope &scope,
                           int batchSize,
                           KIMAP::Session *session)
    : KJob(session),
    m_currentSet(set),
    m_scope(scope),
    m_session(session),
    m_batchSize(batchSize),
    m_uidBased(false),
    m_fetchedItemsInCurrentBatch(0),
    m_messageHelper(messageHelper),
    m_fetchInProgress(false),
    m_continuationRequested(false),
    m_gmailEnabled(false),
    m_searchInChunks(false)
{
}

BatchFetcher::~BatchFetcher()
{
}

void BatchFetcher::setUidBased(bool uidBased)
{
    m_uidBased = uidBased;
}

void BatchFetcher::setSearchUids(const KIMAP::ImapInterval &intervall)
{
    m_searchUidInterval = intervall;

    //We look up the UIDs ourselves
    m_currentSet = KIMAP::ImapSet();

    //MS Exchange can't handle big results so we have to split the search into small chunks
    m_searchInChunks = m_session->serverGreeting().contains("Microsoft Exchange");
}

void BatchFetcher::setGmailExtensionsEnabled(bool enable)
{
    m_gmailEnabled = enable;
}

static const int maxAmountOfUidToSearchInOneTime = 2000;

void BatchFetcher::start()
{
    if (m_searchUidInterval.size()) {
        //Search in chunks also Exchange can handle
        const KIMAP::ImapInterval::Id firstUidToSearch = m_searchUidInterval.begin();
        const KIMAP::ImapInterval::Id lastUidToSearch  = m_searchInChunks
            ? qMin(firstUidToSearch + maxAmountOfUidToSearchInOneTime - 1, m_searchUidInterval.end())
            : m_searchUidInterval.end();

        //Prepare next chunk
        const KIMAP::ImapInterval::Id intervalBegin = lastUidToSearch + 1;
        //Or are we already done?
        if (intervalBegin > m_searchUidInterval.end()) {
            m_searchUidInterval = KIMAP::ImapInterval();
        } else {
            m_searchUidInterval.setBegin(intervalBegin);
        }

        //Resolve the uid to sequence numbers
        KIMAP::SearchJob *search = new KIMAP::SearchJob(m_session);
        search->setUidBased(true);
        search->setTerm(KIMAP::Term(KIMAP::Term::Uid, KIMAP::ImapSet(firstUidToSearch, lastUidToSearch)));
        connect(search, SIGNAL(result(KJob*)), this, SLOT(onUidSearchDone(KJob*)));
        search->start();
    } else {
        fetchNextBatch();
    }
}

void BatchFetcher::onUidSearchDone(KJob* job)
{
    if (job->error()) {
        kWarning() << "Search job failed: " << job->errorString();
        setError(KJob::UserDefinedError);
        emitResult();
        return;
    }

    KIMAP::SearchJob *search = static_cast<KIMAP::SearchJob*>(job);
    m_uidBased = search->isUidBased();
    m_currentSet.add(search->results());

    //More to search?
    start();
}

void BatchFetcher::fetchNextBatch()
{
    if (m_fetchInProgress) {
        m_continuationRequested = true;
        return;
    }
    m_continuationRequested = false;
    Q_ASSERT(m_batchSize > 0);
    if (m_currentSet.isEmpty()) {
        kDebug(5327) << "fetch complete";
        emitResult();
        return;
    }

    KIMAP::FetchJob *fetch = new KIMAP::FetchJob(m_session);
    if (m_scope.changedSince != 0) {
        kDebug(5327) << "Fetching all messages in one batch.";
        fetch->setSequenceSet(m_currentSet);
        m_currentSet = KIMAP::ImapSet();
    } else {
        KIMAP::ImapSet toFetch;
        qint64 counter = 0;
        KIMAP::ImapSet newSet;

        //Take a chunk from the set
        Q_FOREACH (const KIMAP::ImapInterval &interval, m_currentSet.intervals()) {
            if (!interval.hasDefinedEnd()) {
                //If we get an interval without a defined end we simply fetch everything
                kDebug(5327) << "Received interval without defined end, fetching everything in one batch";
                toFetch.add(interval);
                newSet = KIMAP::ImapSet();
                break;
            }
            const qint64 wantedItems = m_batchSize - counter;
            if (counter < m_batchSize) {
                if (interval.size() <= wantedItems) {
                    counter += interval.size();
                    toFetch.add(interval);
                } else {
                    counter += wantedItems;
                    toFetch.add(KIMAP::ImapInterval(interval.begin(), interval.begin() + wantedItems - 1));
                    newSet.add(KIMAP::ImapInterval(interval.begin() + wantedItems, interval.end()));
                }
            } else {
                newSet.add(interval);
            }
        }
        kDebug(5327) << "Fetching " << toFetch.intervals().size() << " intervals";
        fetch->setSequenceSet(toFetch);
        m_currentSet = newSet;
    }

    fetch->setUidBased(m_uidBased);
    fetch->setScope(m_scope);
    fetch->setGmailExtensionsEnabled(m_gmailEnabled);
    connect(fetch, SIGNAL(headersReceived(QString,
                                          QMap<qint64,qint64>,
                                          QMap<qint64,qint64>,
                                          QMap<qint64,KIMAP::MessageAttribute>,
                                          QMap<qint64,KIMAP::MessageFlags>,
                                          QMap<qint64,KIMAP::MessagePtr>)),
            this, SLOT(onHeadersReceived(QString,
                                         QMap<qint64,qint64>,
                                         QMap<qint64,qint64>,
                                         QMap<qint64,KIMAP::MessageAttribute>,
                                         QMap<qint64,KIMAP::MessageFlags>,
                                         QMap<qint64,KIMAP::MessagePtr>)) );
    connect(fetch, SIGNAL(result(KJob*)),
            this, SLOT(onHeadersFetchDone(KJob*)));
    m_fetchInProgress = true;
    fetch->start();
}

void BatchFetcher::onHeadersReceived(const QString &mailBox,
                                     const QMap<qint64, qint64> &uids,
                                     const QMap<qint64, qint64> &sizes,
                                     const QMap<qint64, KIMAP::MessageAttribute> &attrs,
                                     const QMap<qint64, KIMAP::MessageFlags> &flags,
                                     const QMap<qint64, KIMAP::MessagePtr> &messages)
{
    KIMAP::FetchJob *fetch = static_cast<KIMAP::FetchJob*>( sender() );
    Q_ASSERT( fetch );

    Akonadi::Item::List addedItems;
    foreach (qint64 number, uids.keys()) { //krazy:exclude=foreach
        //kDebug( 5327 ) << "Flags: " << i.flags();
        bool ok;
        const Akonadi::Item item = m_messageHelper->createItemFromMessage(messages[number], uids[number], sizes[number], attrs.values(number), flags[number], fetch->scope(), ok);
        if (ok) {
            m_fetchedItemsInCurrentBatch++;
            addedItems << item;
        }
    }
//     kDebug() << addedItems.size();
    if (!addedItems.isEmpty()) {
        emit itemsRetrieved(addedItems);
    }
}

void BatchFetcher::onHeadersFetchDone( KJob *job )
{
    m_fetchInProgress = false;
    if (job->error()) {
        kWarning() << "Fetch job failed " << job->errorString();
        setError(KJob::UserDefinedError);
        emitResult();
        return;
    }
    if (m_currentSet.isEmpty()) {
        emitResult();
        return;
    }
    //Fetch more if we didn't deliver enough yet.
    //This can happen because no message is in the fetched uid range, or if the translation failed
    if (m_fetchedItemsInCurrentBatch < m_batchSize) {
        fetchNextBatch();
    } else {
        m_fetchedItemsInCurrentBatch = 0;
        //Also fetch more if we already got a continuation request during the fetch.
        //This can happen if we deliver too many items during a previous batch (after using )
        //Note that m_fetchedItemsInCurrentBatch will be off by the items that we delivered already.
        if (m_continuationRequested) {
            fetchNextBatch();
        }
    }
}

