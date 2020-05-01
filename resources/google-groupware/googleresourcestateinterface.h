/*
    Copyright (c) 2020 Igor Poboiko <igor.poboiko@gmail.com>

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

#ifndef GOOGLERESOURCESTATEINTERFACE_H
#define GOOGLERESOURCESTATEINTERFACE_H

#include "resourcestateinterface.h"

namespace KGAPI2 {
class Job;
}

/**
 * This is a ResourceStateInterface with some specific for Google Resource bits
 */
class GoogleResourceStateInterface : public ResourceStateInterface
{
public:
    /**
     * Returns whether the resource is operational (i.e. account is configured)
     */
    virtual bool canPerformTask() = 0;

    /**
     * Handles an error (if any) for a job. It includes cancelling a task
     * (if there was an error), or retrying to authenticate (if necessary)
     */
    virtual bool handleError(KGAPI2::Job *job, bool _cancelTask = true) = 0;

    /**
     * Each handler use this to report that it has finished collection fetching
     */
    virtual void collectionsRetrievedFromHandler(const Akonadi::Collection::List &collections) = 0;

    /**
     * FreeBusy
     */
    virtual void freeBusyRetrieved(const QString &email, const QString &freeBusy, bool success, const QString &errorText = QString()) = 0;
    virtual void handlesFreeBusy(const QString &email, bool handles) = 0;

    virtual void scheduleCustomTask(QObject *receiver, const char *method, const QVariant &argument) = 0;
};

#endif // GOOGLERESOURCESTATEINTERFACE_H
