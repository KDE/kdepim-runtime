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

#include "gmailspecialcollectionstask.h"

#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/KMime/SpecialMailCollections>


GmailSpecialCollectionsTask::GmailSpecialCollectionsTask(ResourceStateInterface::Ptr resource, QObject *parent)
    : ResourceTask(ResourceTask::DeferIfNoSession, resource, parent)
{
}

GmailSpecialCollectionsTask::~GmailSpecialCollectionsTask()
{
}

void GmailSpecialCollectionsTask::doStart(KIMAP::Session *session)
{
    Q_UNUSED(session);

    Akonadi::Collection root;
    root.setRemoteId(rootRemoteId());
    Akonadi::CollectionFetchJob *fetch
        = new Akonadi::CollectionFetchJob(root, Akonadi::CollectionFetchJob::FirstLevel, this);
    fetch->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::None);
    connect(fetch, SIGNAL(finished(KJob*)),
            this, SLOT(onCollectionsRetrieved(KJob*)));
}

void GmailSpecialCollectionsTask::onCollectionsRetrieved(KJob *job)
{
    if (job->error()) {
        emitError(job->errorString());
        taskDone();
        return;
    }

    Akonadi::CollectionFetchJob *fetch = qobject_cast<Akonadi::CollectionFetchJob*>(job);
    const Akonadi::Collection::List collections = fetch->collections();

    Q_FOREACH (const Akonadi::Collection &collection, collections) {
        Akonadi::SpecialMailCollections::Type type = Akonadi::SpecialMailCollections::Invalid;

        if (collection.remoteId() == QLatin1String("/INBOX")) {
            type = Akonadi::SpecialMailCollections::Inbox;
        } else if (collection.remoteId() == QLatin1String("/[Gmail]/Sent Mail")) {
            type = Akonadi::SpecialMailCollections::SentMail;
        } else if (collection.remoteId() == QLatin1String("/[Gmail]/Trash")) {
            type = Akonadi::SpecialMailCollections::Trash;
        } else if (collection.remoteId() == QLatin1String("/[Gmail]/Drafts")) {
            type = Akonadi::SpecialMailCollections::Drafts;
        }

        if (type != Akonadi::SpecialMailCollections::Invalid) {
            Akonadi::SpecialMailCollections::self()->registerCollection(type, collection);
        }
    }

    taskDone();
}

