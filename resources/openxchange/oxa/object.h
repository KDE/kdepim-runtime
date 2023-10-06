/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "folder.h"

#include <KCalendarCore/Incidence>
#include <KContacts/Addressee>
#include <KContacts/ContactGroup>

#include <QList>
#include <QString>

namespace OXA
{
class Object
{
public:
    /**
     * Describes a list of objects.
     */
    using List = QList<Object>;

    /**
     * Describes the status of the object.
     */
    enum ObjectStatus {
        Created, ///< The object has been created or modified.
        Deleted ///< The object has been deleted.
    };

    Object();

    void setObjectStatus(ObjectStatus status);
    [[nodiscard]] ObjectStatus objectStatus() const;

    void setObjectId(qlonglong id);
    [[nodiscard]] qlonglong objectId() const;

    void setFolderId(qlonglong id);
    [[nodiscard]] qlonglong folderId() const;

    void setLastModified(const QString &timeStamp);
    [[nodiscard]] QString lastModified() const;

    void setModule(Folder::Module module);
    [[nodiscard]] Folder::Module module() const;

    void setContact(const KContacts::Addressee &contact);
    [[nodiscard]] KContacts::Addressee contact() const;

    void setContactGroup(const KContacts::ContactGroup &group);
    [[nodiscard]] KContacts::ContactGroup contactGroup() const;

    void setEvent(const KCalendarCore::Incidence::Ptr &event);
    [[nodiscard]] KCalendarCore::Incidence::Ptr event() const;

    void setTask(const KCalendarCore::Incidence::Ptr &task);
    [[nodiscard]] KCalendarCore::Incidence::Ptr task() const;

private:
    ObjectStatus mObjectStatus = Created;
    qlonglong mObjectId = -1;
    qlonglong mFolderId = -1;
    QString mLastModified;
    Folder::Module mModule;
    KContacts::Addressee mContact;
    KContacts::ContactGroup mContactGroup;
    KCalendarCore::Incidence::Ptr mEvent;
    KCalendarCore::Incidence::Ptr mTask;
};
}

Q_DECLARE_TYPEINFO(OXA::Object, Q_RELOCATABLE_TYPE);
