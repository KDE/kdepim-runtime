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

#ifndef SENDJOB_H
#define SENDJOB_H

#include <KJob>
#include <AkonadiCore/Item>
namespace Akonadi {
class Item;
class AgentInstance;
}
class QDBusInterface;
/**
 * @short A job to send a mail
 *
 * This class takes a prevalidated Item with all the required attributes,
 * sends it using MailTransport, and then stores the result of the sending
 * operation in the item.
 */
class SendJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Creates a new send job.
     *
     * @param mItem The item to send.
     * @param parent The parent object.
     */
    explicit SendJob(const Akonadi::Item &mItem, QObject *parent = nullptr);

    /**
     * Destroys the send job.
     */
    ~SendJob() override;

    /**
     * Starts the job.
     */
    void start() override;

    /**
     * If this function is called before the job is started, the SendJob will
     * just mark the item as aborted, instead of sending it.
     * Do not call this function more than once.
     */
    void setMarkAborted();

    /**
     * Aborts sending the item.
     *
     * This will give the item an ErrorAttribute of "aborted".
     * (No need to call setMarkAborted() if you call abort().)
     */
    void abort();

private Q_SLOTS:
    void transportPercent(KJob *job, unsigned long percent);
    void resourceResult(qlonglong itemId, int result, const QString &message);
private:
    void doAkonadiTransport();
    void doTraditionalTransport();
    void doPostJob(bool transportSuccess, const QString &transportMessage);
    void storeResult(bool success, const QString &message = QString());
    void abortPostJob();
    bool filterItem(int filterset);

    // slots
    void doTransport();
    void transportResult(KJob *job);
    void resourceProgress(const Akonadi::AgentInstance &instance);
    void postJobResult(KJob *job);
    void doEmitResult(KJob *job);
    void slotSentMailCollectionFetched(KJob *job);

    Akonadi::Item mItem;
    QString mResourceId;
    KJob *mCurrentJob = nullptr;
    QDBusInterface *mInterface = nullptr;
    bool mAborting = false;
};

#endif
