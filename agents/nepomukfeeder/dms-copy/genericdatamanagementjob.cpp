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

#include "genericdatamanagementjob_p.h"
#include "datamanagementinterface.h"
#include "dbustypes.h"
#include "kdbusconnectionpool.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusPendingReply>
#include <QtDBus/QDBusPendingCallWatcher>

#include <QtCore/QVariant>
#include <QtCore/QHash>

#include <KDebug>

Nepomuk::GenericDataManagementJob::GenericDataManagementJob(const char *methodName,
                                                            QGenericArgument val0,
                                                            QGenericArgument val1,
                                                            QGenericArgument val2,
                                                            QGenericArgument val3,
                                                            QGenericArgument val4,
                                                            QGenericArgument val5)
    : KJob(0)
{
    // DBus types necessary for storeResources
    DBus::registerDBusTypes();

    org::kde::nepomuk::DataManagement dms(QLatin1String(DMS_DBUS_SERVICE),
                                          QLatin1String("/datamanagement"),
                                          KDBusConnectionPool::threadConnection());
    QDBusPendingReply<> reply;
    QMetaObject::invokeMethod(&dms,
                              methodName,
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QDBusPendingReply<> , reply),
                              val0,
                              val1,
                              val2,
                              val3,
                              val4,
                              val5);
    QDBusPendingCallWatcher* dbusCallWatcher = new QDBusPendingCallWatcher(reply);
    connect(dbusCallWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(slotDBusCallFinished(QDBusPendingCallWatcher*)));
}

Nepomuk::GenericDataManagementJob::~GenericDataManagementJob()
{
}

void Nepomuk::GenericDataManagementJob::start()
{
    // do nothing
}

void Nepomuk::GenericDataManagementJob::slotDBusCallFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<> reply = *watcher;
    if (reply.isError()) {
        QDBusError error = reply.error();
        kDebug() << error;
        setError(int(error.type()));
        setErrorText(error.message());
    }
    delete watcher;
    emitResult();
}

#include "genericdatamanagementjob_p.moc"
