/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "davpushregistercollectionsjob.h"

#include "davresource_debug.h"

#include "attributes/davpushattribute.h"
#include "settings.h"

#include <Akonadi/CollectionModifyJob>
#include <KDAV/DavPushRegistrationJob>
#include <QDateTime>
#include <QVariant>

DavPushRegisterCollectionJobs::DavPushRegisterCollectionJobs(const QList<Akonadi::Collection> &collections,
                                                             const KDAV::DavPushRegistration &davPushRegistration,
                                                             Settings *settings,
                                                             QObject *parent)
    : KCompositeJob(parent)
    , mCollections(collections)
    , mDavPushRegistration(davPushRegistration)
    , mSettings(settings)
{
    for (const auto &collection : mCollections) {
        Q_ASSERT(collection.isValid());
        Q_ASSERT(collection.hasAttribute<DavPushAttribute>());
        Q_ASSERT(!collection.attribute<DavPushAttribute>()->topic().isEmpty());
    }
}

void DavPushRegisterCollectionJobs::start()
{
    for (const auto &collection : mCollections) {
        auto davUrl = mSettings->davUrlFromCollectionUrl(collection.remoteId());
        auto *registrationJob = new KDAV::DavPushRegistrationJob(davUrl, mDavPushRegistration);
        registrationJob->setProperty("collection", QVariant::fromValue(collection));
        addSubjob(registrationJob);
        registrationJob->start();
    }

    if (!hasSubjobs()) {
        emitResult();
    }
}

void DavPushRegisterCollectionJobs::slotResult(KJob *job)
{
    removeSubjob(job);

    if (const auto *regJob = qobject_cast<KDAV::DavPushRegistrationJob *>(job); regJob) {
        auto collection = job->property("collection").value<Akonadi::Collection>();
        if (job->error()) {
            qCWarning(DAVRESOURCE_LOG) << "DavPushRegisterCollectionJobs: error registering DAV push" << collection.remoteId() << ":" << job->errorText();
            if (!error()) {
                setError(job->error());
                setErrorText(job->errorText());
            }
        } else {
            auto updateCol = Akonadi::Collection();
            updateCol.setId(collection.id());
            auto *attr = static_cast<DavPushAttribute *>(collection.attribute<DavPushAttribute>(Akonadi::Collection::AddIfMissing)->clone());
            attr->setExpirationDate(regJob->expirationDate());
            attr->setRegistrationUrl(regJob->registrationUrl());
            updateCol.addAttribute(attr);

            auto *modifyJob = new Akonadi::CollectionModifyJob(updateCol);
            addSubjob(modifyJob);
            modifyJob->start();
        }
    } else if (const auto *updateJob = qobject_cast<Akonadi::CollectionModifyJob *>(job); updateJob) {
        const auto collection = updateJob->collection();
        if (job->error()) {
            qCWarning(DAVRESOURCE_LOG) << "DavPushRegisterCollectionJobs: error updating collection" << collection.remoteId() << ":" << job->errorText();
            if (!error()) {
                setError(job->error());
                setErrorText(job->errorText());
            }
        }
    }

    if (!hasSubjobs()) {
        qCDebug(DAVRESOURCE_LOG) << "DavPushRegisterCollectionJobs: finished registering and updating";
        emitResult();
    }
}
