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

#include "kjotsbook.h"

#include <QFile>
#include <QDomDocument>

#include <KTemporaryFile>

#include "collectionchildorderattribute.h"
#include <akonadi/entitydisplayattribute.h>

#include "kjotspage.h"
#include "kjotsakonadipage.h"

#include <kdebug.h>
#include "datadir.h"

using namespace Akonadi;

KJotsBook::KJotsBook( const QString &remoteId )
{
  // Temporary for development.
  m_rootDataPath = QString( DATADIR );
  //     rootDataPath = KStandardDirs::locateLocal( "data", "kjots/" );

  if ( remoteId == bookshelfRemoteId() ) {
    // The bookshelf can not contain pages.
    m_collection.setContentMimeTypes( QStringList()
                                      << Collection::mimeType() );
    m_collection.setRemoteId( remoteId );
  } else {
    m_collection.setContentMimeTypes( QStringList()
                                      << KJotsPage::mimeType()
                                      << Collection::mimeType() );
  }

  if ( !remoteId.isEmpty() ) {
    m_collection.setRemoteId( remoteId );

  } else {
    QString newFileName;
    KTemporaryFile newFile;
    newFile.setPrefix( m_rootDataPath );
    newFile.setSuffix( ".kjbook" );
    newFile.setAutoRemove( false );
    if ( newFile.open() ) {
      newFileName = newFile.fileName();
      kDebug() << "creating" << newFileName;
    } else {
      return;
    }

    QDomDocument newDoc( "KJotsBook" );
    QDomElement root = newDoc.createElement( "KJotsBook" );
    newDoc.appendChild( root );

    QDomElement titleTag = newDoc.createElement( "Title" );
    root.appendChild( titleTag );
    QDomText titleText = newDoc.createTextNode( "New Book" );
    titleTag.appendChild( titleText );
    QDomElement contentsTag = newDoc.createElement( "Contents" );
    root.appendChild( contentsTag );

    setDomDocument( newDoc );

    newFile.write( newDoc.toString().toUtf8() );
//     m_remoteId = KUrl( newFile.fileName() ).fileName();
    m_collection.setRemoteId( KUrl( newFile.fileName() ).fileName() );
    return;

  }

  KUrl bookUrl( m_rootDataPath + remoteId );
  QFile bookFile( bookUrl.toLocalFile() );


  QDomDocument doc;
  doc.setContent( &bookFile );
  setDomDocument( doc );
}

void KJotsBook::setCollection( Collection col )
{
  m_collection = col;
}

KJotsBook KJotsBook::getBookshelf()
{
  // Temporary for development.
  QString rootDataPath = QString( DATADIR );
  //     rootDataPath = KStandardDirs::locateLocal( "data", "kjots/" );

  QString rootRemoteId = bookshelfRemoteId();

  KUrl rootUrl( rootDataPath + rootRemoteId );

  QFile bookshelfFile( rootUrl.toLocalFile() );
//   if not exists, create an empty bookshelf.
  if ( !bookshelfFile.exists() ) {
    if ( bookshelfFile.open( QIODevice::WriteOnly ) ) {
      QDomDocument doc( "KJotsBookshelf" );
      QDomElement root = doc.createElement( "KJotsBook" );
      doc.appendChild( root );

      QDomElement titleTag = doc.createElement( "Title" );
      root.appendChild( titleTag );
      QDomText titleText = doc.createTextNode( "Bookshelf" );  // No i18n. That's done later. ???
      titleTag.appendChild( titleText );
      QDomElement contentsTag = doc.createElement( "Contents" );
      root.appendChild( contentsTag );

      bookshelfFile.write( doc.toString().toUtf8() );
    }
  }

  KJotsBook rootBook( rootRemoteId );

  return rootBook;

}

KJotsBook::~KJotsBook()
{
}

void KJotsBook::setDisplayName( const QString &name )
{
//   m_name = name;
}

QString KJotsBook::displayName()
{
  return "m_name";
}

void KJotsBook::setDomDocument( QDomDocument doc )
{
  m_collectionChildOrder.clear();
  m_domDocument = doc;
  QDomElement rootElement = m_domDocument.documentElement();

  QDomElement title = rootElement.firstChildElement( "Title" );
  QString bookname = title.text();

  EntityDisplayAttribute *displayAttribute = new EntityDisplayAttribute();
  displayAttribute->setDisplayName( bookname );
//   displayAttribute->setIconName( "x-office-address-book" );
  m_collection.addAttribute( displayAttribute );
  m_collection.setName( bookname );

  QDomElement contents = rootElement.firstChildElement( "Contents" );

  QDomElement entity = contents.firstChildElement();
  QString filename;
  while ( !entity.isNull() ) {
    filename = entity.attribute( "filename" ) ;
    if ( entity.tagName() == "KJotsPage" ) {
      m_itemremoteIds.append( filename );
    } else if ( entity.tagName() == "KJotsBook" ) {
      m_childCollections.append( filename );
    }
    m_collectionChildOrder.append( filename );

    entity = entity.nextSiblingElement();
  }
  CollectionChildOrderAttribute *ccoa = new CollectionChildOrderAttribute();
  ccoa->setOrderList( m_collectionChildOrder );
//   m_collection.addAttribute( ccoa );
}

Item::List KJotsBook::retrieveItems()
{
  foreach( const QString &remoteId, m_itemremoteIds ) {
    KJotsPage page( remoteId );
    KJotsAkonadiPage kjap( page );
    Item item = kjap.getItem();

    m_items << item;
  }
  return m_items;
}


Item KJotsBook::getItem( Item item )
{
//   KJotsPage page(item.remoteId());

//   Already have everything set.

  return item;
}

Item KJotsBook::addItem( Item item )
{
  // create a new page file.
  // add the remote id of the new file to the end of the m_domDocument
  KJotsAkonadiPage page( item );
  item = page.getItem();

  QDomElement rootElement = m_domDocument.documentElement();
  QDomElement contents = rootElement.firstChildElement( "Contents" );

  QDomElement e = m_domDocument.createElement( "KJotsPage" );
  e.setAttribute( "filename", item.remoteId() );
  contents.appendChild( e );

  m_collectionChildOrder << item.remoteId();
  updateDomNodeOrder();

  writeOut();

  return item;
}

void KJotsBook::removeEntity( const QString &rId )
{

  QDomElement rootElement = m_domDocument.documentElement();
  QDomElement contents = rootElement.firstChildElement( "Contents" );

  QDomElement entity = contents.firstChildElement();
  while ( !entity.isNull() ) {
    if ( entity.attribute( "filename" ) == rId ) {
      contents.removeChild(entity);
      if ( entity.tagName() == "KJotsBook" )
        m_childCollections.removeOne( rId );
      else if ( entity.tagName() == "KJotsPage" )
        m_itemremoteIds.removeOne( rId );
      m_collectionChildOrder.removeOne( rId );
      writeOut();
      return;
    }
    entity = entity.nextSiblingElement();
  }
  kDebug() << "Not Found:" << rId;
}


void KJotsBook::addEntity( const QString &rId )
{

  QDomElement rootElement = m_domDocument.documentElement();
  QDomElement contents = rootElement.firstChildElement( "Contents" );

  QString tagname;
  if (rId.endsWith(".kjbook"))
  {
    tagname = "KJotsBook";
  } else if (rId.endsWith(".kjpage"))
  {
    tagname = "KJotsPage";
  } else {
    kDebug() << "Not a valid entity";
  }

  QDomElement entity = m_domDocument.createElement( tagname );
  entity.setAttribute("filename", rId);

  contents.appendChild( entity );
  writeOut();
}

void KJotsBook::updateDomNodeOrder()
{
  QDomElement rootElement = m_domDocument.documentElement();

  QDomElement contents = rootElement.firstChildElement( "Contents" );

  QDomElement left = contents.firstChildElement();
  QDomElement right = contents.lastChildElement();

  QStringList l = m_collectionChildOrder;

  QStringListIterator li( l );
  QStringListIterator ri( l );
  ri.toBack();


  // TODO: This was written with the wrong assumtion that only one item is changed at a time.
  // Rewrite.
  bool removalOnly   = ( contents.childNodes().size() > l.size() );
  bool insertionOnly = ( contents.childNodes().size() < l.size() );

  bool insertionDone = false;
  bool removalDone = false;

  QString filename;
  while ( li.hasNext() ) {
    filename = li.next();
    if ( left.attribute( "filename" ) != filename ) {
      if ( left.isNull() || ( li.hasNext() && li.peekNext() == left.attribute( "filename" ) ) ) {
        QDomElement newE = m_domDocument.createElement( filename.endsWith( ".kjbook" ) ? "KJotsBook" : "KJotsPage" );
        newE.setAttribute( "filename", filename );
        QDomNode leftNode = contents.insertBefore( newE, left );
        left = leftNode.toElement();
        insertionDone = true;
        if ( insertionOnly || removalDone )
          break;
      } else if ( !left.nextSiblingElement().isNull() && left.nextSiblingElement().attribute( "filename" ) == filename ) {
        contents.removeChild( left );
        removalDone = true;
        if ( removalOnly || insertionDone )
          break;
      }
    }
    left = left.nextSiblingElement();
  }
}

Item KJotsBook::updateItem( Item item )
{
  KJotsAkonadiPage page( item );
  item = page.getItem();

  return item;
}

QString KJotsBook::getRemoteId()
{
  return m_collection.remoteId();
}

Item KJotsBook::removeItem( Item item )
{

  m_collectionChildOrder.removeOne( item.remoteId() );

  QFile::remove( m_rootDataPath + item.remoteId() );
  updateDomNodeOrder();
  item.clearAttributes();
  writeOut();

  return item;
}

Collection KJotsBook::addCollection( Collection collection )
{
  KJotsBook book( collection.remoteId() );

  EntityDisplayAttribute *eda = new EntityDisplayAttribute();
  eda->setIconName( "x-office-address-book" );
  eda->setDisplayName( collection.name() );

  collection.addAttribute( eda );

  QDomElement rootElement = m_domDocument.documentElement();
  QDomElement contents = rootElement.firstChildElement( "Contents" );

  QDomElement e = m_domDocument.createElement( "KJotsBook" );
  e.setAttribute( "filename", collection.remoteId() );
  contents.appendChild( e );
  m_collectionChildOrder << collection.remoteId();
  updateDomNodeOrder();

  writeOut();
  return collection;
}

Collection KJotsBook::updateCollection( Collection collection )
{
  KJotsBook book( collection.remoteId() );

  // TODO Update name etc

// TODO: Only do this if nothing else has been updated.
  if ( collection.hasAttribute<CollectionChildOrderAttribute>() ) {
    QStringList l = collection.attribute<CollectionChildOrderAttribute>()->orderList();
    m_collectionChildOrder = l;
    updateDomNodeOrder();
  }

  writeOut();

  return collection;
}

Collection KJotsBook::removeCollection( Collection collection )
{
  m_collectionChildOrder.removeOne( collection.remoteId() );

  QFile::remove( m_rootDataPath + collection.remoteId() );
  updateDomNodeOrder();
  collection.clearAttributes();
  writeOut();
  return collection;
}

Collection KJotsBook::writeOut()
{
  Collection col;

  KUrl bookUrl( m_rootDataPath + m_collection.remoteId() );
  QFile bookFile( bookUrl.toLocalFile() );

  if ( bookFile.open( QIODevice::WriteOnly ) ) {
    bookFile.write( m_domDocument.toString().toUtf8() );
    bookFile.close();
  }

  return col;
}


Collection::List KJotsBook::getCollections( int readType )
{
  Collection::List list;

  if (( readType == ReadFirstLevel ) || ( readType == ReadCollectionsRecursive ) ) {
    foreach( const QString &rId, m_childCollections ) {
      KJotsBook book( rId );
      Collection col = book.getCollection();
      col.setParent( m_collection );
      // TODO: Why is this neccessary? Possibly something to do with a special case for bookshelf?
      col.attribute<EntityDisplayAttribute>()->setIconName( "x-office-address-book" );

      list << col;
      if ( readType == ReadCollectionsRecursive ) {
        list << book.getCollections( ReadCollectionsRecursive );
      }
    }
  }
  return list;
}

Collection KJotsBook::getCollection()
{
  return m_collection;
}


