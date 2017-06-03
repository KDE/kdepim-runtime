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

#include "objectsrequestjob.h"

#include "davmanager.h"
#include "davutils.h"
#include "objectutils.h"
#include "oxutils.h"

#include <kio/davjob.h>

#include <QDebug>

using namespace OXA;

ObjectsRequestJob::ObjectsRequestJob(const Folder &folder, qulonglong lastSync, Mode mode, QObject *parent)
    : KJob(parent), mFolder(folder), mLastSync(lastSync), mMode(mode)
{
}

void ObjectsRequestJob::start()
{
    QDomDocument document;
    QDomElement multistatus = DAVUtils::addDavElement(document, document, QStringLiteral("multistatus"));
    QDomElement prop = DAVUtils::addDavElement(document, multistatus, QStringLiteral("prop"));
    DAVUtils::addOxElement(document, prop, QStringLiteral("folder_id"), OXUtils::writeNumber(mFolder.objectId()));
    DAVUtils::addOxElement(document, prop, QStringLiteral("lastsync"), OXUtils::writeNumber(mLastSync));
    if (mMode == Modified) {
        DAVUtils::addOxElement(document, prop, QStringLiteral("objectmode"), QStringLiteral("MODIFIED"));
    } else {
        DAVUtils::addOxElement(document, prop, QStringLiteral("objectmode"), QStringLiteral("DELETED"));
    }

    const QString path = ObjectUtils::davPath(mFolder.module());

    KIO::DavJob *job = DavManager::self()->createFindJob(path, document);
    connect(job, &KIO::DavJob::result, this, &ObjectsRequestJob::davJobFinished);
}

Object::List ObjectsRequestJob::objects() const
{
    return mObjects;
}

void ObjectsRequestJob::davJobFinished(KJob *job)
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
    QDomElement response = multistatus.firstChildElement(QStringLiteral("response"));
    while (!response.isNull()) {
        const QDomNodeList props = response.elementsByTagName(QStringLiteral("prop"));
        const QDomElement prop = props.at(0).toElement();
        mObjects.append(ObjectUtils::parseObject(prop, mFolder.module()));
        response = response.nextSiblingElement();
    }

    emitResult();
}

