/*
    Copyright (C) 2010 Bertjan Broeksema <broeksema@kde.org>
    Copyright (C) 2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

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

#ifndef FREEBUSYMANAGER_P_H
#define FREEBUSYMANAGER_P_H

#include <QtCore/QObject>
#include <QtCore/QString>

#include <KJob>
#include <KUrl>

namespace KCal {
class FreeBusy;
}

namespace KIO {
class Job;
}

namespace Akonadi {

class FreeBusyManager;

class FreeBusyDownloadJob : public KJob
{
  Q_OBJECT
  public:
    explicit FreeBusyDownloadJob( const KUrl &url, QWidget *parentWidget = 0 );
    virtual ~FreeBusyDownloadJob();

    /// Implement KJob::start()
    virtual void start();

    KUrl url() const;
    QByteArray rawFreeBusyData() const;

  private slots:
    void slotData( KIO::Job *, const QByteArray &data );
    void slotResult( KJob * );

  private:
    KUrl mUrl;
    QByteArray mFreeBusyData;
    QWidget *mParent;
};

}

#endif // FREEBUSYMANAGER_P_H
