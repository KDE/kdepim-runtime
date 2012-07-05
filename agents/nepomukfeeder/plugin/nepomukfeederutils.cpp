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

#include <nepomuk2/simpleresource.h>
#include <nepomuk2/simpleresourcegraph.h>

#include <nao/tag.h>
#include <nao/freedesktopicon.h>
#include <Soprano/Vocabulary/NAO>
#include <nco/personcontact.h>
#include <nco/emailaddress.h>

#include <KDebug>
#include <KProcess>
#include <KUrl>

#include <QStringList>


namespace NepomukFeederUtils
{

void tagsFromCategories(const QStringList& categories, Nepomuk2::SimpleResource& res, Nepomuk2::SimpleResourceGraph& graph)
{
  foreach ( const QString &category, categories ) {
    addTag( res, graph, category );
  }
}

void setIcon(const QString& iconName, Nepomuk2::SimpleResource& res, Nepomuk2::SimpleResourceGraph& graph)
{
  Nepomuk2::SimpleResource iconRes;
  Nepomuk2::NAO::FreeDesktopIcon icon( &iconRes );
  icon.setIconNames( QStringList() << iconName );
  graph << iconRes;
  res.setProperty( Soprano::Vocabulary::NAO::prefSymbol(), iconRes.uri() );
}

Nepomuk2::SimpleResource addTag( Nepomuk2::SimpleResource& res, Nepomuk2::SimpleResourceGraph& graph, const QString& identifier, const QString &prefLabel )
{
  Nepomuk2::SimpleResource tagResource;
  Nepomuk2::NAO::Tag tag( &tagResource );
  tagResource.addProperty( Soprano::Vocabulary::NAO::identifier(), identifier );
  if ( !prefLabel.isEmpty() ) {
    tag.setPrefLabel( prefLabel );
  } else {
    tag.setPrefLabel( identifier );
  }
  graph << tagResource;
  res.addProperty( Soprano::Vocabulary::NAO::hasTag(), tagResource.uri() );
  return tagResource;
}


Nepomuk2::SimpleResource addContact( const QString &emailAddress, const QString &name, Nepomuk2::SimpleResourceGraph &graph )
{
  Nepomuk2::SimpleResource contactRes;
  Nepomuk2::NCO::Contact contact( &contactRes );
  contactRes.setProperty( Soprano::Vocabulary::NAO::prefLabel(), name.isEmpty() ? emailAddress : name );
  if ( !emailAddress.isEmpty() ) {
    Nepomuk2::SimpleResource emailRes;
    Nepomuk2::NCO::EmailAddress email( &emailRes );
    email.setEmailAddress( emailAddress.toLower() );
    graph << emailRes;
    contact.addHasEmailAddress( emailRes.uri() );
  }
  if ( !name.isEmpty() )
    contact.setFullname( name );

  graph << contactRes;
  return contactRes;
}

void indexData(const KUrl& url, const QByteArray& data, const QDateTime& mtime)
{
  KProcess proc;
  proc.setOutputChannelMode( KProcess::ForwardedChannels );
  proc.setProgram( "nepomukindexer" );
  proc << "--uri" << url.url().toLocal8Bit();
  proc << "--mtime" << QString::number( mtime.toTime_t() );
  proc.start();
  if ( proc.waitForStarted() ) {
    proc.write( data );
    proc.waitForBytesWritten();
    proc.closeWriteChannel();
  } else {
    kDebug() << "Failed to launch nepomukindexer: " << proc.errorString();
  }
  proc.waitForFinished();
  if ( !proc.exitStatus() == QProcess::NormalExit ) {
    kDebug() << proc.exitCode() << proc.errorString();
  }
}

}
