/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "foldersrequestjob.h"

#include "davmanager.h"
#include "davutils.h"
#include "folderutils.h"
#include "oxutils.h"

#include <kio/davjob.h>

#include <QtCore/QDebug>

using namespace OXA;

FoldersRequestJob::FoldersRequestJob(qulonglong lastSync, Mode mode, QObject *parent)
    : KJob(parent), mLastSync(lastSync), mMode(mode)
{
}

void FoldersRequestJob::start()
{
    QDomDocument document;
    QDomElement multistatus = DAVUtils::addDavElement(document, document, QStringLiteral("multistatus"));
    QDomElement prop = DAVUtils::addDavElement(document, multistatus, QStringLiteral("prop"));
    DAVUtils::addOxElement(document, prop, QStringLiteral("lastsync"), OXUtils::writeNumber(mLastSync));
    if (mMode == Modified) {
        DAVUtils::addOxElement(document, prop, QStringLiteral("objectmode"), QStringLiteral("MODIFIED"));
    } else {
        DAVUtils::addOxElement(document, prop, QStringLiteral("objectmode"), QStringLiteral("DELETED"));
    }

    const QString path = QLatin1String("/servlet/webdav.folders");

    KIO::DavJob *job = DavManager::self()->createFindJob(path, document);
    connect(job, &KIO::DavJob::result, this, &FoldersRequestJob::davJobFinished);
}

Folder::List FoldersRequestJob::folders() const
{
    return mFolders;
}

void FoldersRequestJob::davJobFinished(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    KIO::DavJob *davJob = qobject_cast<KIO::DavJob *>(job);

    const QDomDocument document = davJob->response();

    QString errorText, errorStatus;
    if (DAVUtils::davErrorOccurred(document, errorText, errorStatus)) {
        setError(UserDefinedError);
        setErrorText(errorText);
        emitResult();
        return;
    }

    QDomElement multistatus = document.documentElement();
    QDomElement response = multistatus.firstChildElement(QLatin1String("response"));
    while (!response.isNull()) {
        const QDomNodeList props = response.elementsByTagName("prop");
        const QDomElement prop = props.at(0).toElement();
        mFolders.append(FolderUtils::parseFolder(prop));
        response = response.nextSiblingElement();
    }

    emitResult();
}

