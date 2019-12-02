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

#ifndef OXA_OBJECTSREQUESTJOB_H
#define OXA_OBJECTSREQUESTJOB_H

#include <KJob>

#include "folder.h"
#include "object.h"

namespace OXA {
class ObjectsRequestJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Describes the mode of the request job.
     */
    enum Mode {
        Modified,  ///< Fetches all new and modified objects
        Deleted    ///< Fetches all deleted objects
    };

    /**
     * Creates a new objects request job.
     *
     * @param folder The folder the objects shall be request from.
     * @param lastSync The timestamp of the last sync. Only added, modified or deleted objects
     *                 after this date will be requested. 0 will request all available objects.
     * @param mode The mode of objects to request.
     * @param parent The parent object.
     */
    explicit ObjectsRequestJob(const Folder &folder, qulonglong lastSync = 0, Mode mode = Modified, QObject *parent = nullptr);

    void start() override;

    Object::List objects() const;

private Q_SLOTS:
    void davJobFinished(KJob *);

private:
    Folder mFolder;
    qulonglong mLastSync;
    Mode mMode;
    Object::List mObjects;
};
}

#endif
