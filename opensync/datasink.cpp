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

#include "datasink.h"

#include <akonadi/itemfetchjob.h>

#include <KDebug>
#include <KLocale>
#include <KUrl>

using namespace Akonadi;

DataSink::DataSink() :
    SinkBase( GetChanges )
{
}

bool DataSink::initialize(OSyncPlugin * plugin, OSyncPluginInfo * info, OSyncObjTypeSink *sink, OSyncError ** error)
{
  Q_UNUSED( plugin );
  Q_UNUSED( info );
  Q_UNUSED( error );
  wrapSink( sink );
  return true;
}

Akonadi::Collection DataSink::collection() const
{
  OSyncPluginConfig *config = osync_plugin_info_get_config( pluginInfo() );
  Q_ASSERT( config );

  const char *objtype = osync_objtype_sink_get_name( sink() );

  OSyncPluginResource *res = osync_plugin_config_find_active_resource( config, objtype );
  if ( !res ) {
    error( OSYNC_ERROR_MISCONFIGURATION, i18n("No active resource for type \"%1\" found", objtype ) );
    return Collection();
  }

  const KUrl url = KUrl( osync_plugin_resource_get_url( res ) );
  if ( url.isEmpty() ) {
    error( OSYNC_ERROR_MISCONFIGURATION, i18n("Url for object type \"%1\" is not configured.", objtype ) );
    return Collection();
  }

  return Collection::fromUrl( url );
}

void DataSink::getChanges()
{
  Collection col = collection();
  if ( !col.isValid() )
    return;

  // TODO changes since when??

  ItemFetchJob *job = new ItemFetchJob( col );
  // FIXME give me a real eventloop please!
  if ( !job->exec() ) {
    error( OSYNC_ERROR_IO_ERROR, job->errorText() );
    return;
  }

  Item::List items = job->items();
  kDebug() << "retrieved" << items.count() << "items";

  // TODO and now what do we do here?

  success();
}
