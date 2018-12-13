/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#include "addcollectiontask.h"

#include "collectionannotationsattribute.h"

#include "imapresource_debug.h"
#include <KLocalizedString>

#include <kimap/createjob.h>
#include <kimap/session.h>
#include <kimap/setmetadatajob.h>
#include <kimap/subscribejob.h>

#include <collectiondeletejob.h>

AddCollectionTask::AddCollectionTask(const ResourceStateInterface::Ptr &resource, QObject *parent)
    : ResourceTask(DeferIfNoSession, resource, parent)
{
}

AddCollectionTask::~AddCollectionTask()
{
}

void AddCollectionTask::doStart(KIMAP::Session *session)
{
    if (parentCollection().remoteId().isEmpty()) {
        qCWarning(IMAPRESOURCE_LOG) << "Parent collection has no remote id, aborting." << collection().name() << parentCollection().name();
        emitError(i18n("Cannot add IMAP folder '%1' for a non-existing parent folder '%2'.",
                       collection().name(),
                       parentCollection().name()));
        changeProcessed();
        return;
    }

    const QChar separator = separatorCharacter();
    m_pendingJobs = 0;
    m_session = session;
    m_collection = collection();
    m_collection.setName(m_collection.name().remove(separator));
    m_collection.setRemoteId(separator + m_collection.name());

    QString newMailBox = mailBoxForCollection(parentCollection());

    if (!newMailBox.isEmpty()) {
        newMailBox += separator;
    }

    newMailBox += m_collection.name();

    qCDebug(IMAPRESOURCE_LOG) << "New folder: " << newMailBox;

    KIMAP::CreateJob *job = new KIMAP::CreateJob(session);
    job->setMailBox(newMailBox);

    connect(job, &KIMAP::CreateJob::result, this, &AddCollectionTask::onCreateDone);

    job->start();
}

void AddCollectionTask::onCreateDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << "Failed to create folder on server: " << job->errorString();
        emitError(i18n("Failed to create the folder '%1' on the IMAP server. ",
                       m_collection.name()));
        cancelTask(job->errorString());
    } else {
        // Automatically subscribe to newly created mailbox
        KIMAP::CreateJob *create = static_cast<KIMAP::CreateJob *>(job);

        KIMAP::SubscribeJob *subscribe = new KIMAP::SubscribeJob(create->session());
        subscribe->setMailBox(create->mailBox());

        connect(subscribe, &KIMAP::SubscribeJob::result, this, &AddCollectionTask::onSubscribeDone);

        subscribe->start();
    }
}

void AddCollectionTask::onSubscribeDone(KJob *job)
{
    if (job->error() && isSubscriptionEnabled()) {
        qCWarning(IMAPRESOURCE_LOG) << "Failed to subscribe to the new folder: " << job->errorString();
        emitWarning(i18n("Failed to subscribe to the folder '%1' on the IMAP server. "
                         "It will disappear on next sync. Use the subscription dialog to overcome that",
                         m_collection.name()));
    }

    const Akonadi::CollectionAnnotationsAttribute *attribute = m_collection.attribute<Akonadi::CollectionAnnotationsAttribute>();
    if (!attribute || !serverSupportsAnnotations()) {
        // we are finished
        changeCommitted(m_collection);
        synchronizeCollectionTree();
        return;
    }

    QMapIterator<QByteArray, QByteArray> i(attribute->annotations());
    while (i.hasNext()) {
        i.next();
        KIMAP::SetMetaDataJob *job = new KIMAP::SetMetaDataJob(m_session);
        if (serverCapabilities().contains(QLatin1String("METADATA"))) {
            job->setServerCapability(KIMAP::MetaDataJobBase::Metadata);
        } else {
            job->setServerCapability(KIMAP::MetaDataJobBase::Annotatemore);
        }
        job->setMailBox(mailBoxForCollection(m_collection));

        if (!i.key().startsWith("/shared") && !i.key().startsWith("/private")) {
            //Support for legacy annotations that don't include the prefix
            job->addMetaData(QByteArray("/shared") + i.key(), i.value());
        } else {
            job->addMetaData(i.key(), i.value());
        }

        connect(job, &KIMAP::SetMetaDataJob::result, this, &AddCollectionTask::onSetMetaDataDone);

        m_pendingJobs++;

        job->start();
    }
}

void AddCollectionTask::onSetMetaDataDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << "Failed to write annotations: " << job->errorString();
        emitWarning(i18n("Failed to write some annotations for '%1' on the IMAP server. %2",
                         collection().name(), job->errorText()));
    }

    m_pendingJobs--;

    if (m_pendingJobs == 0) {
        changeCommitted(m_collection);
    }
}
