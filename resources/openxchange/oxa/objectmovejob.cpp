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

#include "objectmovejob.h"

#include "davmanager.h"
#include "davutils.h"
#include "objectutils.h"
#include "oxutils.h"

#include <kio/davjob.h>

using namespace OXA;

ObjectMoveJob::ObjectMoveJob(const Object &object, const Folder &destinationFolder, QObject *parent)
    : KJob(parent), mObject(object), mDestinationFolder(destinationFolder)
{
}

void ObjectMoveJob::start()
{
    QDomDocument document;
    QDomElement propertyupdate = DAVUtils::addDavElement(document, document, QStringLiteral("propertyupdate"));
    QDomElement set = DAVUtils::addDavElement(document, propertyupdate, QStringLiteral("set"));
    QDomElement prop = DAVUtils::addDavElement(document, set, QStringLiteral("prop"));
    DAVUtils::addOxElement(document, prop, QStringLiteral("object_id"), OXUtils::writeNumber(mObject.objectId()));
    DAVUtils::addOxElement(document, prop, QStringLiteral("folder_id"), OXUtils::writeNumber(mObject.folderId()));
    DAVUtils::addOxElement(document, prop, QStringLiteral("last_modified"), OXUtils::writeString(mObject.lastModified()));
    DAVUtils::addOxElement(document, prop, QStringLiteral("folder"), OXUtils::writeNumber(mDestinationFolder.objectId()));

    const QString path = ObjectUtils::davPath(mObject.module());

    KIO::DavJob *job = DavManager::self()->createPatchJob(path, document);
    connect(job, &KIO::DavJob::result, this, &ObjectMoveJob::davJobFinished);
}

Object ObjectMoveJob::object() const
{
    return mObject;
}

void ObjectMoveJob::davJobFinished(KJob *job)
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
    const QDomNodeList props = response.elementsByTagName(QStringLiteral("prop"));
    const QDomElement prop = props.at(0).toElement();

    QDomElement element = prop.firstChildElement();
    while (!element.isNull()) {
        if (element.tagName() == QLatin1String("last_modified")) {
            mObject.setLastModified(OXUtils::readString(element.text()));
        }

        element = element.nextSiblingElement();
    }

    emitResult();
}

