/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "folderdeletejob.h"

#include "davmanager.h"
#include "davutils.h"
#include "oxutils.h"

#include <kio/davjob.h>
#include <kio_version.h>
using namespace OXA;

FolderDeleteJob::FolderDeleteJob(const Folder &folder, QObject *parent)
    : KJob(parent)
    , mFolder(folder)
{
}

void FolderDeleteJob::start()
{
    QDomDocument document;
    QDomElement propertyupdate = DAVUtils::addDavElement(document, document, QStringLiteral("propertyupdate"));
    QDomElement set = DAVUtils::addDavElement(document, propertyupdate, QStringLiteral("set"));
    QDomElement prop = DAVUtils::addDavElement(document, set, QStringLiteral("prop"));
    DAVUtils::addOxElement(document, prop, QStringLiteral("object_id"), OXUtils::writeNumber(mFolder.objectId()));
    DAVUtils::addOxElement(document, prop, QStringLiteral("method"), OXUtils::writeString(QStringLiteral("DELETE")));
    DAVUtils::addOxElement(document, prop, QStringLiteral("last_modified"), OXUtils::writeString(mFolder.lastModified()));

    const QString path = QStringLiteral("/servlet/webdav.folders");

    KIO::DavJob *job = DavManager::self()->createPatchJob(path, document);
    connect(job, &KIO::DavJob::result, this, &FolderDeleteJob::davJobFinished);
}

void FolderDeleteJob::davJobFinished(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    auto davJob = qobject_cast<KIO::DavJob *>(job);

#if KIO_VERSION > QT_VERSION_CHECK(5, 85, 0)
    const QByteArray ba = davJob->responseData();
    QDomDocument document;
    document.setContent(ba);
#else
    const QDomDocument document = davJob->response();
#endif
    QString errorText, errorStatus;
    if (DAVUtils::davErrorOccurred(document, errorText, errorStatus)) {
        setError(UserDefinedError);
        setErrorText(errorText);
        emitResult();
        return;
    }

    emitResult();
}
