/*
  This file is part of libkdepim.

  Copyright (c) 2002 Tobias Koenig <tokoe@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kvcarddrag.h"
#include <kabc/vcardconverter.h>
#include <QMimeData>

using namespace KPIM;

QString KVCardDrag::mimeType()
{
  return "text/directory";
}

bool KVCardDrag::populateMimeData( QMimeData *md, const QByteArray &content )
{
  md->setData( mimeType(), content );
  return true;
}

bool KVCardDrag::populateMimeData( QMimeData *md, const KABC::Addressee::List &addressees )
{
  KABC::VCardConverter converter;
  QByteArray vcards = converter.createVCards( addressees );
  if ( !vcards.isEmpty() ) {
    return populateMimeData( md, vcards );
  } else {
    return false;
  }
}

bool KVCardDrag::canDecode( const QMimeData *md )
{
  return md->hasFormat( mimeType() );
}

bool KVCardDrag::fromMimeData( const QMimeData *md, QByteArray &content )
{
  if ( !canDecode(md) ) {
    return false;
  }
  content = md->data( mimeType() );
  return true;
}

bool KVCardDrag::fromMimeData( const QMimeData *md, KABC::Addressee::List &addressees )
{
  if ( !canDecode( md ) ) {
    return false;
  }
  addressees = KABC::VCardConverter().parseVCards( md->data( mimeType() ) );
  return true;
}

