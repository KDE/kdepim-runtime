/*
    This file is part of oxaccess.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#ifndef OXA_OBJECTSREQUESTDELTAJOB_H
#define OXA_OBJECTSREQUESTDELTAJOB_H

#include <KJob>

#include "object.h"

namespace OXA {
/**
 * @short A job that requests the delta for objects changes from the OX server.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class ObjectsRequestDeltaJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Creates a new objects request delta job.
     *
     * @param folder The folder the objects shall be request from.
     * @param lastSync The timestamp of the last sync. Only added, modified and deleted objects
     *                 after this date will be requested. 0 will request all available objects.
     * @param parent The parent object.
     */
    ObjectsRequestDeltaJob(const Folder &folder, qulonglong lastSync, QObject *parent = nullptr);

    /**
     * Starts the job.
     */
    void start() override;

    /**
     * Returns the list of all added and modified objects.
     */
    Object::List modifiedObjects() const;

    /**
     * Returns the list of all deleted objects.
     */
    Object::List deletedObjects() const;

private Q_SLOTS:
    void fetchModifiedJobFinished(KJob *);
    void fetchDeletedJobFinished(KJob *);

private:
    Folder mFolder;
    qulonglong mLastSync;
    Object::List mModifiedObjects;
    Object::List mDeletedObjects;
    int mJobFinishedCount;
};
}

#endif
