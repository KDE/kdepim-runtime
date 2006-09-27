/*
    Copyright (c) 2006 Till Adam <adam@kde.org>

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

#ifndef MAILDIR_H
#define MAILDIR_H

#include <resourcebase.h>
#include <QtDBus/QDBusMessage>

namespace KCal {
  class CalendarLocal;
}

namespace Akonadi {
class Job;
}

class ICalResource : public Akonadi::ResourceBase
{
  Q_OBJECT

  public:
    ICalResource( const QString &id );
    ~ICalResource();

    void setParameters(const QByteArray &path, const QByteArray &filename, const QByteArray &mimetype );

  public Q_SLOTS:
    virtual bool requestItemDelivery( const QString & uid,const QString &remoteId, const QString & collection, int type, const QDBusMessage &msg );
    virtual void synchronize();

  protected:
    virtual void aboutToQuit();

  private:
    KCal::CalendarLocal *mCalendar;

};

#endif
