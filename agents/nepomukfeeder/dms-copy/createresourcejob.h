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

#ifndef CREATERESOURCEJOB_H
#define CREATERESOURCEJOB_H

#include <KJob>

#include <QtCore/QHash>
#include <QtCore/QUrl>

#include "nepomukdatamanagement_export.h"

class KComponentData;
class QDBusPendingCallWatcher;

namespace Nepomuk {
/**
 * \class CreateResourceJob createresourcejob.h Nepomuk/CreateResourceJob
 *
 * \brief Job returned by Nepomuk::createResource().
 *
 * Access the result through the resources() method in the slot connected
 * to the KJOb::result() signal.
 *
 * \author Sebastian Trueg <trueg@kde.org>
 */
class NEPOMUK_DATA_MANAGEMENT_EXPORT CreateResourceJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Destructor. The job does delete itself as soon
     * as it is done.
     */
    ~CreateResourceJob();

    /**
     * The returned resource URI.
     *
     * Access the result in a slot connected to the KJob::result()
     * signal.
     */
    QUrl resourceUri() const;

private Q_SLOTS:
    void slotDBusCallFinished(QDBusPendingCallWatcher *watcher);

private:
    CreateResourceJob(const QList<QUrl>& types,
                      const QString& label,
                      const QString& description,
                      const KComponentData& component);
    void start();

    class Private;
    Private* const d;

    friend Nepomuk::CreateResourceJob* Nepomuk::createResource(const QList<QUrl>&,
                                                               const QString&,
                                                               const QString&,
                                                               const KComponentData&);
};
}

#endif
