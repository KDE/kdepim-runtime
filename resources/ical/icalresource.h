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

#include "singlefileresource.h"
#include "settings.h"

namespace KCal {
  class AssignmentVisitor;
  class CalendarLocal;
  class IncidenceBase;
}

namespace Akonadi {
  class KCalMimeTypeVisitor;
}

class ICalResource : public Akonadi::SingleFileResource<Settings>
{
  Q_OBJECT

  public:
    ICalResource( const QString &id );
    ~ICalResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    bool readFromFile( const QString &fileName );
    bool writeToFile( const QString &fileName );
    virtual void aboutToQuit();
    bool isNotesResource() const;

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

  private:
    KCal::CalendarLocal *mCalendar;
    Akonadi::KCalMimeTypeVisitor *mMimeVisitor;
    KCal::AssignmentVisitor *mIncidenceAssigner;
};

#endif
