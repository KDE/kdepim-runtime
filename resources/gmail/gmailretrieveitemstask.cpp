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

#include "gmailretrieveitemstask.h"
#include "gmailresourcestate.h"

#include <imap/batchfetcher.h>

#include <Akonadi/CollectionFetchJob>
#include <Akonadi/LinkJob>


GmailRetrieveItemsTask::GmailRetrieveItemsTask(ResourceStateInterface::Ptr resource, QObject *parent)
    : RetrieveItemsTask(resource, parent)
{
    kDebug();
    dynamic_cast<GmailResourceState*>(resource.get())->setCurrentTask(this);
}

GmailRetrieveItemsTask::~GmailRetrieveItemsTask()
{

}

BatchFetcher *GmailRetrieveItemsTask::createBatchFetcher(MessageHelper::Ptr messageHelper,
                                                         const KIMAP::ImapSet &set,
                                                         const KIMAP::FetchJob::FetchScope &scope,
                                                         int batchSize,
                                                         KIMAP::Session *session)
{
    kDebug();
    KIMAP::FetchJob::FetchScope gmailScope = scope;
    BatchFetcher *batchFetcher = new BatchFetcher(messageHelper, set, gmailScope, batchSize, session);
    batchFetcher->setGmailExtensionsEnabled(true);
    return batchFetcher;
}

bool GmailRetrieveItemsTask::serverSupportsCondstore() const
{
    return true;
}
