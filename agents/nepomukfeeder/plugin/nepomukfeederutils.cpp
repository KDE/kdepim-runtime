/*
    Copyright (C) 2011  Christian Mollekopf <chrigi_1@fastmail.fm>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "nepomukfeederutils.h"

#include <dms-copy/simpleresource.h>
#include <dms-copy/simpleresourcegraph.h>

#include <nao/tag.h>
#include <nao/freedesktopicon.h>
#include <QStringList>
#include <Soprano/Vocabulary/NAO>
#include <nco/personcontact.h>
#include <nco/emailaddress.h>



namespace NepomukFeederUtils 
{

void tagsFromCategories(const QStringList& categories, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph)
{
  foreach ( const QString &category, categories ) {
    addTag( res, graph, category );
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

Nepomuk::SimpleResource addTag( Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph, const QString& identifier, const QString &prefLabel )
{
  Nepomuk::SimpleResource tagResource;
  Nepomuk::NAO::Tag tag( &tagResource );
  tagResource.addProperty( Soprano::Vocabulary::NAO::identifier(), identifier);
  if (!prefLabel.isEmpty()) {
    tag.setPrefLabel( prefLabel );
  } else {
    tag.setPrefLabel( identifier );
  }
  graph << tagResource;
  res.addProperty( Soprano::Vocabulary::NAO::hasTag(), tagResource.uri() );
  return tagResource;
}


Nepomuk::SimpleResource addContact( const QString &emailAddress, const QString &name, Nepomuk::SimpleResourceGraph &graph )
{
  Nepomuk::SimpleResource contactRes;
  Nepomuk::NCO::PersonContact contact(&contactRes);
  contactRes.setProperty( Soprano::Vocabulary::NAO::prefLabel(), name.isEmpty() ? emailAddress : name);
  if ( !emailAddress.isEmpty() ) {
    Nepomuk::SimpleResource emailRes;
    Nepomuk::NCO::EmailAddress email( &emailRes );
    email.setEmailAddress( emailAddress );
    graph << emailRes;
    contact.addHasEmailAddress( emailRes.uri() );
  }
  if ( !name.isEmpty() )
    contact.setFullname( name );

  graph << contactRes;
  return contactRes;
}


}
