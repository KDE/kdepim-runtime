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

#ifndef OXA_FOLDERDELETEJOB_H
#define OXA_FOLDERDELETEJOB_H

#include "folder.h"

#include <KJob>

namespace OXA {
/**
 * @short A job that deletes a folder on the OX server.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class FolderDeleteJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Creates a new folder delete job.
     *
     * @param folder The folder to delete.
     * @param parent The parent object.
     *
     * @note The folder needs the objectId, folderId and lastModified property set.
     */
    explicit FolderDeleteJob(const Folder &folder, QObject *parent = nullptr);

    /**
     * Starts the job.
     */
    void start() override;

private Q_SLOTS:
    void davJobFinished(KJob *);

private:
    Folder mFolder;
};
}

#endif
