/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef GMAILRESOURCESTATE_H
#define GMAILRESOURCESTATE_H

#include <imap/resourcestate.h>

class GmailResource;
class ResourceTask;

class GmailResourceState : public ResourceState
{
public:
    explicit GmailResourceState (GmailResource *resource, const TaskArguments &arguments);

    void setCurrentTask(ResourceTask *task);
    ResourceTask *currentTask() const;

protected:
    virtual MessageHelper::Ptr messageHelper() const;

private:
    GmailResource *mResource;
    ResourceTask *mTask;
};

#endif // GMAILRESOURCESTATE_H
