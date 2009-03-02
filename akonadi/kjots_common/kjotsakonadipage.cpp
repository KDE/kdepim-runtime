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

#include "kjotsakonadipage.h"
#include "kjotspage.h"

#include "kdebug.h"

#include <QIODevice>
#include <QFile>

#include <KTemporaryFile>

#include <QString>
#include <QLatin1String>
#include <QDomDocument>
#include <akonadi/entitydisplayattribute.h>

#include "datadir.h"

KJotsAkonadiPage::KJotsAkonadiPage()
{
  m_changes = 0;
  // Temporary for development.
  m_rootDataPath = QString( DATADIR );
  //     rootDataPath = KStandardDirs::locateLocal( "data", "kjots/" );


//     EntityDisplayAttribute *eda = item.attribute<EntityDisplayAttribute>();
//     QString itemTitle = eda->displayName();

//   KJotsAkonadiPage(page);

}

KJotsAkonadiPage::KJotsAkonadiPage( Item item )
{
  m_changes = 0;
  // Temporary for development.
  m_rootDataPath = QString( DATADIR );
  //     rootDataPath = KStandardDirs::locateLocal( "data", "kjots/" );

  if ( item.remoteId().isEmpty() ) {
//     Invalid item. Create a page for it.
    EntityDisplayAttribute *eda = item.attribute<EntityDisplayAttribute>();
    QString itemTitle( "New Page" ); // = eda->displayName(); // "New Page" instead?

    QString newFileName;
    KTemporaryFile newFile;
    newFile.setPrefix( m_rootDataPath );
    newFile.setSuffix( ".kjpage" );
    newFile.setAutoRemove( false );
    if ( newFile.open() ) {
      newFileName = newFile.fileName();
    } else {
      return;
    }


    QDomDocument newDoc( "KJotsPage" );
    QDomElement root = newDoc.createElement( "KJotsPage" );
    newDoc.appendChild( root );

    QDomElement titleTag = newDoc.createElement( "Title" );
    root.appendChild( titleTag );
    QDomText titleText = newDoc.createTextNode( itemTitle );
    titleTag.appendChild( titleText );


    QDomElement contentTag = newDoc.createElement( "Content" );
    contentTag.appendChild( newDoc.createCDATASection( QString() ) );
    root.appendChild( contentTag );

    // Add page content.

    newFile.write( newDoc.toString().toUtf8() );

    QString itemRemoteId = KUrl( newFile.fileName() ).fileName();

    item.setRemoteId( itemRemoteId );

  }

  m_item = item;

}

KJotsAkonadiPage::KJotsAkonadiPage( KJotsPage page )
{
  Item item( KJotsPage::mimeType() );
  item.setRemoteId( page.remoteId() );

  item.setPayload<KJotsPage>( page );

  EntityDisplayAttribute *displayAttribute = new EntityDisplayAttribute();
  displayAttribute->setIconName( "text-x-generic" );
  displayAttribute->setDisplayName( page.title() );
  item.addAttribute( displayAttribute );

  m_item = item;
}

QString KJotsAkonadiPage::remoteId()
{
  return m_item.remoteId();
}

KJotsAkonadiPage::~KJotsAkonadiPage()
{
}

Item KJotsAkonadiPage::getItem()
{
  return m_item;
}


bool KJotsAkonadiPage::isValid()
{
  return !m_page.title().isNull();
}

Item KJotsAkonadiPage::writeOut()
{
  KUrl pageUrl( m_rootDataPath + m_item.remoteId() );
  QFile pageFile( pageUrl.toLocalFile() );

  pageFile.write( m_domDocument.toString().toUtf8() );

  return m_item;

}

int KJotsAkonadiPage::changes()
{
  return m_changes;
//   return 0;
}

