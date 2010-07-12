/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "akonadi_serializer_contactgroup.h"

#include <QtCore/qplugin.h>

#include <kabc/contactgroup.h>
#include <kabc/contactgrouptool.h>

#include <kdebug.h>

#include <akonadi/item.h>
#include <akonadi/kabc/contactparts.h>

using namespace Akonadi;

bool SerializerPluginContactGroup::deserialize( Item& item, const QByteArray& label, QIODevice& data, int version )
{
  Q_UNUSED( label );
  Q_UNUSED( version );

  KABC::ContactGroup contactGroup;

  if ( !KABC::ContactGroupTool::convertFromXml( &data, contactGroup ) ){
    // TODO: error reporting
    return false;
  }

  item.setPayload<KABC::ContactGroup>( contactGroup );

  return true;
}

void SerializerPluginContactGroup::serialize( const Item& item, const QByteArray& label, QIODevice& data, int &version )
{
  Q_UNUSED( label );
  Q_UNUSED( version );

  if ( !item.hasPayload<KABC::ContactGroup>() )
    return;

  KABC::ContactGroupTool::convertToXml( item.payload<KABC::ContactGroup>(), &data );
}

Q_EXPORT_PLUGIN2( akonadi_serializer_contactgroup, Akonadi::SerializerPluginContactGroup )

#include "akonadi_serializer_contactgroup.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
