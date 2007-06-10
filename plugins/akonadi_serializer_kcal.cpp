/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "akonadi_serializer_kcal.h"

#include <libakonadi/item.h>
#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

using namespace Akonadi;

void SerializerPluginKCal::deserialize(Item & item, const QString & label, const QByteArray & data) const
{
  if ( label != "RFC822" ) {
    item.addPart( label, data );
    return;
  }
  if ( item.mimeType() != QString::fromLatin1("text/calendar") ) {
    //throw ItemSerializerException();
    return;
  }

  KCal::Incidence* i = const_cast<KCal::ICalFormat*>( &mFormat )->fromString( QString::fromUtf8( data ) );
  if ( !i ) {
    qWarning() << "Failed to parse incidence!";
    qWarning() << QString::fromUtf8( data );
    return;
  }
  item.setPayload<IncidencePtr>( IncidencePtr( i ) );
}

void SerializerPluginKCal::deserialize(Item & item, const QString & label, const QIODevice & data) const
{
  Q_UNUSED( item );
  Q_UNUSED( label );
  Q_UNUSED( data );
}

void SerializerPluginKCal::serialize(const Item & item, const QString & label, QByteArray & data) const
{
  if ( label != "RFC822" || !item.hasPayload<IncidencePtr>() )
    return;
  IncidencePtr i = item.payload<IncidencePtr>();
  // ### I guess this can be done without hardcoding stuff?
  data = "BEGIN:VCALENDAR\nPRODID:-//K Desktop Environment//NONSGML libkcal 3.2//EN\nVERSION:2.0\n";
  data += const_cast<KCal::ICalFormat*>( &mFormat )->toString( i.get() ).toUtf8();
  data += "\nEND:VCALENDAR";
}

void SerializerPluginKCal::serialize(const Item & item, const QString & label, QIODevice & data) const
{
  Q_UNUSED( item );
  Q_UNUSED( label );
  Q_UNUSED( data );
}

extern "C"
KDE_EXPORT Akonadi::ItemSerializerPlugin *
libakonadi_serializer_kcal_create_item_serializer_plugin() {
  return new SerializerPluginKCal();
}


