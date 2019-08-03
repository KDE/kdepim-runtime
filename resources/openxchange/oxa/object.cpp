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

#include "object.h"

using namespace OXA;

Object::Object()
    : mObjectStatus(Created)
    , mObjectId(-1)
    , mFolderId(-1)
{
}

void Object::setObjectStatus(ObjectStatus status)
{
    mObjectStatus = status;
}

Object::ObjectStatus Object::objectStatus() const
{
    return mObjectStatus;
}

void Object::setObjectId(qlonglong id)
{
    mObjectId = id;
}

qlonglong Object::objectId() const
{
    return mObjectId;
}

void Object::setFolderId(qlonglong id)
{
    mFolderId = id;
}

qlonglong Object::folderId() const
{
    return mFolderId;
}

void Object::setLastModified(const QString &timeStamp)
{
    mLastModified = timeStamp;
}

QString Object::lastModified() const
{
    return mLastModified;
}

void Object::setModule(Folder::Module module)
{
    mModule = module;
}

Folder::Module Object::module() const
{
    return mModule;
}

void Object::setContact(const KContacts::Addressee &contact)
{
    mModule = Folder::Contacts;
    mContact = contact;
}

KContacts::Addressee Object::contact() const
{
    return mContact;
}

void Object::setContactGroup(const KContacts::ContactGroup &group)
{
    mModule = Folder::Contacts;
    mContactGroup = group;
}

KContacts::ContactGroup Object::contactGroup() const
{
    return mContactGroup;
}

void Object::setEvent(const KCalendarCore::Incidence::Ptr &event)
{
    mModule = Folder::Calendar;
    mEvent = event;
}

KCalendarCore::Incidence::Ptr Object::event() const
{
    return mEvent;
}

void Object::setTask(const KCalendarCore::Incidence::Ptr &task)
{
    mModule = Folder::Tasks;
    mTask = task;
}

KCalendarCore::Incidence::Ptr Object::task() const
{
    return mTask;
}
