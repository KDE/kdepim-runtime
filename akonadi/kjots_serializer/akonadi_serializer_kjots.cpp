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

bool SerializerPluginKJots::deserialize( Item & item, const QByteArray &label, QIODevice &data, int version )
{
  Q_UNUSED( version );

  KJotsPage page;
  if (item.hasPayload<KJotsPage>())
    page = item.payload<KJotsPage>();

  QDomDocument doc;
  doc.setContent( &data );

  QDomElement pageRootElement = doc.documentElement();

  if ( pageRootElement.tagName() == QLatin1String( "KJotsPage" ) ) {
    QDomNode n = pageRootElement.firstChild();
    while ( !n.isNull() ) {
      QDomElement e = n.toElement();
      if ( !e.isNull() ) {
        if ( e.tagName() == QLatin1String( "Title" ) ) {
            page.setTitle( e.text() );
        }

        if ( e.tagName() == QLatin1String( "Content" ) ) {
            page.setContent( e.text() );
        }
      }
      n = n.nextSibling();
    }
  }

  item.setPayload< KJotsPage > ( page );
  return true;
}

void SerializerPluginKJots::serialize( const Item &item, const QByteArray &label, QIODevice &data, int &version )
{
  Q_UNUSED( version );

  if ( !item.hasPayload() ) {
    return;
  }

  KJotsPage page = item.payload< KJotsPage >();

  QDomDocument doc( QLatin1String( "KJotsPage" ) );
  QDomElement root = doc.createElement( QLatin1String( "KJotsPage" ) );
  doc.appendChild( root );

  QDomElement title = doc.createElement( QLatin1String( "Title" ) );
  title.appendChild( doc.createTextNode( page.title() ) );
  root.appendChild( title );

  QDomElement content = doc.createElement( QLatin1String( "Content" ) );
  content.appendChild( doc.createCDATASection( page.content() ) );
  root.appendChild( content );

  data.write( doc.toString().toUtf8() );
}

QSet< QByteArray > SerializerPluginKJots::parts(const Akonadi::Item& item) const
{
  QSet<QByteArray> loadedParts;

  if ( !item.hasPayload<KJotsPage>() )
    return loadedParts;

  KJotsPage page = item.payload<KJotsPage>();

  if (!page.title().isEmpty())
  {
    loadedParts << "title";
  }

  if (!page.content().isEmpty())
  {
    loadedParts << "content";
    if (!page.title().isEmpty())
      loadedParts << Item::FullPayload;
  }

  return loadedParts;
}

QSet< QByteArray > SerializerPluginKJots::availableParts(const Akonadi::Item& item) const
{
  QSet<QByteArray> available;
  if (!item.hasPayload<KJotsPage>())
    return available;

  KJotsPage page = item.payload<KJotsPage>();

  if (page.title().isEmpty())
    available << "title";

  if (page.content().isEmpty())
  {
    available << "content";
    if (page.title().isEmpty())
      available << Item::FullPayload;
  }

  return available;
}


void SerializerPluginKJots::merge(Item& item, const Akonadi::Item& other)
{
  if (!item.hasPayload<KJotsPage>() || !other.hasPayload<KJotsPage>())
    return;

  KJotsPage p1 = item.payload<KJotsPage>();
  KJotsPage p2 = other.payload<KJotsPage>();

  if (!p2.title().isEmpty())
    p1.setTitle(p2.title());

  if (!p2.content().isEmpty())
    p1.setContent(p2.content());

  item.setPayload(p1);
}


Q_EXPORT_PLUGIN2( akonadi_serializer_kjots, SerializerPluginKJots )

#include "akonadi_serializer_kjots.moc"
