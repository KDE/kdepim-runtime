/*
    This file is part of KJots.

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

#include "akonadi_serializer_kjots.h"

#include <QtCore/qplugin.h>
#include <QDomDocument>

#include <akonadi/item.h>
#include <KTemporaryFile>
#include <KSaveFile>
#include <KStandardDirs>

#include "kjotspage.h"

#include <kdebug.h>

bool SerializerPluginKJots::deserialize( Item & item, const QByteArray &label, QIODevice &data, int version )
{
  Q_UNUSED( version );

  if ( label != Item::FullPayload ) {
    return false;
  }

  KJotsPage page = KJotsPage::fromIODevice( &data );

  if ( page.isValid() ) {
    item.setPayload< KJotsPage > ( page );
    return true;
  }


  return false;
}

void SerializerPluginKJots::serialize( const Item &item, const QByteArray &label, QIODevice &data, int &version )
{
  Q_UNUSED( version );

  if ( label != Item::FullPayload || !item.hasPayload() ) {
    return;
  }

  KJotsPage page = item.payload< KJotsPage >();

  QDomDocument doc( "KJotsPage" );
  QDomElement root = doc.createElement( "KJotsPage" );
  doc.appendChild( root );

  QDomElement title = doc.createElement( "Title" );
  title.appendChild( doc.createTextNode( page.title().toUtf8() ) );
  root.appendChild( title );

  QDomElement content = doc.createElement( "Content" );
  content.appendChild( doc.createCDATASection( page.content().toUtf8() ) );
  root.appendChild( content );

  data.write( doc.toString().toUtf8() );
}

Q_EXPORT_PLUGIN2( akonadi_serializer_kjots, SerializerPluginKJots )

#include "akonadi_serializer_kjots.moc"
