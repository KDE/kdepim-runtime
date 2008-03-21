/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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

#include "akonadi_serializer_bookmark.h"

#include <QDebug>
#include <QMimeData>
#include <kbookmark.h>

#include <akonadi/item.h>


using namespace Akonadi;

bool SerializerPluginBookmark::deserialize( Item& item, const QString& label, QIODevice& data )
{
  if ( label != Item::PartBody )
    return false;

  KBookmark bk;
  QMimeData *mimeData = new QMimeData();
  mimeData->setData( QString::fromLatin1( "application/x-xbel" ), data.readAll() );
  KBookmark::List bkl = KBookmark::List::fromMimeData( mimeData );

  if ( !bkl.isEmpty() )
    item.setPayload<KBookmark>( bkl[0] );
  return true;
}

void SerializerPluginBookmark::serialize( const Item& item, const QString& label, QIODevice& data )
{
  if ( label != Item::PartBody )
    return;

  if ( item.mimeType() != QString::fromLatin1( "application/x-xbel" ) )
    return;

  KBookmark bk;
  if ( item.hasPayload() )
    bk = item.payload<KBookmark>();

  QMimeData *mimeData = new QMimeData();
  bk.populateMimeData( mimeData );

  data.write( mimeData->data( QString::fromLatin1( "application/x-xbel" ) ) );

}

extern "C"
KDE_EXPORT Akonadi::ItemSerializerPlugin *
akonadi_serializer_bookmark_create_item_serializer_plugin() {
  return new SerializerPluginBookmark();
}
