/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GENERICDATAMANAGEMENTJOB_H
#define GENERICDATAMANAGEMENTJOB_H

#include <QtGlobal>

#ifndef NDEBUG
#define DMS_DBUS_SERVICE (qgetenv("NEPOMUK_FAKE_DMS_DBUS_SERVICE").isEmpty() ? "org.kde.nepomuk.DataManagement" : qgetenv("NEPOMUK_FAKE_DMS_DBUS_SERVICE").constData())
#else
#define DMS_DBUS_SERVICE "org.kde.nepomuk.DataManagement"
#endif

#include <KJob>

class QDBusPendingCallWatcher;

namespace Nepomuk {
class GenericDataManagementJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Start any Data Management Service method with a void
     * return type.
     */
    GenericDataManagementJob(const char* methodName,
                             QGenericArgument val0,
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument());
    ~GenericDataManagementJob();

    /// does nothing, we do all in the constructor - it simply needs to be implemented
    void start();

private Q_SLOTS:
    void slotDBusCallFinished(QDBusPendingCallWatcher*);
};
}

#endif
