/*
    SPDX-FileCopyrightText: 2014 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Kevin Krammer <kevin.krammer@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kolabrelationresourcetask.h"
#include "kolabresource_debug.h"
#include "kolabhelpers.h"

#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/CollectionCreateJob>
#include <kimap/createjob.h>
#include <kimap/setmetadatajob.h>

#include <KLocalizedString>

KolabRelationResourceTask::KolabRelationResourceTask(const ResourceStateInterface::Ptr &resource, QObject *parent)
    : ResourceTask(DeferIfNoSession, resource, parent)
    , mImapSession(nullptr)
{
}

Akonadi::Collection KolabRelationResourceTask::relationCollection() const
{
    return mRelationCollection;
}

void KolabRelationResourceTask::doStart(KIMAP::Session *session)
{
    mImapSession = session;

    // need to find the configuration collection.

    Akonadi::Collection topLevelCollection;
    topLevelCollection.setRemoteId(rootRemoteId());
    topLevelCollection.setParentCollection(Akonadi::Collection::root());

    Akonadi::CollectionFetchJob *fetchJob = new Akonadi::CollectionFetchJob(topLevelCollection, Akonadi::CollectionFetchJob::Recursive);
    fetchJob->fetchScope().setResource(resourceState()->resourceIdentifier());
    fetchJob->fetchScope().setContentMimeTypes(QStringList() << KolabHelpers::getMimeType(Kolab::ConfigurationType));
    fetchJob->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
    fetchJob->fetchScope().setListFilter(Akonadi::CollectionFetchScope::NoFilter);
    connect(fetchJob, &KJob::result, this, &KolabRelationResourceTask::onCollectionFetchResult);
}

void KolabRelationResourceTask::onCollectionFetchResult(KJob *job)
{
    if (job->error() == 0) {
        Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);
        Q_ASSERT(fetchJob != nullptr);

        const Akonadi::Collection::List lstCols = fetchJob->collections();
        for (const Akonadi::Collection &collection : lstCols) {
            if (!collection.contentMimeTypes().contains(KolabHelpers::getMimeType(Kolab::ConfigurationType))) {
                // Skip parents of the actual Configuration folder
                continue;
            }
            const QString mailBox = mailBoxForCollection(collection);
            if (!mailBox.isEmpty()) {
                mRelationCollection = collection;
                startRelationTask(mImapSession);
                return;
            }
        }
    }

    qCDebug(KOLABRESOURCE_LOG) << "Couldn't find collection for relations, creating one.";

    const QChar separator = separatorCharacter();
    mRelationCollection = Akonadi::Collection();
    mRelationCollection.setName(QStringLiteral("Configuration"));
    mRelationCollection.setContentMimeTypes(QStringList() << KolabHelpers::getMimeType(Kolab::ConfigurationType));
    mRelationCollection.setRemoteId(separator + mRelationCollection.name());
    const QString newMailBox = QStringLiteral("Configuration");
    KIMAP::CreateJob *imapCreateJob = new KIMAP::CreateJob(mImapSession);
    imapCreateJob->setMailBox(newMailBox);
    connect(imapCreateJob, &KJob::result,
            this, &KolabRelationResourceTask::onCreateDone);
    imapCreateJob->start();
}

void KolabRelationResourceTask::onCreateDone(KJob *job)
{
    if (job->error()) {
        qCWarning(KOLABRESOURCE_LOG) << "Failed to create configuration folder: " << job->errorString();
        cancelTask(i18n("Failed to create configuration folder on server"));
        return;
    }

    KIMAP::SetMetaDataJob *setMetadataJob = new KIMAP::SetMetaDataJob(mImapSession);
    if (serverCapabilities().contains(QLatin1String("METADATA"))) {
        setMetadataJob->setServerCapability(KIMAP::MetaDataJobBase::Metadata);
    } else {
        setMetadataJob->setServerCapability(KIMAP::MetaDataJobBase::Annotatemore);
    }
    setMetadataJob->setMailBox(QStringLiteral("Configuration"));
    setMetadataJob->addMetaData(QByteArrayLiteral("/shared/vendor/kolab/folder-type"), QByteArrayLiteral("configuration.default"));
    connect(setMetadataJob, &KJob::result,
            this, &KolabRelationResourceTask::onSetMetaDataDone);
    setMetadataJob->start();
}

void KolabRelationResourceTask::onSetMetaDataDone(KJob *job)
{
    if (job->error()) {
        qCWarning(KOLABRESOURCE_LOG) << "Failed to write annotations: " << job->errorString();
        cancelTask(i18n("Failed to write some annotations for '%1' on the IMAP server. %2",
                        collection().name(), job->errorText()));
        return;
    }

    Akonadi::CollectionCreateJob *createJob = new Akonadi::CollectionCreateJob(mRelationCollection, this);
    connect(createJob, &KJob::result, this, &KolabRelationResourceTask::onLocalCreateDone);
}

void KolabRelationResourceTask::onLocalCreateDone(KJob *job)
{
    if (job->error()) {
        qCWarning(KOLABRESOURCE_LOG) << "Failed to create local folder: " << job->errorString();
        cancelTask(i18n("Failed to create configuration folder"));
        return;
    }
    mRelationCollection = static_cast<Akonadi::CollectionCreateJob *>(job)->collection();
    startRelationTask(mImapSession);
}
