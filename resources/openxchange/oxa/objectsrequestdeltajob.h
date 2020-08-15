/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
