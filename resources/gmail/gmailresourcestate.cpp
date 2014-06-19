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

#include "gmailresourcestate.h"
#include "gmailmessagehelper.h"
#include "gmailretrieveitemstask.h"
#include "gmailresource.h"

#include <imap/resourcetask.h>

GmailResourceState::GmailResourceState(GmailResource *resource, const TaskArguments &arguments)
    : ResourceState(resource, arguments)
    , mResource(resource)
    , mTask(0)
{
}

void GmailResourceState::setCurrentTask(ResourceTask *task)
{
    mTask = task;
}

ResourceTask *GmailResourceState::currentTask() const
{
    return mTask;
}

MessageHelper::Ptr GmailResourceState::messageHelper() const
{
    return MessageHelper::Ptr(new GmailMessageHelper(collection(),
                                                     qobject_cast<GmailRetrieveItemsTask*>(mTask)));
}
