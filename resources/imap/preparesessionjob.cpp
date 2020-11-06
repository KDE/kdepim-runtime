/*
    SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "preparesessionjob.h"

#include <KIMAP/CapabilitiesJob>
#include <KIMAP/IdJob>
#include <KIMAP/NamespaceJob>
#include <KIMAP/EnableJob>

PrepareSessionJob::PrepareSessionJob(KIMAP::Session *session, const QByteArray &clientId)
    : m_session(session)
    , m_clientId(clientId)
{}

void PrepareSessionJob::start()
{
    // First we need to retrieve list of capabilities
    auto *job = new KIMAP::CapabilitiesJob(m_session);
    addSubjob(job);
    job->start();
}

void PrepareSessionJob::slotResult(KJob *job)
{
    if (qobject_cast<KIMAP::CapabilitiesJob *>(job)) {
        capabilitiesJobDone(job);
    }

    removeSubjob(job);

    if (!hasSubjobs()) {
        emitResult();
    }
}

bool PrepareSessionJob::handleError(KJob *job, Error error)
{
    if (job->error()) {
        setError(error);
        setErrorText(job->errorString());
        return true;
    }

    return false;
}

void PrepareSessionJob::capabilitiesJobDone(KJob *job)
{
    if (handleError(job, CapabilitiesTestError)) {
        return;
    }

    auto *capsJob = qobject_cast<KIMAP::CapabilitiesJob *>(job);
    m_capabilities = capsJob->capabilities();

    for (const auto &cap : m_capabilities) {
        if (cap == QStringView{u"NAMESPACE"}) {
            auto *job = new KIMAP::NamespaceJob(m_session);
            connect(job, &KJob::result, this, &PrepareSessionJob::namespaceJobDone);
            addSubjob(job);
            job->start();
        } else if (cap == QStringView{u"ID"}) {
            auto *job = new KIMAP::IdJob(m_session);
            job->setField("name", m_clientId);
            connect(job, &KJob::result, this, &PrepareSessionJob::idJobDone);
            addSubjob(job);
            job->start();
        }
    }
}

void PrepareSessionJob::namespaceJobDone(KJob *job)
{
    if (handleError(job, NamespaceFetchError)) {
        return;
    }

    auto *nsJob = qobject_cast<KIMAP::NamespaceJob *>(job);

    m_personalNamespaces = nsJob->personalNamespaces();
    m_userNamespaces = nsJob->userNamespaces();
    m_sharedNamespaces = nsJob->sharedNamespaces();

    if (nsJob->containsEmptyNamespace()) {
        // When we got the empty namespace here, we assume that the other
        // ones can be freely ignored and that the server will give us all
        // the mailboxes if we list from the empty namespace itself...

        m_namespaces.clear();
    } else {
        // ... otherwise we assume that we have to list explicitly each
        // namespace

        m_namespaces = nsJob->personalNamespaces()
                       +nsJob->userNamespaces()
                       +nsJob->sharedNamespaces();
    }
}

void PrepareSessionJob::idJobDone(KJob *job)
{
    handleError(job, IdentificationError);
}

