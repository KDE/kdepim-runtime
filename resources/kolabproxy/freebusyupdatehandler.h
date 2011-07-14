/*
    Copyright (c) 2011 Tobias Koenig <tokoe@kde.org>

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

#ifndef FREEBUSYUPDATEHANDLER_H
#define FREEBUSYUPDATEHANDLER_H

#include <QtCore/QObject>
#include <QtCore/QSet>

class KJob;
class KUrl;
class QTimer;

/**
 * A class that triggers the update of freebusy lists on the Kolab server
 * whenever a calendar IMAP folder has been changed.
 */
class FreeBusyUpdateHandler : public QObject
{
  Q_OBJECT

  public:
    FreeBusyUpdateHandler( QObject *parent = 0 );
    ~FreeBusyUpdateHandler();

    void updateFolder( const QString &path, const QString &userName, const QString &password, const QString &host );

  private Q_SLOTS:
    void timeout();
    void slotFreeBusyTriggerResult( KJob* );

  private:
    QSet<KUrl> mUrls;
    QTimer *mTimer;
};

#endif
