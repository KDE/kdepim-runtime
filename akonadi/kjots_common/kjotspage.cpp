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

#include "kjotspage.h"

#include "kdebug.h"

#include <QIODevice>

#include <QString>
#include <KUrl>
#include <QFile>
#include <QLatin1String>
#include <QDomDocument>

#include "datadir.h"

KJotsPage::KJotsPage()
{
}

KJotsPage::KJotsPage( const QString &remoteId )
{

  // Temporary for development.
  m_rootDataPath = QString( DATADIR );
  //     rootDataPath = KStandardDirs::locateLocal( "data", "kjots/" );

  KUrl pageUrl( m_rootDataPath + remoteId );
  QFile pageFile( pageUrl.toLocalFile() );
  QDomDocument doc;
  doc.setContent( &pageFile );
  setDomDocument( doc );
  m_remoteId = remoteId;
}

KJotsPage::KJotsPage( QDomDocument doc )
{
  setDomDocument( doc );
}

void KJotsPage::setDomDocument( QDomDocument doc )
{

  m_domDocument = doc;
  QDomElement pageRootElement = m_domDocument.documentElement();
  if ( pageRootElement.tagName() ==  "KJotsPage" ) {

    QDomNode n = pageRootElement.firstChild();
    while ( !n.isNull() ) {
      QDomElement e = n.toElement();
      if ( !e.isNull() ) {
        if ( e.tagName() == "Title" ) {
          setTitle( e.text() );
        }

        if ( e.tagName() == "Content" ) {
          setContent( QString( e.text() ) );
        }
      }
      n = n.nextSibling();
    }
  }
}

KJotsPage::~KJotsPage()
{
}

void KJotsPage::setTitle( const QString &title )
{
  m_title = title;
}

void KJotsPage::setContent( const QString &content )
{
  m_content = content;
}

void KJotsPage::setRemoteId( const QString &remoteId )
{
  m_remoteId = remoteId;
}

QString KJotsPage::title()
{
  return m_title;
}

QString KJotsPage::content()
{
  return m_content;
}


QString KJotsPage::remoteId()
{
  return m_remoteId;
}

KJotsPage KJotsPage::fromIODevice( QIODevice *dev )
{
  QDomDocument pageDoc( "KjotsPage" );
  pageDoc.setContent( dev );

  KJotsPage page( pageDoc );
  return page;
}

bool KJotsPage::isValid()
{
  return !title().isNull();
}

bool KJotsPage::save()
{
  QDomDocument newDoc( "KJotsPage" );
  QDomElement root = newDoc.createElement( "KJotsPage" );
  newDoc.appendChild( root );

  QDomElement titleTag = newDoc.createElement( "Title" );
  root.appendChild( titleTag );
  QDomText titleText = newDoc.createTextNode( title() );
  titleTag.appendChild( titleText );


  QDomElement contentTag = newDoc.createElement( "Content" );
  contentTag.appendChild( newDoc.createCDATASection( content() ) );
  root.appendChild( contentTag );

  KUrl pageUrl( QString( DATADIR ) + remoteId() );
  QFile pageFile( pageUrl.toLocalFile() );

  kDebug() << pageFile.fileName();

  if ( pageFile.open( QIODevice::WriteOnly ) ) {
    pageFile.write( m_domDocument.toString().toUtf8() );
    pageFile.close();
    return true;
  }
  return false;

}
