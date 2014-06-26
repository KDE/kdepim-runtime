/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef GMAILRETRIEVEITEMSTASK_H
#define GMAILRETRIEVEITEMSTASK_H

#include <imap/retrieveitemstask.h>

#include <KIMAP/FetchJob>

class GmailRetrieveItemsTask : public RetrieveItemsTask
{
    Q_OBJECT
public:
    explicit GmailRetrieveItemsTask(ResourceStateInterface::Ptr resource, QObject *parent = 0);
    ~GmailRetrieveItemsTask();

    virtual bool serverSupportsCondstore() const;

Q_SIGNALS:
    void linkItem(const QString &remoteId, const QVector<QByteArray> &labels);

protected:
    BatchFetcher *createBatchFetcher(MessageHelper::Ptr messageHelper,
                                     const KIMAP::ImapSet &set,
                                     const KIMAP::FetchJob::FetchScope &scope,
                                     int batchSize,
                                     KIMAP::Session *session);

private:
    // Allow GmailMessageHelper to emit linkItem() for us
    friend class GmailMessageHelper;
};

#endif // GMAILRETRIEVEITEMSTASK_H
