/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#ifndef PIM_MONITOR_H
#define PIM_MONITOR_H

#include <libakonadi/job.h>
#include <QtCore/QObject>

namespace PIM {

class MonitorPrivate;

/**
  Monitors an Item or Collection for changes and emits signals if some
  of these objects are changed or removed or new ones are added to the storage
  backend.

  @todo: support un-monitoring
  @todo: distinguish between monitoring collection properties and collection content.
*/
class AKONADI_EXPORT Monitor : public QObject
{
  Q_OBJECT

  public:
    /**
      Creates a new monitor.
      @param parent The parent object.
    */
    Monitor( QObject *parent = 0 );

    /**
      Destroys this monitor.
    */
    virtual ~Monitor();

    /**
      Monitors the specified collection for changes.
      @param path The collection path.
      @param recursive If true also sub-collection will be monitored.
    */
    void monitorCollection( const QByteArray &path, bool recursive );

    /**
      Monitors the specified PIM Item for changes.
      @param ref The item references.
    */
    void monitorItem( const DataReference &ref );

    /**
      Monitors the specified resource for changes.
      @param resource The resource identifier.
    */
    void monitorResource( const QByteArray &resource );

    /**
      Monitor all items matching the specified mimetype.
      @param mimetype The mimetype.
    */
    void monitorMimeType( const QByteArray &mimetype );

  signals:
    /**
      Emitted if a monitored object has changed.
      @param ref Reference of the changed objects.
    */
    void itemChanged( const PIM::DataReference &ref );

    /**
      Emitted if a item has been added to a monitored collection.
      @param ref Reference of the added object.
    */
    void itemAdded( const PIM::DataReference &ref );

    /**
      Emitted if a monitored object has been removed.
      @param ref Reference of the removed object.
    */
    void itemRemoved( const PIM::DataReference &ref);

    /**
      Emitted if a monitored got a new child collection.
      @param path The path of the new collection.
    */
    void collectionAdded( const QByteArray &path );

    /**
      Emitted if a monitored collection changed (its properties, not its
      content).
      @param path The path of the modified collection.
    */
    void collectionChanged( const QByteArray &path );

    /**
      Emitted if a monitored collection has been removed.
      @param path The path of the rmeoved collection.
    */
    void collectionRemoved( const QByteArray &path );

  private:
    bool connectToNotificationManager();

  private slots:
    void slotItemChanged( const QByteArray &uid, const QByteArray &collection,
                          const QByteArray &mimetype, const QByteArray &resource );
    void slotItemAdded( const QByteArray &uid, const QByteArray &collection,
                        const QByteArray &mimetype, const QByteArray &resource );
    void slotItemRemoved( const QByteArray &uid, const QByteArray &collection,
                          const QByteArray &mimetype, const QByteArray &resource );
    void slotCollectionAdded( const QByteArray &path, const QByteArray &resource );
    void slotCollectionChanged( const QByteArray &path, const QByteArray &resource );
    void slotCollectionRemoved( const QByteArray &path, const QByteArray &resource );

  private:
    MonitorPrivate* d;
};

}

#endif
