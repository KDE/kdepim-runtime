/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
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

#include "gmailchangeitemslabelstask.h"

GmailChangeItemsLabelsTask::GmailChangeItemsLabelsTask(ResourceStateInterface::Ptr resource, QObject *parent)
    : ChangeItemsFlagsTask(resource, parent)
{
}

GmailChangeItemsLabelsTask::~GmailChangeItemsLabelsTask()
{
}

void GmailChangeItemsLabelsTask::doStart(KIMAP::Session *session)
{
    const QString mailbox = QLatin1String("[Gmail]/All Mail");
    if (session->selectedMailBox() != mailbox) {
        KIMAP::SelectJob *select = new KIMAP::SelectJob(session);
        select->setMailBox(mailbox);
        connect(select, SIGNAL(finished(KJob*)),
                this, SLOT(onSelectDone(KJob*)));
        select->start();
    } else {
        if (!addedFlags().isEmpty()) {
            triggerAppendFlagsJob(session);
        } else if (!removedFlags().isEmpty()) {
            triggerRemoveFlagsJob(session);
        } else {
            changeProcessed();
        }
    }
}

void GmailChangeItemsLabelsTask::triggerAppendFlagsJob(KIMAP::Session *session)
{
    KIMAP::StoreJob *store = prepareJob(session);
    store->setGMLabels(addedFlags().toList());
    store->setMode(KIMAP::StoreJob::AppendFlags);
    connect(store, SIGNAL(result(KJob*)),
            this, SLOT(onAppendFlagsDone(KJob*)));
    store->start();
}

void GmailChangeItemsLabelsTask::triggerRemoveFlagsJob(KIMAP::Session *session)
{
    KIMAP::StoreJob *store = prepareJob(session);
    store->setGMLabels(removedFlags().toList());
    store->setMode(KIMAP::StoreJob::RemoveFlags);
    connect(store, SIGNAL(result(KJob*)),
            this, SLOT(onRemoveFlagsDone(KJob*)));
    store->start();
}
