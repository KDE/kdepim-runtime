/*
    SPDX-FileCopyrightText: 2020  Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "retrieveitemstask_qresync.h"
#include "imapresource_debug.h"
#include "uidvalidityattribute.h"
#include "highestmodseqattribute.h"
#include "uidnextattribute.h"
#include "noselectattribute.h"
#include "collectionflagsattribute.h"
#include "messagehelper.h"
#include "batchfetcher.h"

#include <Akonadi/KMime/MessageParts>
#include <AkonadiCore/CachePolicy>

#include <KIMAP/Session>
#include <KIMAP/SelectJob>
#include <KIMAP/ExpungeJob>
#include <KIMAP/FetchJob>
#include <KIMAP/ImapSet>
#include <KIMAP/CloseJob>

#include <KMime/Message>

#include <KLocalizedString>

namespace {

qint64 getUidValidity(const Akonadi::Collection &col)
{
    if (col.hasAttribute<UidValidityAttribute>()) {
        return col.attribute<UidValidityAttribute>()->uidValidity();
    }

    return -1;
}

quint64 getHighestModSeq(const Akonadi::Collection &col)
{
    if (col.hasAttribute<HighestModSeqAttribute>()) {
        return col.attribute<HighestModSeqAttribute>()->highestModSequence();
    }

    return 0;
}

int getUidNext(const Akonadi::Collection &col)
{
    if (col.hasAttribute<UidNextAttribute>()) {
        return col.attribute<UidNextAttribute>()->uidNext();
    }

    return -1;
}

QList<QByteArray> getFlags(const Akonadi::Collection &col)
{
    if (col.hasAttribute<Akonadi::CollectionFlagsAttribute>()) {
        return col.attribute<Akonadi::CollectionFlagsAttribute>()->flags();
    }

    return {};
}

bool isNoSelect(const Akonadi::Collection &col)
{
    return col.hasAttribute<NoSelectAttribute>()
            && col.attribute<NoSelectAttribute>()->noSelect();
}

bool shouldFetchFullPayload(const Akonadi::Collection &col)
{
    return col.cachePolicy().localParts().contains(QLatin1String(Akonadi::MessagePart::Body));
}

template<typename T, typename Getter, typename Setter, typename Val>
bool updateAttr(Akonadi::Collection &col, Getter getter, Setter setter, const Val &val)
{
    if (col.hasAttribute<T>()) {
        if ((col.attribute<T>()->*getter)() == val) {
            return false;
        }
    }

    auto *attr = col.attribute<T>(Akonadi::Collection::AddIfMissing);
    (attr->*setter)(val);
    return true;
}

} // namespace





RetrieveItemsTaskQResync::RetrieveItemsTaskQResync(const ResourceStateInterface::Ptr &resource, QObject *parent)
    : ResourceTask(CancelIfNoSession, resource, parent)
{
    setItemMergingMode(Akonadi::ItemSync::RIDMerge);
}

RetrieveItemsTaskQResync::~RetrieveItemsTaskQResync() = default;

bool RetrieveItemsTaskQResync::shouldExpunge(const Akonadi::Collection &col, bool readOnly) const
{
    const bool hasACL = serverCapabilities().contains(QLatin1String("ACL"));
    const auto rights = myRights(col);
    return !readOnly
            && isAutomaticExpungeEnabled()
            && (!hasACL || (rights &KIMAP::Acl::Expunge) || (rights & KIMAP::Acl::Delete));
}

void RetrieveItemsTaskQResync::doStart(KIMAP::Session *session)
{
    m_stats.timer.start();

    emitPercent(0);

    // The flow is as follows
    // CLOSE (optionally)
    //  - closes the mailbox (if it needed)
    // SELECT mailbox (QRESYNC)
    //  - reports vanished emails
    //  - reports changed emails
    // UID EXPUNGE
    //  - reports vanished emails
    // FETCH (lastNextUid:*)
    //  - fetches all new messages

    const auto col = collection();
    m_localState = MailBoxState {
        getUidValidity(col),
        getUidNext(col),
        getHighestModSeq(col),
        getFlags(col),
        -1, // messageCount (reported only by server)
        -1, // recentCount (reported only by server)
        -1  // firstUnseenIndex (reported only by server)
    };

    const auto mailbox = mailBoxForCollection(collection());
    qCInfo(IMAPRESOURCE_LOG) << "Starting sync of mailbox" << mailbox << "(col" << col.id() << ")";
    qCDebug(IMAPRESOURCE_LOG) << "Local cache state:";
    qCDebug(IMAPRESOURCE_LOG) << " UidValidity=" << m_localState.uidValidity;
    qCDebug(IMAPRESOURCE_LOG) << " UidNext=" << m_localState.nextUid;
    qCDebug(IMAPRESOURCE_LOG) << " HighestModSeq=" << m_localState.highestModSeq;
    qCDebug(IMAPRESOURCE_LOG) << " Flags=" << m_localState.flags;

    // Prevent fetching items from noselect folders.
    if (isNoSelect(collection())) {
        qCDebug(IMAPRESOURCE_LOG) << "Mailbox" << mailbox << "(col" << col.id() << ") is no-select, not synchronizing.";
        finishSync();
        return;
    }

    // If the mailbox is already opened we need to re-open it in order to get all the
    // metadata.
    if (session->selectedMailBox() == mailbox) {
        qCDebug(IMAPRESOURCE_LOG) << "Mailbox" << mailbox << "already selected, re-opening it.";

        auto *close = new KIMAP::CloseJob(session);
        connect(close, &KJob::result, this, [this, session, mailbox](KJob *job) {
            if (job->error()) {
                qCWarning(IMAPRESOURCE_LOG) << "Failed to close current mailbox" << mailbox << ":" << job->errorString();
                cancelTask(job->errorString());
                return;
            }

            selectMailbox(session);
        });
        close->start();
    } else {
        selectMailbox(session);
    }
}

void RetrieveItemsTaskQResync::selectMailbox(KIMAP::Session *session)
{
    auto *select = new KIMAP::SelectJob(session);
    select->setMailBox(mailBoxForCollection(collection()));
    select->setCondstoreEnabled(true);
    if (m_localState.uidValidity > -1 && m_localState.highestModSeq > 0) {
        select->setQResync(m_localState.uidValidity, m_localState.highestModSeq);
    }
    connect(select, &KIMAP::SelectJob::vanished, this, &RetrieveItemsTaskQResync::removeLocalMessages);
    connect(select, &KIMAP::SelectJob::modified, this, &RetrieveItemsTaskQResync::updateLocalMessages);
    connect(select, &KJob::result, this, [this, select](KJob * /*job*/) {
        if (select->error()) {
            qCWarning(IMAPRESOURCE_LOG) << "Failed to select mailbox" << mailBoxForCollection(collection()) << ":" << select->errorString();
            cancelTask(select->errorString());
            return;
        }

        m_serverState = MailBoxState{
            select->uidValidity(),
            select->nextUid(),
            select->highestModSequence(),
            select->flags(),
            select->messageCount(),
            select->recentCount(),
            select->firstUnseenIndex()
        };
        qCDebug(IMAPRESOURCE_LOG) << "Server state reported by SELECT command:";
        qCDebug(IMAPRESOURCE_LOG) << "  UidValidity=" << m_serverState.uidValidity;
        qCDebug(IMAPRESOURCE_LOG) << "  NextUid=" << m_serverState.nextUid;
        qCDebug(IMAPRESOURCE_LOG) << "  HighestModSeq=" << m_serverState.highestModSeq;
        qCDebug(IMAPRESOURCE_LOG) << "  Flags=" << m_serverState.flags;
        qCDebug(IMAPRESOURCE_LOG) << "  MessageCount=" << m_serverState.messageCount;
        qCDebug(IMAPRESOURCE_LOG) << "  RecentCount=" << m_serverState.recentCount;
        qCDebug(IMAPRESOURCE_LOG) << "  FirstUnseenIndex=" << m_serverState.firstUnseen;

        if (m_serverState.nextUid < 0) {
            qCInfo(IMAPRESOURCE_LOG) << "Server did not report UIDNEXT, server is broken.";
            cancelTask(i18n("Server has not reported UIDNEXT."));
            return;
        } else if (m_serverState.uidValidity != m_localState.uidValidity || m_localState.nextUid <= 0) {
            // Check UIDVALIDITY matches. If not, we must do a full re-sync
            qCInfo(IMAPRESOURCE_LOG) << "UidValidity mismatch for mailbox" << mailBoxForCollection(collection()) << ", forcing full resync.";
            m_syncMode = SyncMode::Full;
            m_localState.highestModSeq = 0;
            m_localState.nextUid = 1; // We want to re-fetch all messages
            setTotalItems(m_serverState.messageCount);
        } else if (m_serverState.highestModSeq == m_localState.highestModSeq) {
            // Optimization: there's nothing to sync!
            qCInfo(IMAPRESOURCE_LOG) << "Mailbox" << mailBoxForCollection(collection()) << "already up to date";
            finishSync();
            return;
        }

        // At this point, we have synchronized:
        // * all messages removed from the mailbox on the server
        // * all flags changes

        // Next, perform expunge (if applicable) to also get rid of messages
        // that are marked as \Deleted. We don't want to sync those.
        if (shouldExpunge(collection(), select->isOpenReadOnly())) {
            expungeMessages(select->session());
        } else {
            fetchNewMessages(select->session());
        }
    });
    select->start();
}

void RetrieveItemsTaskQResync::removeLocalMessages(const KIMAP::ImapSet &messages)
{
    Q_ASSERT(m_syncMode == SyncMode::Incremental);

    Akonadi::Item::List removedItems;

    const auto intervals = messages.intervals();
    for (const auto &interval : intervals) {
        for (auto uid = interval.begin(); uid <= interval.end(); ++uid) {
            Akonadi::Item item;
            item.setParentCollection(collection());
            item.setRemoteId(QString::number(uid));
            removedItems.push_back(std::move(item));
        }
    }

    m_stats.removed += removedItems.size();
    if (!removedItems.empty()) {
        itemsRetrievedIncremental({}, removedItems);
    }
}

void RetrieveItemsTaskQResync::updateLocalMessages(const QMap<qint64, KIMAP::Message> &messages)
{
    Q_ASSERT(m_syncMode == SyncMode::Incremental);

    Akonadi::Item::List receivedItems;
    receivedItems.reserve(messages.size());

    for (auto message = messages.cbegin(), end = messages.cend(); message != end; ++message) {
        const KIMAP::Message m = message.value();
        Akonadi::Item item;
        item.setMimeType(KMime::Message::mimeType());
        item.setRemoteId(QString::number(m.uid));
        item.setFlags(toAkonadiFlags(m.flags));

        receivedItems.push_back(item);
    }

    m_stats.updated += receivedItems.size();

    Q_ASSERT(!receivedItems.empty());
    itemsRetrievedIncremental(receivedItems, {});
}

void RetrieveItemsTaskQResync::expungeMessages(KIMAP::Session *session)
{
    qCDebug(IMAPRESOURCE_LOG) << "Expunging mailbox" << mailBoxForCollection(collection());

    auto *expunge = new KIMAP::ExpungeJob(session);
    connect(expunge, &KJob::result, this, [this, expunge](KJob *job) {
        if (job->error()) {
            qCWarning(IMAPRESOURCE_LOG) << "Expunging mailbox" << mailBoxForCollection(collection()) << "failed:" << job->errorString();
            cancelTask(job->errorString());
            return;
        }

        // No point incrementally removing messages, if we are doing a full sync.
        if (m_syncMode == SyncMode::Incremental) {
            removeLocalMessages(expunge->vanishedMessages());
        }

        m_serverState.highestModSeq = std::max(expunge->newHighestModSeq(), m_serverState.highestModSeq);
        qCDebug(IMAPRESOURCE_LOG) << "New highestmodseq for mailbox" << mailBoxForCollection(collection()) << "after expunge:" << m_serverState.highestModSeq;

        fetchNewMessages(expunge->session());
    });
    expunge->start();
}

void RetrieveItemsTaskQResync::onReadyForNextBatch(int size)
{
    Q_UNUSED(size);

    if (m_batchFetcher) {
        m_batchFetcher->fetchNextBatch();
    }
}

void RetrieveItemsTaskQResync::fetchNewMessages(KIMAP::Session *session)
{
    // If nextUID remained the same, no new messages arrived, so we don't need to fetch anything.
    if (m_localState.nextUid == m_serverState.nextUid) {
        finishSync();
        return;
    }


    KIMAP::FetchJob::FetchScope scope;
    scope.parts.clear();
    scope.mode = shouldFetchFullPayload(collection()) ? KIMAP::FetchJob::FetchScope::Full
                                                      : KIMAP::FetchJob::FetchScope::FullHeaders;

    m_batchFetcher.reset(new BatchFetcher(resourceState()->messageHelper(), {}, scope, batchSize(), session));
    m_batchFetcher->setUidBased(true);
    m_batchFetcher->setSearchUids(KIMAP::ImapInterval{m_localState.nextUid, m_serverState.nextUid});

    connect(m_batchFetcher.get(), &BatchFetcher::itemsRetrieved, [this](const Akonadi::Item::List &items) {
        if (m_syncMode == SyncMode::Full) {
            itemsRetrieved(items);
        } else {
            itemsRetrievedIncremental(items, {});
        }

        m_stats.appended += items.size();
    });
    connect(m_batchFetcher.get(), &KJob::result, this, [this](KJob *job) {
        if (job->error()) {
            qCWarning(IMAPRESOURCE_LOG) << "Fetching emails from mailbox" << mailBoxForCollection(collection()) << "(col" << collection().id() << "):" << job->errorString();
            cancelTask(job->errorString());
            return;
        }

        qCDebug(IMAPRESOURCE_LOG) << "New message retrieval for mailbox" << mailBoxForCollection(collection()) << "done";

        // Aaaaand done!
        finishSync();
    });

    qCDebug(IMAPRESOURCE_LOG) << "Starting fetch of messages (" << m_localState.nextUid << ":*) from" << mailBoxForCollection(collection());
    m_batchFetcher->start();
}

void RetrieveItemsTaskQResync::updateLocalCollection()
{
    Akonadi::Collection col = collection();
    bool changed = false;
    changed |= updateAttr<UidNextAttribute>(col, &UidNextAttribute::uidNext, &UidNextAttribute::setUidNext, m_serverState.nextUid);
    changed |= updateAttr<UidValidityAttribute>(col, &UidValidityAttribute::uidValidity, &UidValidityAttribute::setUidValidity, m_serverState.uidValidity);
    changed |= updateAttr<HighestModSeqAttribute>(col, &HighestModSeqAttribute::highestModSequence, &HighestModSeqAttribute::setHighestModSeq, static_cast<qint64>(m_serverState.highestModSeq));
    changed |= updateAttr<Akonadi::CollectionFlagsAttribute>(col, &Akonadi::CollectionFlagsAttribute::flags, &Akonadi::CollectionFlagsAttribute::setFlags, m_serverState.flags);

    if (changed) {
        applyCollectionChanges(col);
    }
}

void RetrieveItemsTaskQResync::finishSync()
{
    // Tell the Resource implementation if we did an incremental sync that came up empty
    if (m_stats.appended == 0 && m_stats.removed == 0 && m_stats.updated == 0) {
        if (m_syncMode == SyncMode::Incremental) {
            itemsRetrievedIncremental({}, {});
        }
    }

    itemsRetrievalDone();

    updateLocalCollection();

    qCInfo(IMAPRESOURCE_LOG) << "Sync of mailbox" << mailBoxForCollection(collection()) << "finished";
    qCDebug(IMAPRESOURCE_LOG) << "Duration:" << m_stats.timer.elapsed() << " ms";
    qCDebug(IMAPRESOURCE_LOG) << "Removed local messages:" << m_stats.removed;
    qCDebug(IMAPRESOURCE_LOG) << "Updated local messages:" << m_stats.updated;
    qCDebug(IMAPRESOURCE_LOG) << "Created local messages:" << m_stats.appended;
}

