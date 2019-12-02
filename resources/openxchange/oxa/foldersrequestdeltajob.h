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

#ifndef OXA_FOLDERSREQUESTDELTAJOB_H
#define OXA_FOLDERSREQUESTDELTAJOB_H

#include <KJob>

#include "folder.h"

namespace OXA {
/**
 * @short A job that requests the delta for folders changes from the OX server.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class FoldersRequestDeltaJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Creates a new folders request delta job.
     *
     * @param lastSync The timestamp of the last sync. Only added, modified and deleted folders
     *                 after this date will be requested. 0 will request all available folders.
     * @param parent The parent object.
     */
    explicit FoldersRequestDeltaJob(qulonglong lastSync, QObject *parent = nullptr);

    /**
     * Starts the job.
     */
    void start() override;

    /**
     * Returns the list of all added and modified folders.
     */
    Folder::List modifiedFolders() const;

    /**
     * Returns the list of all deleted folders.
     */
    Folder::List deletedFolders() const;

private Q_SLOTS:
    void fetchModifiedJobFinished(KJob *);
    void fetchDeletedJobFinished(KJob *);

private:
    qulonglong mLastSync;
    Folder::List mModifiedFolders;
    Folder::List mDeletedFolders;
    int mJobFinishedCount;
};
}

#endif
