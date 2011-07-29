/*
    Copyright (c) 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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


#include "nepomukfeederutils.h"

#include <nepomuk/simpleresource.h>
#include <nepomuk/simpleresourcegraph.h>

#include <nao/tag.h>
#include <nao/freedesktopicon.h>
#include <QStringList>
#include <Soprano/Vocabulary/NAO>


namespace NepomukFeederUtils 
{

void tagsFromCategories(const QStringList& categories, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph)
{
  foreach ( const QString &category, categories ) {
    Nepomuk::SimpleResource tagResource;
    Nepomuk::NAO::Tag tag( &tagResource );
    tag.setPrefLabel( category );
    graph << tagResource;
    res.addProperty( Soprano::Vocabulary::NAO::hasTag(), tagResource.uri() );
  }
}

void setIcon(const QString& iconName, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph)
{
  Nepomuk::SimpleResource iconRes;
  Nepomuk::NAO::FreeDesktopIcon icon(&iconRes);
  icon.setIconNames( QStringList() << iconName );
  graph << iconRes;
  res.setProperty( Soprano::Vocabulary::NAO::prefSymbol(), iconRes.uri() );
}

}