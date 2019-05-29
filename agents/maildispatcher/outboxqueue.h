/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef OUTBOXQUEUE_H
#define OUTBOXQUEUE_H

#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>

#include <QObject>

class KJob;
class QTimer;
namespace Akonadi {
class Monitor;
}

/**
 * @short Monitors the outbox collection and provides a queue of messages for the MDA to send.
 */
class OutboxQueue : public QObject
{
    Q_OBJECT
    friend class MailDispatcherAgent;

public:
    /**
     * Creates a new outbox queue.
     *
     * @param parent The parent object.
     */
    explicit OutboxQueue(QObject *parent = nullptr);

    /**
     * Destroys the outbox queue.
     */
    ~OutboxQueue() override;

    /**
     * Returns whether the queue is empty.
     */
    Q_REQUIRED_RESULT bool isEmpty() const;

    /**
     * Returns the number of items in the queue.
     */
    Q_REQUIRED_RESULT int count() const;

    /**
     * Returns the size (in bytes) of all items in the queue.
     */
    qulonglong totalSize() const;

    /**
     * Fetches an item and emits itemReady() when done.
     */
    void fetchOne();

Q_SIGNALS:
    void itemReady(const Akonadi::Item &item);
    void newItems();
    void error(const QString &error);

private:
    void initQueue();
    void addIfComplete(const Akonadi::Item &item);

    // Q_SLOTS:
    void checkFuture();
    void collectionFetched(KJob *job);
    void itemFetched(KJob *job);
    void localFoldersChanged();
    void localFoldersRequestResult(KJob *job);
    void itemAdded(const Akonadi::Item &item);
    void itemChanged(const Akonadi::Item &item);
    void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &dest);
    void itemRemoved(const Akonadi::Item &item);
    void itemProcessed(const Akonadi::Item &item, bool result);

    QList<Akonadi::Item> mQueue;
    QSet<Akonadi::Item> mFutureItems; // keeps track of items removed in the meantime
    QMultiMap<QDateTime, Akonadi::Item> mFutureMap;
    Akonadi::Collection mOutbox = Akonadi::Collection(-1);
    Akonadi::Monitor *mMonitor = nullptr;
    QTimer *mFutureTimer = nullptr;
    qulonglong mTotalSize = 0;
    int mOutboxDiscoveryRetries = 0;

#if 0
    // If an item is modified externally between the moment we pass it to
    // the MDA and the time the MDA marks it as sent, then we will get
    // itemChanged() and may mistakenly re-add the item to the queue.
    // So we ignore the item that we pass to the MDA, until the MDA finishes
    // sending it.
    Item currentItem;
#endif
    // HACK: The above is not enough.
    // Apparently change notifications are delayed sometimes (???)
    // and we re-add an item long after it was sent.  So keep a list of sent
    // items.
    // TODO debug and figure out why this happens.
    QSet<Akonadi::Item::Id> mIgnore;
};

#endif
