/*
    This file is part of Akonadi.

    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#include "akonadi_serializer_qimage.h"

#include <QtCore/qplugin.h>
#include <QBuffer>

#include <akonadi/item.h>

#include <kdebug.h>
#include <QImage>

bool SerializerPluginQImage::deserialize( Item & item, const QByteArray &label, QIODevice &data, int version )
{
  Q_UNUSED( version );

  if ( label != Item::FullPayload ) {
    return false;
  }

  QByteArray b64Data = data.readAll();
  QByteArray ba = QByteArray::fromBase64( b64Data );

  QImage image = QImage::fromData(ba);

//   QDataStream in(&data);
//   QImage image;
//   in >> image;

  if (image.isNull())
    return false;

  item.setPayload< QImage > ( image );
  return true;
}

void SerializerPluginQImage::serialize( const Item &item, const QByteArray &label, QIODevice &data, int &version )
{
  Q_UNUSED( version );

  if ( label != Item::FullPayload || !item.hasPayload() ) {
    return;
  }

  QImage image = item.payload< QImage >();

  Q_ASSERT(!image.isNull());

  QByteArray ba;
  QBuffer buffer(&ba);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");

  QByteArray b64Data = ba.toBase64();

  data.write( b64Data );

//   QDataStream out(&data);
//   out << b64Data;

}

Q_EXPORT_PLUGIN2( akonadi_serializer_qimage, SerializerPluginQImage )

#include "akonadi_serializer_qimage.moc"
