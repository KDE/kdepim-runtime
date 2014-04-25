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

#include <akonadi/cachepolicy.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/kmime/messageflags.h>
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

/**
 * A job that retrieves a set of messages in reverse-ordered batches.
 * After each batch fetchNextBatch() needs to be called (for throttling the download speed)
 */
class BatchFetcher : public KJob {
    Q_OBJECT
public:
    BatchFetcher(const KIMAP::ImapSet &set, const KIMAP::FetchJob::FetchScope &scope, int batchSize, KIMAP::Session *session);
    virtual ~BatchFetcher();
    virtual void start();
    void fetchNextBatch();
    void setUidBased(bool);
    qint64 lowestUidFetched() const;

Q_SIGNALS:
    void itemsRetrieved(Akonadi::Item::List);

private Q_SLOTS:
    void onHeadersReceived(const QString &mailBox, const QMap<qint64, qint64> &uids,
                                           const QMap<qint64, qint64> &sizes,
                                           const QMap<qint64, KIMAP::MessageFlags> &flags,
                                           const QMap<qint64, KIMAP::MessagePtr> &messages);
    void onHeadersFetchDone(KJob *job);

private:
    //Batch fetching
    KIMAP::ImapSet m_currentSet;
    KIMAP::FetchJob::FetchScope m_scope;
    KIMAP::Session *m_session;
    int m_batchSize;
    bool m_uidBased;
    qint64 m_lowestUid;
};

BatchFetcher::BatchFetcher(const KIMAP::ImapSet &set, const KIMAP::FetchJob::FetchScope& scope, int batchSize, KIMAP::Session* session)
    : KJob(session),
    m_currentSet(set),
    m_scope(scope),
    m_session(session),
    m_batchSize(batchSize),
    m_uidBased(false),
    m_lowestUid(-1)
{
}

BatchFetcher::~BatchFetcher()
{
}

void BatchFetcher::setUidBased(bool uidBased)
{
    m_uidBased = uidBased;
}

void BatchFetcher::start()
{
    fetchNextBatch();
}

void BatchFetcher::fetchNextBatch()
{
    Q_ASSERT(m_batchSize > 0);
    if (m_currentSet.isEmpty()) {
        kDebug() << "fetch complete";
        emitResult();
        return;
    }
    const KIMAP::ImapInterval interval = m_currentSet.intervals().first();
    Q_ASSERT(interval.hasDefinedEnd());

    KIMAP::FetchJob *fetch = new KIMAP::FetchJob(m_session);
    if (!m_uidBased) {
        //Reverse fetching would be great, because it gives you the most relevant (recent) messages first,
        //but since we usually just check the number of messages, we have the problem that we don't know
        //whether the interval at the beginning or the end is missing. Therefore it's disabled for now

        //get an interval of m_batchSize
//         const qint64 end = interval.end();
//         const qint64 begin = qMax(interval.begin(), end - m_batchSize + 1);
//         const KIMAP::ImapSet intervalToFetch(begin, end);
//         //fetch items in reverse order in chunks
//         if (interval.begin() < (begin - 1)) {
//             m_currentSet = KIMAP::ImapSet(interval.begin(), begin - 1);
//         } else {
//             m_currentSet = KIMAP::ImapSet();
//         }
        const qint64 begin = interval.begin();
        const qint64 end = qMin(interval.end(), begin + m_batchSize - 1);
        const KIMAP::ImapSet intervalToFetch(begin, end);
        if (interval.end() > end) {
            m_currentSet = KIMAP::ImapSet(end + 1, interval.end());
        } else {
            m_currentSet = KIMAP::ImapSet();
        }
        fetch->setSequenceSet(intervalToFetch);
    } else {
        fetch->setSequenceSet(m_currentSet);
        m_currentSet = KIMAP::ImapSet();
    }

    if (m_uidBased) {
        fetch->setUidBased(true);
    }
    fetch->setScope(m_scope);
    connect(fetch, SIGNAL(headersReceived(QString,QMap<qint64,qint64>,QMap<qint64,qint64>,QMap<qint64,KIMAP::MessageFlags>,QMap<qint64,KIMAP::MessagePtr>)),
            this, SLOT(onHeadersReceived(QString,QMap<qint64,qint64>,QMap<qint64,qint64>,QMap<qint64,KIMAP::MessageFlags>,QMap<qint64,KIMAP::MessagePtr>)) );
    connect(fetch, SIGNAL(result(KJob*)),
            this, SLOT(onHeadersFetchDone(KJob*)));
    fetch->start();
}

void BatchFetcher::onHeadersReceived(const QString &mailBox, const QMap<qint64, qint64> &uids,
                                           const QMap<qint64, qint64> &sizes,
                                           const QMap<qint64, KIMAP::MessageFlags> &flags,
                                           const QMap<qint64, KIMAP::MessagePtr> &messages)
{
    Akonadi::Item::List addedItems;
    foreach (qint64 number, uids.keys()) { //krazy:exclude=foreach
        const qint64 uid = uids[number];
        Akonadi::Item i;
        i.setRemoteId(QString::number(uid));
        i.setMimeType(KMime::Message::mimeType());
        i.setPayload(KMime::Message::Ptr(messages[number]));
        i.setSize(sizes[number]);

        // update status flags
        if (KMime::isSigned(messages[number].get())) {
            i.setFlag(Akonadi::MessageFlags::Signed);
        }
        if (KMime::isEncrypted(messages[number].get())) {
            i.setFlag(Akonadi::MessageFlags::Encrypted);
        }
        if (KMime::isInvitation(messages[number].get())) {
            i.setFlag(Akonadi::MessageFlags::HasInvitation);
        }
        if (KMime::hasAttachment(messages[number].get())) {
            i.setFlag(Akonadi::MessageFlags::HasAttachment);
        }

        const QList<QByteArray> akonadiFlags = ResourceTask::toAkonadiFlags(flags[number]);
        foreach (const QByteArray &flag, akonadiFlags) {
            i.setFlag(flag);
        }
        if (m_lowestUid > uid || m_lowestUid == -1) {
            m_lowestUid = uid;
        }
        //kDebug( 5327 ) << "Flags: " << i.flags();
        addedItems << i;
    }
//     kDebug() << addedItems.size();
    emit itemsRetrieved(addedItems);
}

void BatchFetcher::onHeadersFetchDone( KJob *job )
{
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
}

qint64 BatchFetcher::lowestUidFetched() const
{
    return m_lowestUid;
}


RetrieveItemsTask::RetrieveItemsTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( CancelIfNoSession, resource, parent ),
  m_session( 0 ),
  m_fetchedMissingBodies( -1 ),
  m_fetchMissingBodies( false ),
  m_batchFetcher( 0 ),
  m_lowestExpectedUid( 0 )
{

}

RetrieveItemsTask::~RetrieveItemsTask()
{
}

void RetrieveItemsTask::setFetchMissingItemBodies(bool enabled)
{
  m_fetchMissingBodies = enabled;
}

void RetrieveItemsTask::doStart( KIMAP::Session *session )
{
  emitPercent(0);
  // Prevent fetching items from noselect folders.
  if ( collection().hasAttribute( "noselect" ) ) {
    NoSelectAttribute* noselect = static_cast<NoSelectAttribute*>( collection().attribute( "noselect" ) );
    if ( noselect->noSelect() ) {
      kDebug( 5327 ) << "No Select folder";
      itemsRetrievalDone();
      return;
    }
  }

  m_session = session;

  const Akonadi::Collection col = collection();
  if ( m_fetchMissingBodies && col.cachePolicy()
       .localParts().contains( QLatin1String(Akonadi::MessagePart::Body) ) ) { //disconnected mode, make sure we really have the body cached

    Akonadi::Session *session = new Akonadi::Session( resourceName().toLatin1() + "_body_checker", this );
    Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob( col, session );
    fetchJob->fetchScope().setCheckForCachedPayloadPartsOnly();
    fetchJob->fetchScope().fetchPayloadPart( Akonadi::MessagePart::Body );
    fetchJob->fetchScope().setFetchModificationTime( false );
    connect( fetchJob, SIGNAL(result(KJob*)), this, SLOT(fetchItemsWithoutBodiesDone(KJob*)) );
    connect( fetchJob, SIGNAL(result(KJob*)), session, SLOT(deleteLater()) );
  } else {
    startRetrievalTasks();
  }
}

void RetrieveItemsTask::fetchItemsWithoutBodiesDone( KJob *job )
{
  Akonadi::ItemFetchJob *fetch = static_cast<Akonadi::ItemFetchJob*>( job );
  QList<qint64> uids;
  if ( job->error() ) {
    kWarning() << job->errorString();
    cancelTask( job->errorString() );
    return;
  } else {
    int i = 0;
    Q_FOREACH( const Akonadi::Item &item, fetch->items() )  {
      if ( !item.cachedPayloadParts().contains( Akonadi::MessagePart::Body ) ) {
          kWarning() << "Item " << item.id() << " is missing the payload! Cached payloads: " << item.cachedPayloadParts();
          uids.append( item.remoteId().toInt() );
          i++;
      }
    }
    if ( i > 0 ) {
      kWarning() << "Number of items missing the body: " << i;
    }
  }

  onFetchItemsWithoutBodiesDone(uids);
}

void RetrieveItemsTask::onFetchItemsWithoutBodiesDone( const QList<qint64> &items )
{
  m_messageUidsMissingBody = items;
  startRetrievalTasks();
}


void RetrieveItemsTask::startRetrievalTasks()
{
  const QString mailBox = mailBoxForCollection( collection() );

  // Now is the right time to expunge the messages marked \\Deleted from this mailbox.
  if ( isAutomaticExpungeEnabled() ) {
    if ( m_session->selectedMailBox() != mailBox ) {
      triggerPreExpungeSelect( mailBox );
    } else {
      triggerExpunge( mailBox );
    }
  } else {
    // Always select to get the stats updated
    triggerFinalSelect( mailBox );
  }
}

void RetrieveItemsTask::triggerPreExpungeSelect( const QString &mailBox )
{
  KIMAP::SelectJob *select = new KIMAP::SelectJob( m_session );
  select->setMailBox( mailBox );
  select->setCondstoreEnabled( serverSupportsCondstore() );
  connect( select, SIGNAL(result(KJob*)),
           this, SLOT(onPreExpungeSelectDone(KJob*)) );
  select->start();
}

void RetrieveItemsTask::onPreExpungeSelectDone( KJob *job )
{
  if ( job->error() ) {
    kWarning() << job->errorString();
    cancelTask( job->errorString() );
  } else {
    KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob*>( job );
    triggerExpunge( select->mailBox() );
  }
}

void RetrieveItemsTask::triggerExpunge( const QString &mailBox )
{
  kDebug( 5327 ) << mailBox;

  KIMAP::ExpungeJob *expunge = new KIMAP::ExpungeJob( m_session );
  connect( expunge, SIGNAL(result(KJob*)),
           this, SLOT(onExpungeDone(KJob*)) );
  expunge->start();
}

void RetrieveItemsTask::onExpungeDone( KJob *job )
{
  // We can ignore the error, we just had a wrong expunge so some old messages will just reappear.
  // TODO we should probably hide messages that are marked as deleted (skipping will not work with our mode of synchronization)
  if (job->error()) {
    kWarning() << "Expunge failed: " << job->errorString();
  }
  // Except for network errors.
  if ( job->error() && m_session->state() == KIMAP::Session::Disconnected ) {
    cancelTask( job->errorString() );
    return;
  }

  // We have to re-select the mailbox to update all the stats after the expunge
  // (the EXPUNGE command doesn't return enough for our needs)
  triggerFinalSelect( m_session->selectedMailBox() );
}

void RetrieveItemsTask::triggerFinalSelect( const QString &mailBox )
{
  KIMAP::SelectJob *select = new KIMAP::SelectJob( m_session );
  select->setMailBox( mailBox );
  select->setCondstoreEnabled( serverSupportsCondstore() );
  connect( select, SIGNAL(result(KJob*)),
           this, SLOT(onFinalSelectDone(KJob*)) );
  select->start();
}

void RetrieveItemsTask::onFinalSelectDone( KJob *job )
{
  if ( job->error() ) {
    kWarning() << job->errorString();
    cancelTask( job->errorString() );
    return;
  }

  KIMAP::SelectJob *select = qobject_cast<KIMAP::SelectJob*>( job );

  const QString mailBox = select->mailBox();
  const int messageCount = select->messageCount();
  const qint64 uidValidity = select->uidValidity();
  const qint64 nextUid = select->nextUid();
  quint64 highestModSeq = select->highestModSequence();
  const QList<QByteArray> flags = select->permanentFlags();

  //The select job retrieves highestmodset whenever it's available, but in case of no CONDSTORE support we ignore it
  if ( !serverSupportsCondstore() ) {
    highestModSeq = 0;
  }

  Akonadi::Collection col = collection();
  bool modifyNeeded = false;

  // Get the current uid validity value and store it
  int oldUidValidity = 0;
  if ( !col.hasAttribute( "uidvalidity" ) ) {
    UidValidityAttribute* currentUidValidity  = new UidValidityAttribute( uidValidity );
    col.addAttribute( currentUidValidity );
    modifyNeeded = true;
  } else {
    UidValidityAttribute* currentUidValidity =
      static_cast<UidValidityAttribute*>( col.attribute( "uidvalidity" ) );
    oldUidValidity = currentUidValidity->uidValidity();
    if ( oldUidValidity != uidValidity ) {
      currentUidValidity->setUidValidity( uidValidity );
      modifyNeeded = true;
    }
  }

  // Get the current uid next value and store it
  int oldNextUid = 0;
  if ( !col.hasAttribute( "uidnext" ) ) {
    UidNextAttribute* currentNextUid  = new UidNextAttribute( nextUid );
    col.addAttribute( currentNextUid );
    modifyNeeded = true;
  } else {
    UidNextAttribute* currentNextUid =
      static_cast<UidNextAttribute*>( col.attribute( "uidnext" ) );
    oldNextUid = currentNextUid->uidNext();
    if ( oldNextUid != nextUid ) {
      currentNextUid->setUidNext( nextUid );
      modifyNeeded = true;
    }
  }

  // Store the mailbox flags
  if ( !col.hasAttribute( "collectionflags" ) ) {
    Akonadi::CollectionFlagsAttribute *flagsAttribute  = new Akonadi::CollectionFlagsAttribute( flags );
    col.addAttribute( flagsAttribute );
    modifyNeeded = true;
  } else {
    Akonadi::CollectionFlagsAttribute *flagsAttribute =
      static_cast<Akonadi::CollectionFlagsAttribute*>( col.attribute( "collectionflags" ) );
    const QList<QByteArray> oldFlags = flagsAttribute->flags();
    if ( oldFlags != flags ) {
      flagsAttribute->setFlags( flags );
      modifyNeeded = true;
    }
  }

  quint64 oldHighestModSeq = 0;
  if ( serverSupportsCondstore() && highestModSeq > 0 ) {
    if ( !col.hasAttribute( "highestmodseq" ) ) {
      HighestModSeqAttribute *attr = new HighestModSeqAttribute( highestModSeq );
      col.addAttribute( attr );
      modifyNeeded = true;
    } else {
      HighestModSeqAttribute *attr = col.attribute<HighestModSeqAttribute>();
      if ( attr->highestModSequence() < highestModSeq ) {
        oldHighestModSeq = attr->highestModSequence();
        attr->setHighestModSeq( highestModSeq );
        modifyNeeded = true;
      } else if ( attr->highestModSequence() == highestModSeq ) {
        oldHighestModSeq = attr->highestModSequence();
      } else if ( attr->highestModSequence() > highestModSeq ) {
        // This situation should not happen. If it does, update the highestModSeq
        // attribute, but rather do a full sync
        attr->setHighestModSeq( highestModSeq );
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

  qint64 realMessageCount = col.statistics().count();
  if (realMessageCount < 0) {
      realMessageCount = 0;
  }
  m_fetchedMissingBodies = -1;

  // First check the uidvalidity, if this has changed, it means the folder
  // has been deleted and recreated. So we wipe out the messages and
  // retrieve all.
  if ( oldUidValidity != uidValidity && messageCount > 0 ) {
    if (oldUidValidity != 0) {
        kDebug( 5327 ) << "UIDVALIDITY check failed (" << oldUidValidity << "|"
                    << uidValidity << ") refetching " << mailBox;
    } else {
        kDebug( 5327 ) << "Fetching complete mailbox " << mailBox;
    }

    setTotalItems(messageCount);
    retrieveItems(KIMAP::ImapSet(1, messageCount), scope, false);
  } else if ( messageCount > realMessageCount && messageCount > 0 ) {
    // The amount on the server is bigger than that we have in the cache
    // that probably means that there is new mail. Fetch missing.
    kDebug( 5327 ) << "Fetch missing: " << messageCount << " But: " << realMessageCount;

    // Since we're only fetching new messages, we know the lowest expected uid
    // If we get something lower, our realMessageCount doesn't correspond to that of the server,
    // meaning we're missing some messages locally.
    m_lowestExpectedUid = oldNextUid;
    setTotalItems(messageCount - realMessageCount);
    retrieveItems(KIMAP::ImapSet( realMessageCount + 1, messageCount ), scope, realMessageCount != 0);
  } else if ( messageCount == realMessageCount && oldNextUid != nextUid
           && oldNextUid != 0 && messageCount > 0 ) {
    // amount is right but uidnext is different.... something happened
    // behind our back...

    qint64 startIndex = 1;
    // one scenario we can recover from is that an equal amount of mails has been deleted and added while we were not looking
    // the amount has to be less or equal to (nextUid - oldNextUid) due to strictly ascending UIDs
    // so, we just have to reload the last (nextUid - oldNextUid) mails if the uidnext values seem sane
    if ( oldNextUid < nextUid && oldNextUid != 0 ) {
      startIndex = qMax( 1ll, messageCount - ( nextUid - oldNextUid ) );
    }

    Q_ASSERT( startIndex >= 1 );
    Q_ASSERT( startIndex <= messageCount );

    kDebug( 5327 ) << "UIDNEXT check failed, refetching mailbox: " << startIndex << messageCount;
    setTotalItems(messageCount - startIndex);
    retrieveItems(KIMAP::ImapSet( startIndex, messageCount ), scope, false);
  } else if (!m_messageUidsMissingBody.isEmpty() ) {
    //fetch missing uids
    m_fetchedMissingBodies = 0;
    setTotalItems(m_messageUidsMissingBody.size());
    KIMAP::ImapSet imapSet;
    imapSet.add( m_messageUidsMissingBody );

    m_batchFetcher = new BatchFetcher(imapSet, scope, batchSize(), m_session);
    m_batchFetcher->setUidBased(true);
    m_incremental = true;
    m_batchFetcher->setProperty("alreadyFetched", 1);
    connect(m_batchFetcher, SIGNAL(itemsRetrieved(Akonadi::Item::List)),
            this, SLOT(onItemsRetrieved(Akonadi::Item::List)));
    connect(m_batchFetcher, SIGNAL(result(KJob*)),
            this, SLOT(onRetrievalDone(KJob*)));
    m_batchFetcher->start();
  } else if ( messageCount > 0 ) {
    if ( messageCount < realMessageCount ) {
        // Some messages were removed, list all flags to find out which messages
        // are missing
        kDebug( 5327 ) << ( realMessageCount - messageCount ) << "messages were removed from maildir";
        m_incremental = false;
    } else {
        kDebug( 5327 ) << "All fine, asking for changed flags looking for changes";
        m_incremental = true;
    }
    setTotalItems(messageCount);
    listFlagsForImapSet( KIMAP::ImapSet( 1, messageCount ));
  } else {
    m_incremental = false;
    if (realMessageCount > 0) {
        kDebug( 5327 ) << "No messages present so we are done, deleting local messages.";
        itemsRetrieved(Akonadi::Item::List());
    } else {
        kDebug( 5327 ) << "No messages present so we are done";
    }
    taskComplete();
  }
}

void RetrieveItemsTask::retrieveItems(const KIMAP::ImapSet& set, const KIMAP::FetchJob::FetchScope &scope, bool incremental)
{
  Q_ASSERT(set.intervals().size() == 1);

  m_incremental = incremental;

  m_batchFetcher = new BatchFetcher(set, scope, batchSize(), m_session);
  m_batchFetcher->setProperty("alreadyFetched", set.intervals().first().begin());
  connect(m_batchFetcher, SIGNAL(itemsRetrieved(Akonadi::Item::List)),
          this, SLOT(onItemsRetrieved(Akonadi::Item::List)));
  connect(m_batchFetcher, SIGNAL(result(KJob*)),
          this, SLOT(onRetrievalDone(KJob*)));
  m_batchFetcher->start();
}

void RetrieveItemsTask::onReadyForNextBatch(int size)
{
  if (m_batchFetcher) {
    m_batchFetcher->fetchNextBatch();
  }
}

void RetrieveItemsTask::onItemsRetrieved(const Akonadi::Item::List &addedItems)
{
  if ( m_incremental ) {
    itemsRetrievedIncremental( addedItems, Akonadi::Item::List() );
  } else {
    itemsRetrieved( addedItems );
  }

  //m_fetchedMissingBodies is -1 if we fetch for other reason, but missing bodies
  if ( m_fetchedMissingBodies != -1 ) {
    const QString mailBox = mailBoxForCollection( collection() );
    m_fetchedMissingBodies += addedItems.count();
    emit status(Akonadi::AgentBase::Running,
                i18nc( "@info:status", "Fetching missing mail bodies in %3: %1/%2", m_fetchedMissingBodies, m_messageUidsMissingBody.count(), mailBox));
  }
}

void RetrieveItemsTask::onRetrievalDone( KJob *job )
{
    m_batchFetcher = 0;
    if ( job->error() ) {
        kWarning() << job->errorString();
        cancelTask( job->errorString() );
        m_fetchedMissingBodies = -1;
        return;
    }

    BatchFetcher *batchFetcher = static_cast<BatchFetcher*>(job);
    if (batchFetcher->lowestUidFetched() < m_lowestExpectedUid) {
        //We just fetched an item we already have,
        //we probably miss some messages on the lower end.
        Akonadi::Collection col = collection();
        //This will trigger a complete resync
        col.removeAttribute("uidvalidity");
        applyCollectionChanges(col);
        kWarning() << "Inconsistent cache, aborting sync.";
        cancelTask( i18n("Inconsistent cache, aborting sync.") );
        //TODO deferTask instead?
        return;
    }

    //This is the lowest sequence number that we just fetched.
    const KIMAP::ImapSet::Id alreadyFetchedBegin = job->property("alreadyFetched").value<KIMAP::ImapSet::Id>();

    // If this is the first fetch of a folder, skip getting flags, we
    // already have them all from the previous full fetch. This is not
    // just an optimization, as incremental retrieval assumes nothing
    // will be listed twice.
    if ( m_fetchedMissingBodies != -1 || alreadyFetchedBegin <= 1 ) {
        taskComplete();
        return;
    }

    // Fetch flags of all items that were not fetched by the fetchJob. After
    // that /all/ items in the folder are synced.
    listFlagsForImapSet(KIMAP::ImapSet(1, alreadyFetchedBegin - 1));
}


void RetrieveItemsTask::listFlagsForImapSet( const KIMAP::ImapSet& set )
{
  KIMAP::FetchJob::FetchScope scope;
  scope.parts.clear();
  scope.mode = KIMAP::FetchJob::FetchScope::Flags;
  // Only use changeSince when doing incremental listings,
  // otherwise we would overwrite our local data with an incomplete dataset
  if(m_incremental && serverSupportsCondstore()) {
      scope.changedSince = m_highestModseq;
  }

  KIMAP::FetchJob* fetch = new KIMAP::FetchJob( m_session );
  fetch->setSequenceSet( set );
  fetch->setScope( scope );
  connect( fetch, SIGNAL(headersReceived(QString,QMap<qint64,qint64>,QMap<qint64,qint64>,QMap<qint64,KIMAP::MessageFlags>,QMap<qint64,KIMAP::MessagePtr>)),
           this, SLOT(onFlagsReceived(QString,QMap<qint64,qint64>,QMap<qint64,qint64>,QMap<qint64,KIMAP::MessageFlags>,QMap<qint64,KIMAP::MessagePtr>)) );
  connect( fetch, SIGNAL(result(KJob*)),
           this, SLOT(onFlagsFetchDone(KJob*)) );
  fetch->start();
}

void RetrieveItemsTask::onFlagsReceived( const QString &mailBox, const QMap<qint64, qint64> &uids,
                                         const QMap<qint64, qint64> &sizes,
                                         const QMap<qint64, KIMAP::MessageFlags> &flags,
                                         const QMap<qint64, KIMAP::MessagePtr> &messages )
{
  Q_UNUSED( mailBox );
  Q_UNUSED( sizes );
  Q_UNUSED( messages );

  Akonadi::Item::List changedItems;

  foreach ( qint64 number, uids.keys() ) { //krazy:exclude=foreach
    Akonadi::Item i;
    i.setRemoteId( QString::number( uids[number] ) );
    i.setMimeType( KMime::Message::mimeType() );
    i.setFlags( Akonadi::Item::Flags::fromList( toAkonadiFlags( flags[number] ) ) );

    //kDebug( 5327 ) << "Flags: " << i.flags();
    changedItems << i;
  }

  if ( !changedItems.isEmpty() ) {
    KIMAP::FetchJob *fetch = static_cast<KIMAP::FetchJob*>( sender() );
    if ( m_incremental ) {
        itemsRetrievedIncremental( changedItems, Akonadi::Item::List() );
    } else {
        itemsRetrieved( changedItems );
    }
  }
}

void RetrieveItemsTask::onFlagsFetchDone( KJob *job )
{
  if ( job->error() ) {
    kWarning() << job->errorString();
    cancelTask( job->errorString() );
  } else {
    taskComplete();
  }
}

void RetrieveItemsTask::taskComplete()
{
    if (m_modifiedCollection.isValid()) {
        applyCollectionChanges(m_modifiedCollection);
    }
    if ( m_incremental ) {
        // Calling itemsRetrievalDone() before previous call to itemsRetrievedIncremental()
        // behaves like if we called itemsRetrieved(Items::List()), so make sure
        // Akonadi knows we did incremental fetch that came up with no changes
        itemsRetrievedIncremental(Akonadi::Item::List(), Akonadi::Item::List());
    }
    itemsRetrievalDone();
}

#include "retrieveitemstask.moc"
