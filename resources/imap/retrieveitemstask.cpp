/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "retrieveitemstask.h"

#include "collectionflagsattribute.h"
#include "noselectattribute.h"
#include "uidvalidityattribute.h"
#include "uidnextattribute.h"
#include "highestmodseqattribute.h"
#include "messagehelper.h"
#include "batchfetcher.h"

#include <akonadi/cachepolicy.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/kmime/messageparts.h>
#include <akonadi/agentbase.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/session.h>

#include <KDE/KDebug>
#include <KDE/KLocale>

#include <kimap/expungejob.h>
#include <kimap/fetchjob.h>
#include <kimap/selectjob.h>
#include <kimap/session.h>
#include <kimap/searchjob.h>

RetrieveItemsTask::RetrieveItemsTask(ResourceStateInterface::Ptr resource, QObject *parent)
    : ResourceTask(CancelIfNoSession, resource, parent),
    m_session(0),
    m_fetchedMissingBodies(-1),
    m_fetchMissingBodies(false),
    m_batchFetcher(0),
    m_uidBasedFetch(true),
    m_flagsChanged(false)
{

}

RetrieveItemsTask::~RetrieveItemsTask()
{
}

void RetrieveItemsTask::setFetchMissingItemBodies(bool enabled)
{
    m_fetchMissingBodies = enabled;
}

void RetrieveItemsTask::doStart(KIMAP::Session *session)
{
    emitPercent(0);
    // Prevent fetching items from noselect folders.
    if (collection().hasAttribute("noselect")) {
        NoSelectAttribute* noselect = static_cast<NoSelectAttribute*>(collection().attribute("noselect"));
        if (noselect->noSelect()) {
            kDebug(5327) << "No Select folder";
            itemsRetrievalDone();
            return;
        }
    }

    m_session = session;

    const Akonadi::Collection col = collection();
    if (m_fetchMissingBodies && col.cachePolicy()
        .localParts().contains( QLatin1String(Akonadi::MessagePart::Body))) { //disconnected mode, make sure we really have the body cached

        Akonadi::Session *session = new Akonadi::Session(resourceName().toLatin1() + "_body_checker", this);
        Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(col, session);
        fetchJob->fetchScope().setCheckForCachedPayloadPartsOnly();
        fetchJob->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Body);
        fetchJob->fetchScope().setFetchModificationTime(false);
        connect(fetchJob, SIGNAL(result(KJob*)), this, SLOT(fetchItemsWithoutBodiesDone(KJob*)));
        connect(fetchJob, SIGNAL(result(KJob*)), session, SLOT(deleteLater()));
    } else {
        startRetrievalTasks();
    }
}

BatchFetcher* RetrieveItemsTask::createBatchFetcher(MessageHelper::Ptr messageHelper,
                                                    const KIMAP::ImapSet &set,
                                                    const KIMAP::FetchJob::FetchScope &scope,
                                                    int batchSize, KIMAP::Session* session)
{
    return new BatchFetcher(messageHelper, set, scope, batchSize, session);
}

void RetrieveItemsTask::fetchItemsWithoutBodiesDone(KJob *job)
{
    QList<qint64> uids;
    if (job->error()) {
        kWarning() << job->errorString();
        cancelTask(job->errorString());
        return;
    } else {
        int i = 0;
        Akonadi::ItemFetchJob *fetch = static_cast<Akonadi::ItemFetchJob*>(job);
        Q_FOREACH(const Akonadi::Item &item, fetch->items())  {
            if (!item.cachedPayloadParts().contains(Akonadi::MessagePart::Body)) {
                kWarning() << "Item " << item.id() << " is missing the payload! Cached payloads: " << item.cachedPayloadParts();
                uids.append(item.remoteId().toInt());
                i++;
            }
        }
        if (i > 0) {
            kWarning() << "Number of items missing the body: " << i;
        }
    }
    onFetchItemsWithoutBodiesDone(uids);
}

void RetrieveItemsTask::onFetchItemsWithoutBodiesDone(const QList<qint64> &items)
{
    m_messageUidsMissingBody = items;
    startRetrievalTasks();
}


void RetrieveItemsTask::startRetrievalTasks()
{
    const QString mailBox = mailBoxForCollection(collection());
    kDebug(5327) << "Starting retrieval for " << mailBox;
    m_time.start();

    // Now is the right time to expunge the messages marked \\Deleted from this mailbox.
    // We assume that we can only expunge if we can delete items (correct would be to check for "e" ACL right).
    if (isAutomaticExpungeEnabled() && (collection().rights() & Akonadi::Collection::CanDeleteItem)) {
        if (m_session->selectedMailBox() != mailBox) {
            triggerPreExpungeSelect(mailBox);
        } else {
            triggerExpunge(mailBox);
        }
    } else {
        // Always select to get the stats updated
        triggerFinalSelect(mailBox);
    }
}

void RetrieveItemsTask::triggerPreExpungeSelect(const QString &mailBox)
{
    KIMAP::SelectJob *select = new KIMAP::SelectJob(m_session);
    select->setMailBox(mailBox);
    select->setCondstoreEnabled(serverSupportsCondstore());
    connect(select, SIGNAL(result(KJob*)),
            this, SLOT(onPreExpungeSelectDone(KJob*)));
    select->start();
}

void RetrieveItemsTask::onPreExpungeSelectDone(KJob *job)
{
    if (job->error()) {
        kWarning() << job->errorString();
        cancelTask(job->errorString());
    } else {
        KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob*>(job);
        triggerExpunge(select->mailBox());
    }
}

void RetrieveItemsTask::triggerExpunge(const QString &mailBox)
{
    KIMAP::ExpungeJob *expunge = new KIMAP::ExpungeJob(m_session);
    connect(expunge, SIGNAL(result(KJob*)),
            this, SLOT(onExpungeDone(KJob*)));
    expunge->start();
}

void RetrieveItemsTask::onExpungeDone(KJob *job)
{
    // We can ignore the error, we just had a wrong expunge so some old messages will just reappear.
    // TODO we should probably hide messages that are marked as deleted (skipping will not work because we rely on the message count)
    if (job->error()) {
        kWarning() << "Expunge failed: " << job->errorString();
    }
    // Except for network errors.
    if (job->error() && m_session->state() == KIMAP::Session::Disconnected) {
        cancelTask( job->errorString() );
        return;
    }

    // We have to re-select the mailbox to update all the stats after the expunge
    // (the EXPUNGE command doesn't return enough for our needs)
    triggerFinalSelect(m_session->selectedMailBox());
}

void RetrieveItemsTask::triggerFinalSelect(const QString &mailBox)
{
    KIMAP::SelectJob *select = new KIMAP::SelectJob(m_session);
    select->setMailBox(mailBox);
    select->setCondstoreEnabled(serverSupportsCondstore());
    connect( select, SIGNAL(result(KJob*)),
            this, SLOT(onFinalSelectDone(KJob*)) );
    select->start();
}

void RetrieveItemsTask::onFinalSelectDone(KJob *job)
{
    if (job->error()) {
        kWarning() << job->errorString();
        cancelTask(job->errorString());
        return;
    }

    KIMAP::SelectJob *select = qobject_cast<KIMAP::SelectJob*>(job);

    const QString mailBox = select->mailBox();
    const int messageCount = select->messageCount();
    const qint64 uidValidity = select->uidValidity();
    qint64 nextUid = select->nextUid();
    quint64 highestModSeq = select->highestModSequence();
    const QList<QByteArray> flags = select->permanentFlags();

    //This is known to happen with Courier IMAP. A better solution would be to issue STATUS in case UIDNEXT is not delivered on select,
    //but that would have to be implemented in KIMAP first. See Bug 338186.
    if (nextUid < 0) {
        kWarning() << "Server bug: Your IMAP Server delivered an invalid UIDNEXT value. This is a known problem with Courier IMAP.";
        nextUid = 0;
    }

    //The select job retrieves highestmodseq whenever it's available, but in case of no CONDSTORE support we ignore it
    if (!serverSupportsCondstore()) {
        highestModSeq = 0;
    }

    Akonadi::Collection col = collection();
    bool modifyNeeded = false;

    // Get the current uid validity value and store it
    int oldUidValidity = 0;
    if (!col.hasAttribute("uidvalidity")) {
        UidValidityAttribute* currentUidValidity  = new UidValidityAttribute(uidValidity);
        col.addAttribute(currentUidValidity);
        modifyNeeded = true;
    } else {
        UidValidityAttribute* currentUidValidity =
        static_cast<UidValidityAttribute*>(col.attribute("uidvalidity" ));
        oldUidValidity = currentUidValidity->uidValidity();
        if (oldUidValidity != uidValidity) {
            currentUidValidity->setUidValidity(uidValidity);
            modifyNeeded = true;
        }
    }

    // Get the current uid next value and store it
    int oldNextUid = 0;
    if (nextUid > 0) { //this can fail with faulty servers that don't deliver uidnext
        if (UidNextAttribute* currentNextUid = col.attribute<UidNextAttribute>()) {
            oldNextUid = currentNextUid->uidNext();
            if (oldNextUid != nextUid) {
                currentNextUid->setUidNext(nextUid);
                modifyNeeded = true;
            }
        } else {
            col.attribute<UidNextAttribute>(Akonadi::Collection::AddIfMissing)->setUidNext(nextUid);
            modifyNeeded = true;
        }
    }

    // Store the mailbox flags
    if (!col.hasAttribute("collectionflags")) {
        Akonadi::CollectionFlagsAttribute *flagsAttribute  = new Akonadi::CollectionFlagsAttribute(flags);
        col.addAttribute(flagsAttribute);
        modifyNeeded = true;
    } else {
        Akonadi::CollectionFlagsAttribute *flagsAttribute =
        static_cast<Akonadi::CollectionFlagsAttribute*>(col.attribute("collectionflags"));
        const QList<QByteArray> oldFlags = flagsAttribute->flags();
        if (oldFlags != flags) {
            flagsAttribute->setFlags(flags);
            modifyNeeded = true;
        }
    }

    quint64 oldHighestModSeq = 0;
    if (serverSupportsCondstore() && highestModSeq > 0) {
        if (!col.hasAttribute("highestmodseq")) {
            HighestModSeqAttribute *attr = new HighestModSeqAttribute(highestModSeq);
            col.addAttribute(attr);
            modifyNeeded = true;
        } else {
            HighestModSeqAttribute *attr = col.attribute<HighestModSeqAttribute>();
            if (attr->highestModSequence() < highestModSeq) {
                oldHighestModSeq = attr->highestModSequence();
                attr->setHighestModSeq(highestModSeq);
                modifyNeeded = true;
            } else if (attr->highestModSequence() == highestModSeq) {
                oldHighestModSeq = attr->highestModSequence();
            } else if (attr->highestModSequence() > highestModSeq) {
                // This situation should not happen. If it does, update the highestModSeq
                // attribute, but rather do a full sync
                attr->setHighestModSeq(highestModSeq);
                modifyNeeded = true;
            }
        }
    }
    m_highestModseq = oldHighestModSeq;

    if ( modifyNeeded ) {
        m_modifiedCollection = col;
    }

    KIMAP::FetchJob::FetchScope scope;
    scope.parts.clear();
    scope.mode = KIMAP::FetchJob::FetchScope::FullHeaders;

    if ( col.cachePolicy()
        .localParts().contains( QLatin1String(Akonadi::MessagePart::Body) ) ) {
        scope.mode = KIMAP::FetchJob::FetchScope::Full;
    }

    const qint64 realMessageCount = col.statistics().count();

    kDebug(5327) << "Starting message retrieval. Elapsed(ms): " << m_time.elapsed();
    kDebug(5327) << "MessageCount: " << messageCount << "Local message count: " << realMessageCount;
    kDebug(5327) << "UidNext: " << nextUid << "Local UidNext: "<< oldNextUid;
    kDebug(5327) << "HighestModSeq: " << highestModSeq << "Local HighestModSeq: "<< oldHighestModSeq;

    /*
    * A synchronization has 3 mandatory steps:
    * * If uidvalidity changed the local cache must be invalidated
    * * New messages can be fetched usin uidNext and the last known fetched uid
    * * flag changes and removals can be detected by listing all messages that weren't part of the previous step
    *
    * Everything else is optimizations.
    *
    * TODO: Note that the local message count can be larger than the remote message count although no messages
    * have been deleted remotely, if we locally have messages that were not yet uploaded.
    * We cannot differentiate that from remotely removed messages, so we have to do a full flag
    * listing in that case. This can be optimized once we support QRESYNC and therefore have a way
    * to determine wether messages have been removed.
    */

    if (messageCount == 0) {
        //Shortcut:
        //If no messages are present on the server, clear local cash and finish
        m_incremental = false;
        if (realMessageCount > 0) {
            kDebug( 5327 ) << "No messages present so we are done, deleting local messages.";
            itemsRetrieved(Akonadi::Item::List());
        } else {
            kDebug( 5327 ) << "No messages present so we are done";
        }
        taskComplete();
    } else if (oldUidValidity != uidValidity) {
        //If uidvalidity has changed our local cache is worthless and has to be refetched completely
        if (oldUidValidity != 0) {
            kDebug( 5327 ) << "UIDVALIDITY check failed (" << oldUidValidity << "|"
                            << uidValidity << ") refetching " << mailBox;
        } else {
            kDebug( 5327 ) << "Fetching complete mailbox " << mailBox;
        }

        setTotalItems(messageCount);
        retrieveItems(KIMAP::ImapSet(1, nextUid), scope, false, true);
    } else if (nextUid <= 0) {
        //This is a compatibilty codepath for Courier IMAP. It probably introduces problems, but at least it syncs.
        //Since we don't have uidnext available, we simply use the messagecount. This will miss simultaneously added/removed messages.
        kDebug() << "Running courier imap compatiblity codepath";
        if (messageCount > realMessageCount) {
            //Get new messages
            retrieveItems(KIMAP::ImapSet(realMessageCount + 1, messageCount), scope, false, false);
        } else if (messageCount == realMessageCount) {
            m_uidBasedFetch = false;
            m_incremental = true;
            setTotalItems(messageCount);
            listFlagsForImapSet(KIMAP::ImapSet(1, messageCount));
        } else {
            m_uidBasedFetch = false;
            m_incremental = false;
            setTotalItems(messageCount);
            listFlagsForImapSet(KIMAP::ImapSet(1, messageCount));
        }
    } else if (!m_messageUidsMissingBody.isEmpty()) {
        //fetch missing uids
        m_fetchedMissingBodies = 0;
        setTotalItems(m_messageUidsMissingBody.size());
        KIMAP::ImapSet imapSet;
        imapSet.add(m_messageUidsMissingBody);
        retrieveItems(imapSet, scope, true, true);
    } else if (nextUid > oldNextUid && ((realMessageCount + nextUid - oldNextUid) == messageCount) && realMessageCount > 0) {
        //Optimization:
        //New messages are available, but we know no messages have been removed.
        //Fetch new messages, and then check for changed flags and removed messages
        //We can make an incremental update and use modseq.
        kDebug( 5327 ) << "Incrementally fetching new messages: UidNext: " << nextUid << " Old UidNext: " << oldNextUid << " message count " << messageCount << realMessageCount;
        setTotalItems(qMax(1ll, messageCount - realMessageCount));
        m_flagsChanged = !(highestModSeq == oldHighestModSeq);
        retrieveItems(KIMAP::ImapSet(qMax(1, oldNextUid), nextUid), scope, true, true);
    } else if (nextUid > oldNextUid && messageCount > (realMessageCount + nextUid - oldNextUid) && realMessageCount > 0) {
        //Error recovery:
        //New messages are available, but not enough to justify the difference between the local and remote message count.
        //This can be triggered if we i.e. clear the local cache, but the keep the annotations.
        //If we didn't catch this case, we end up inserting flags only for every missing message.
        kWarning() << "Detected inconsistency in local cache, we're missing some messages. Server: " << messageCount << " Local: "<< realMessageCount;
        kWarning() << "Refetching complete mailbox.";
        setTotalItems(messageCount);
        retrieveItems(KIMAP::ImapSet(1, nextUid), scope, false, true);
    } else if (nextUid > oldNextUid) {
        //New messages are available. Fetch new messages, and then check for changed flags and removed messages
        kDebug( 5327 ) << "Fetching new messages: UidNext: " << nextUid << " Old UidNext: " << oldNextUid;
        setTotalItems(messageCount);
        retrieveItems(KIMAP::ImapSet(qMax(1, oldNextUid), nextUid), scope, false, true);
    } else if (messageCount == realMessageCount && oldNextUid == nextUid) {
        //Optimization:
        //We know no messages were added or removed (if the message count and uidnext is still the same)
        //We only check the flags incrementally and can make use of modseq
        m_uidBasedFetch = true;
        m_incremental = true;
        m_flagsChanged = !(highestModSeq == oldHighestModSeq);
        //Workaround: If the server doesn't support CONDSTORE we would end up syncing all flags during every sync.
        //Instead we only sync flags when new messages are available or removed and skip this step.
        //WARNING: This sacrifices consistency as we will not detect flag changes until a new message enters the mailbox.
        if (m_incremental && !serverSupportsCondstore()) {
            kDebug(5327) << "Avoiding flag sync due to missing CONDSTORE support";
            taskComplete();
            return;
        }
        setTotalItems(messageCount);
        listFlagsForImapSet(KIMAP::ImapSet(1, nextUid));
    } else if (messageCount > realMessageCount) {
        //Error recovery:
        //We didn't detect any new messages based on the uid, but according to the message count there are new ones.
        //Our local cache is invalid and has to be refetched.
        kWarning() << "Detected inconsistency in local cache, we're missing some messages. Server: " << messageCount << " Local: "<< realMessageCount;
        kWarning() << "Refetching complete mailbox.";
        setTotalItems(messageCount);
        retrieveItems(KIMAP::ImapSet(1, nextUid), scope, false, true);
    } else {
        //Shortcut:
        //No new messages are available. Directly check for changed flags and removed messages.
        m_uidBasedFetch = true;
        m_incremental = false;
        setTotalItems(messageCount);
        listFlagsForImapSet(KIMAP::ImapSet(1, nextUid));
    }
}

void RetrieveItemsTask::retrieveItems(const KIMAP::ImapSet& set, const KIMAP::FetchJob::FetchScope &scope, bool incremental, bool uidBased)
{
    Q_ASSERT(set.intervals().size() == 1);

    m_incremental = incremental;
    m_uidBasedFetch = uidBased;

    m_batchFetcher = createBatchFetcher(resourceState()->messageHelper(), set, scope, batchSize(), m_session);
    m_batchFetcher->setUidBased(m_uidBasedFetch);
    if (m_uidBasedFetch && set.intervals().size() == 1) {
        m_batchFetcher->setSearchUids(set.intervals().front());
    }
    m_batchFetcher->setProperty("alreadyFetched", set.intervals().first().begin());
    connect(m_batchFetcher, SIGNAL(itemsRetrieved(Akonadi::Item::List)),
            this, SLOT(onItemsRetrieved(Akonadi::Item::List)));
    connect(m_batchFetcher, SIGNAL(result(KJob*)),
            this, SLOT(onRetrievalDone(KJob*)));
    m_batchFetcher->start();
}

void RetrieveItemsTask::onReadyForNextBatch(int size)
{
    Q_UNUSED(size);
    if (m_batchFetcher) {
        m_batchFetcher->fetchNextBatch();
    }
}

void RetrieveItemsTask::onItemsRetrieved(const Akonadi::Item::List &addedItems)
{
    if (m_incremental) {
        itemsRetrievedIncremental(addedItems, Akonadi::Item::List());
    } else {
        itemsRetrieved(addedItems);
    }

    //m_fetchedMissingBodies is -1 if we fetch for other reason, but missing bodies
    if (m_fetchedMissingBodies != -1) {
        const QString mailBox = mailBoxForCollection(collection());
        m_fetchedMissingBodies += addedItems.count();
        emit status(Akonadi::AgentBase::Running,
                    i18nc( "@info:status", "Fetching missing mail bodies in %3: %1/%2", m_fetchedMissingBodies, m_messageUidsMissingBody.count(), mailBox));
    }
}

void RetrieveItemsTask::onRetrievalDone(KJob *job)
{
    m_batchFetcher = 0;
    if (job->error()) {
        kWarning() << job->errorString();
        cancelTask(job->errorString());
        m_fetchedMissingBodies = -1;
        return;
    }

    //This is the lowest sequence number that we just fetched.
    const KIMAP::ImapSet::Id alreadyFetchedBegin = job->property("alreadyFetched").value<KIMAP::ImapSet::Id>();

    // If this is the first fetch of a folder, skip getting flags, we
    // already have them all from the previous full fetch. This is not
    // just an optimization, as incremental retrieval assumes nothing
    // will be listed twice.
    if (m_fetchedMissingBodies != -1 || alreadyFetchedBegin <= 1) {
        taskComplete();
        return;
    }

    // Fetch flags of all items that were not fetched by the fetchJob. After
    // that /all/ items in the folder are synced.
    listFlagsForImapSet(KIMAP::ImapSet(1, alreadyFetchedBegin - 1));
}

void RetrieveItemsTask::listFlagsForImapSet(const KIMAP::ImapSet& set)
{
  kDebug(5327) << "Listing flags " << set.intervals().first().begin() << set.intervals().first().end();
  kDebug(5327) << "Starting flag retrieval. Elapsed(ms): " << m_time.elapsed();

  KIMAP::FetchJob::FetchScope scope;
  scope.parts.clear();
  scope.mode = KIMAP::FetchJob::FetchScope::Flags;
  // Only use changeSince when doing incremental listings,
  // otherwise we would overwrite our local data with an incomplete dataset
  if(m_incremental && serverSupportsCondstore()) {
      scope.changedSince = m_highestModseq;
      if (!m_flagsChanged) {
          kDebug(5327)  << "No flag changes.";
          taskComplete();
          return;
      }
  }

  m_batchFetcher = createBatchFetcher(resourceState()->messageHelper(), set, scope, 10 * batchSize(), m_session);
  m_batchFetcher->setUidBased(m_uidBasedFetch);
  if (m_uidBasedFetch && scope.changedSince == 0 && set.intervals().size() == 1) {
      m_batchFetcher->setSearchUids(set.intervals().front());
  }
  connect(m_batchFetcher, SIGNAL(itemsRetrieved(Akonadi::Item::List)),
          this, SLOT(onItemsRetrieved(Akonadi::Item::List)));
  connect(m_batchFetcher, SIGNAL(result(KJob*)),
          this, SLOT(onFlagsFetchDone(KJob*)));
  m_batchFetcher->start();
}

void RetrieveItemsTask::onFlagsFetchDone(KJob *job)
{
    m_batchFetcher = 0;
    if ( job->error() ) {
        kWarning() << job->errorString();
        cancelTask(job->errorString());
    } else {
        taskComplete();
    }
}

void RetrieveItemsTask::taskComplete()
{
    if (m_modifiedCollection.isValid()) {
        kDebug(5327) << "Applying collection changes";
        applyCollectionChanges(m_modifiedCollection);
    }
    if (m_incremental) {
        // Calling itemsRetrievalDone() before previous call to itemsRetrievedIncremental()
        // behaves like if we called itemsRetrieved(Items::List()), so make sure
        // Akonadi knows we did incremental fetch that came up with no changes
        itemsRetrievedIncremental(Akonadi::Item::List(), Akonadi::Item::List());
    }
    kDebug(5327) << "Retrieval complete. Elapsed(ms): " << m_time.elapsed();
    itemsRetrievalDone();
}

#include "retrieveitemstask.moc"
