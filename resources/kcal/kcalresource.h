/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef KCALRESOURCE
#define KCALRESOURCE

#include <akonadi/resourcebase.h>

namespace KRES {
  template <class T> class Manager;
}

namespace KCal {
  class CalendarResources;
  class ResourceCalendar;
}

namespace Akonadi {
  class KCalMimeTypeVisitor;
}

class QTimer;

class KCalResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    KCalResource( const QString &id );
    virtual ~KCalResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

  private:
    KCal::CalendarResources *mCalendar;
    bool mLoaded;

    QString mLastError;

    QTimer *mDelayedUpdateTimer;

    Akonadi::KCalMimeTypeVisitor *mMimeVisitor;

  private:
    bool loadCalendar();

  private Q_SLOTS:
    void calendarError( const QString& message );
    void calendarChanged();
    void delayedUpdate();
};

#endif
