/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef MAILDISPATCHERAGENT_H
#define MAILDISPATCHERAGENT_H

#include <AgentBase>
#include <AkonadiCore/Item>

class OutboxQueue;
class SendJob;
class SentActionHandler;
/**
 * @short This agent dispatches mail put into the outbox collection.
 */
class MailDispatcherAgent : public Akonadi::AgentBase
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Akonadi.MailDispatcherAgent")

public:
    explicit MailDispatcherAgent(const QString &id);
    ~MailDispatcherAgent() override;

Q_SIGNALS:
    /**
     * Emitted when the MDA has attempted to send an item.
     */
    void itemProcessed(const Akonadi::Item &item, bool result);

    /**
     * Emitted when the MDA has begun processing an item
     */
    Q_SCRIPTABLE void itemDispatchStarted();

protected:
    void doSetOnline(bool online) override;

private Q_SLOTS:
    void sendPercent(KJob *job, unsigned long percent);
private:
    // Q_SLOTS:
    void abort();
    void dispatch();
    void itemFetched(const Akonadi::Item &item);
    void queueError(const QString &message);
    void sendResult(KJob *job);
    void emitStatusReady();

    OutboxQueue *mQueue = nullptr;
    SendJob *mCurrentJob = nullptr;
    Akonadi::Item mCurrentItem;
    bool mAborting = false;
    bool mSendingInProgress = false;
    bool mSentAnything = false;
    bool mErrorOccurred = false;
    bool mShowSentNotification = true;
    qulonglong mSentItemsSize = 0;
    SentActionHandler *mSentActionHandler = nullptr;
};

#endif // MAILDISPATCHERAGENT_H
