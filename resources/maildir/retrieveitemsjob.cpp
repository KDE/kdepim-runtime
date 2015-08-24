/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#include "retrieveitemsjob.h"
#include <itemfetchjob.h>
#include <itemmodifyjob.h>
#include <itemdeletejob.h>
#include <itemcreatejob.h>
#include <collectionmodifyjob.h>
#include <transactionsequence.h>
#include <Akonadi/KMime/MessageFlags>
#include <AkonadiCore/vectorhelper.h>
#include <QDirIterator>
#include <KMime/Message>

RetrieveItemsJob::RetrieveItemsJob(const Akonadi::Collection &collection, const KPIM::Maildir &md, QObject *parent) :
    Job(parent),
    m_collection(collection),
    m_maildir(md),
    m_mimeType(KMime::Message::mimeType()),
    m_transaction(Q_NULLPTR),
    m_transactionSize(0),
    m_entryIterator(Q_NULLPTR),
    m_previousMtime(0),
    m_highestMtime(0)
{
    Q_ASSERT(m_collection.isValid());
    Q_ASSERT(m_maildir.isValid());
}

void RetrieveItemsJob::setMimeType(const QString &mimeType)
{
    m_mimeType = mimeType;
}

void RetrieveItemsJob::doStart()
{
    Q_ASSERT(!m_mimeType.isEmpty());
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(m_collection, this);
    connect(job, &Akonadi::ItemFetchJob::result, this, &RetrieveItemsJob::localListDone);
}

void RetrieveItemsJob::localListDone(KJob *job)
{
    if (job->error()) {
        return;    // handled by base class
    }

    const Akonadi::Item::List items = qobject_cast<Akonadi::ItemFetchJob *>(job)->items();
    m_localItems.reserve(items.size());
    Q_FOREACH (const Akonadi::Item &item, items) {
        if (!item.remoteId().isEmpty()) {
            m_localItems.insert(item.remoteId(), item);
        }
    }

    m_listingPath = m_maildir.path() + QLatin1String("/new/");
    delete m_entryIterator;
    m_entryIterator = new QDirIterator(m_maildir.pathToNew(), QDir::Files);
    m_previousMtime = m_collection.remoteRevision().toLongLong();
    m_highestMtime = 0;
    processEntry();
}

void RetrieveItemsJob::processEntry()
{
    QFileInfo entryInfo;

    QString filePath = m_entryIterator->next();

    QString fileName = m_entryIterator->fileName();

    bool newItemFound = false;
    while (!newItemFound) {
        if (filePath.isEmpty()) {
            if (m_listingPath.endsWith(QLatin1String("/new/"))) {
                m_listingPath = m_maildir.path() + QLatin1String("/cur/");
                delete m_entryIterator;
                m_entryIterator = new QDirIterator(m_maildir.pathToCurrent(), QDir::Files);
                processEntry();
            } else {
                entriesProcessed();
            }
            return;
        }

        entryInfo = m_entryIterator->fileInfo();
        const qint64 currentMtime = entryInfo.lastModified().toMSecsSinceEpoch();
        m_highestMtime = qMax(m_highestMtime, currentMtime);
        if (currentMtime <= m_previousMtime) {
            auto localItemIter = m_localItems.find(fileName);
            if (localItemIter != m_localItems.end()) {  // old, we got this one already
                m_localItems.erase(localItemIter);
                filePath = m_entryIterator->next();
                fileName = m_entryIterator->fileName();
            } else {
                newItemFound = true;
            }
        } else {
            newItemFound = true;
        }
    }

    Akonadi::Item item;
    item.setRemoteId(fileName);
    item.setMimeType(m_mimeType);
    const qint64 entrySize = entryInfo.size();
    if (entrySize >= 0) {
        item.setSize(entrySize);
    }

    KMime::Message *msg = new KMime::Message;
    msg->setHead(KMime::CRLFtoLF(m_maildir.readEntryHeadersFromFile(m_listingPath + fileName)));
    msg->parse();

    Akonadi::Item::Flags flags = m_maildir.readEntryFlags(fileName);
    Q_FOREACH (const Akonadi::Item::Flag &flag, flags) {
        item.setFlag(flag);
    }

    item.setPayload(KMime::Message::Ptr(msg));
    Akonadi::MessageFlags::copyMessageFlags(*msg, item);
    KJob *job = Q_NULLPTR;
    auto localItemIter = m_localItems.find(fileName);
    if (localItemIter == m_localItems.end()) { // new item
        job = new Akonadi::ItemCreateJob(item, m_collection, transaction());
    } else { // modification
        item.setId((*localItemIter).id());
        job = new Akonadi::ItemModifyJob(item, transaction());
        m_localItems.erase(localItemIter);
    }
    connect(job, &Akonadi::ItemCreateJob::result, this, &RetrieveItemsJob::processEntryDone);
}

void RetrieveItemsJob::processEntryDone(KJob *)
{
    processEntry();
}

void RetrieveItemsJob::entriesProcessed()
{
    delete m_entryIterator;
    m_entryIterator = Q_NULLPTR;
    if (!m_localItems.isEmpty()) {
        Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob(Akonadi::valuesToVector(m_localItems), transaction());
        m_maildir.removeCachedKeys(m_localItems.keys());
        // We ensure m_transaction is valid by calling transaction() above,
        // however calling it again here could cause it to give us another transaction
        // object (see transaction() comment for details)
        m_transaction->setIgnoreJobFailure(job);
    }

    // update mtime
    if (m_highestMtime != m_previousMtime) {
        Akonadi::Collection newCol(m_collection);
        newCol.setRemoteRevision(QString::number(m_highestMtime));
        Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob(newCol, transaction());
        m_transaction->setIgnoreJobFailure(job);
    }

    if (!m_transaction) { // no jobs created here -> done
        emitResult();
    } else {
        connect(m_transaction, &Akonadi::TransactionSequence::result,
                this, &RetrieveItemsJob::transactionDone);
        m_transaction->commit();
    }
}

Akonadi::TransactionSequence *RetrieveItemsJob::transaction()
{
    // Commit transaction every 100 items, otherwise we are forcing server to
    // hold the database transaction opened for potentially massive amount of
    // operations, which slowly overloads the database journal causing simple
    // INSERT to take several seconds
    if (++m_transactionSize >= 100) {
        m_transaction->commit();
        m_transaction = Q_NULLPTR;
        m_transactionSize = 0;
    }

    if (!m_transaction) {
        m_transaction = new Akonadi::TransactionSequence(this);
        m_transaction->setAutomaticCommittingEnabled(false);
    }
    return m_transaction;
}

void RetrieveItemsJob::transactionDone(KJob *job)
{
    if (job->error()) {
        return;    // handled by base class
    }
    emitResult();
}

