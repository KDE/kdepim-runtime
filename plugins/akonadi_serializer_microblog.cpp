/*
    Copyright (C) 2009 Omat Holding B.V. <info@omat.nl>

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

#include "akonadi_serializer_microblog.h"

#include <microblog/statusitem.h>
#include <akonadi/item.h>

#include <QtCore/qplugin.h>

using namespace Akonadi;
using namespace Microblog;

bool SerializerPluginmicroblog::deserialize( Item& item, const QByteArray& label, QIODevice& data, int version )
{
  Q_UNUSED( version );
 
  if ( label != Item::FullPayload )
    return false;
 
  StatusItem status;
  status.setData( data.readAll() );
  
  item.setPayload<StatusItem>( status );
 
  return true;
}

void SerializerPluginmicroblog::serialize( const Item& item, const QByteArray& label, QIODevice& data, int &version )
{
  Q_UNUSED( version );

  if ( label != Item::FullPayload || !item.hasPayload<StatusItem>() )
    return;
 
  const StatusItem status = item.payload<StatusItem>();
  data.write( status.data() );
}

QSet<QByteArray> SerializerPluginmicroblog::parts( const Item &item ) const
{
  // only need to reimplement this when implementing partial serialization
  // i.e. when using the "label" parameter of the other two methods
  return ItemSerializerPlugin::parts( item );
}

Q_EXPORT_PLUGIN2( akonadi_serializer_microblog, Akonadi::SerializerPluginmicroblog )

#include "akonadi_serializer_microblog.moc"
