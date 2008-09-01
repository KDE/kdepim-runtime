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

#include <opensync/opensync.h>
#include <opensync/opensync-plugin.h>

/**
 * Base class for data sink classes, dealing with the type-independent stuff.
 */
class DataSink : public SinkBase
{
  Q_OBJECT
  public:
    DataSink();

    bool initialize( OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncObjTypeSink *sink, OSyncError **error );

    void getChanges();
    void commit( OSyncChange *change );
    void syncDone();

  protected:
    /**
     * Returns the collection we are supposed to sync with.
     */
    Akonadi::Collection collection() const;
};

#endif
