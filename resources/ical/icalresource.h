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

#ifndef ICALRESOURCE_H
#define ICALRESOURCE_H

#include <akonadi/resourcebase.h>

namespace KCal {
  class CalendarLocal;
}

class KCalMimeTypeVisitor;

class ICalResource : public Akonadi::ResourceBase
{
  Q_OBJECT

  public:
    ICalResource( const QString &id );
    ~ICalResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col, const QStringList &parts );
    bool retrieveItem( const Akonadi::Item &item, const QStringList &parts );

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QStringList &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

  private:
    void loadFile();

  private:
    KCal::CalendarLocal *mCalendar;
    KCalMimeTypeVisitor *mMimeVisitor;
};

#endif
