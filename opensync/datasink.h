/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef DATASINK_H
#define DATASINK_H

#include "sinkbase.h"

#include <akonadi/collection.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemfetchscope.h>

#include <opensync/opensync.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>

#include <boost/shared_ptr.hpp>

using namespace Akonadi;

/**
 * Base class for data sink classes, dealing with the type-independent stuff.
 */
class DataSink : public SinkBase
{
  Q_OBJECT
  public:
    DataSink( int features );
    ~DataSink();

    virtual bool initialize( OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncObjTypeSink *sink, OSyncError **error );

    virtual void getChanges();
    virtual void commit( OSyncChange *change );
    virtual void syncDone();

  public slots:
    void slotGetChangesFinished( KJob * );
    void slotItemsReceived( const Akonadi::Item::List & );

  protected:
    /**
     * Returns the collection we are supposed to sync with.
     */
    Akonadi::Collection collection() const;

    /**
     * This reports the change back to opensync
    */
    void reportChange( const Item& item, const QString& format );

    // in case you're writing a new datasink, you have to reimplement the following methods.

    /**
     * Reimplement in subclass to provide data about changes to opensync. Note, that you must call DataSink::reportChange( Item, QString, int ) after you have organized data to be send to opensync.
     */
    virtual void reportChange( const Item & item ) = 0;

    /**
     * Creates a new item based on the data given by opensync.
     */
    virtual int createItem( OSyncData *data ) = 0;
    /**
     * Modified an item based on the data given by opensync.
     */
    virtual int modifyItem( OSyncData *data ) = 0;
    /**
     * Deletes an item based on the data given by opensync.
     */
    //virtual void deleteItem( OSyncData *data );

  private:
    OSyncHashTable *m_hashtable;
};

#endif
