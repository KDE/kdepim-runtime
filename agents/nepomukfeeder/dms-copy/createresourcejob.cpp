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

#include "createresourcejob.h"
#include "datamanagementinterface.h"
#include "dbustypes.h"
#include "genericdatamanagementjob_p.h"
#include "kdbusconnectionpool.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusPendingReply>
#include <QtDBus/QDBusPendingCallWatcher>

#include <QtCore/QVariant>
#include <QtCore/QUrl>

#include <KComponentData>
#include <KUrl>


class Nepomuk::CreateResourceJob::Private
{
public:
    QUrl m_resourceUri;
};

Nepomuk::CreateResourceJob::CreateResourceJob(const QList<QUrl>& types,
                                     const QString& label,
                                     const QString& description,
                                     const KComponentData& component)
    : KJob(0),
      d(new Private)
{
    org::kde::nepomuk::DataManagement dms(QLatin1String(DMS_DBUS_SERVICE),
                                          QLatin1String("/datamanagement"),
                                          KDBusConnectionPool::threadConnection());
    QDBusPendingCallWatcher* dbusCallWatcher
            = new QDBusPendingCallWatcher(dms.createResource(Nepomuk::DBus::convertUriList(types),
                                                             label,
                                                             description,
                                                             component.componentName()));
    connect(dbusCallWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(slotDBusCallFinished(QDBusPendingCallWatcher*)));
}

Nepomuk::CreateResourceJob::~CreateResourceJob()
{
    delete d;
}

void Nepomuk::CreateResourceJob::start()
{
    // do nothing, we do everything in the constructor
}

void Nepomuk::CreateResourceJob::slotDBusCallFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QString> reply = *watcher;
    if (reply.isError()) {
        QDBusError error = reply.error();
        setError(1);
        setErrorText(error.message());
    }
    else {
        d->m_resourceUri = KUrl(reply.value());
    }
    watcher->deleteLater();
    emitResult();
}

QUrl Nepomuk::CreateResourceJob::resourceUri() const
{
    return d->m_resourceUri;
}

#include "createresourcejob.moc"
