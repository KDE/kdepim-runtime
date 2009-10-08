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

#include "kjotsresource.h"

#include <QtCore/QDir>
#include <QtXml/QDomDocument>

#include <akonadi/changerecorder.h>
#include <akonadi/attributefactory.h>
#include <akonadi/entitydisplayattribute.h>
#include "collectionchildorderattribute.h"
#include <akonadi/itemfetchscope.h>

#include <KUrl>
#include <KStandardDirs>
#include <kfiledialog.h>
#include <klocalizedstring.h>
#include <KTemporaryFile>

#include "kjotspage.h"

using namespace Akonadi;

QString KJotsResource::getFileUrl( const Collection &col ) const
{
  return m_rootDataPath + QLatin1Char( '/' ) + col.remoteId();
}

QString KJotsResource::getFileUrl( const Item &item ) const
{
  return m_rootDataPath + QLatin1Char( '/' ) + item.remoteId();
}

QDomDocument KJotsResource::getBook( const Collection &collection ) const
{
  QString fileUrl = getFileUrl( collection );
  KUrl bookUrl( fileUrl );
  QFile bookFile( bookUrl.toLocalFile() );

  QDomDocument doc;
  doc.setContent( &bookFile );
  return doc;
}

Collection::List KJotsResource::getDescendantCollections( Collection& col ) const
{
  Collection::List list;

  QDomDocument doc = getBook( col );

  QDomElement rootElement = doc.documentElement();

  QDomElement title = rootElement.firstChildElement( QLatin1String( "Title" ) );
  QString bookname = title.text();

  if (col.parentCollection() != Collection::root() )
  {
    EntityDisplayAttribute *displayAttribute = new EntityDisplayAttribute();
    displayAttribute->setDisplayName( bookname );
    displayAttribute->setIconName( QLatin1String( "x-office-address-book" ) );
    col.addAttribute( displayAttribute );
  }
  col.setName( bookname );

  QDomElement contents = rootElement.firstChildElement( QLatin1String( "Contents" ) );

  QDomElement entity = contents.firstChildElement();
  QString filename;
  while ( !entity.isNull() ) {
    if ( entity.tagName() == QLatin1String( "KJotsBook" ) ) {
      filename = entity.attribute( QLatin1String( "filename" ) );
      Collection child;
      child.setRemoteId( filename );
      child.setContentMimeTypes( QStringList() << Collection::mimeType() << KJotsPage::mimeType() );
      child.setParentCollection( col );
      list << getDescendantCollections( child );
      list << child;
    }
    entity = entity.nextSiblingElement();
  }

  return list;
}

Item::List KJotsResource::getContainedItems( const Collection &col ) const
{
  Item::List list;

  QDomDocument doc = getBook( col );

  QDomElement rootElement = doc.documentElement();

  QDomElement contents = rootElement.firstChildElement( QLatin1String( "Contents" ) );

  QDomElement entity = contents.firstChildElement();
  QString filename;
  while ( !entity.isNull() ) {
    filename = entity.attribute( QLatin1String( "filename" ) );
    if ( entity.tagName() == QLatin1String( "KJotsPage" ) ) {
      Item item;
      item.setRemoteId( filename );
      item.setMimeType( KJotsPage::mimeType() );
      item.setParentCollection( col );
      list << item;
    }
    entity = entity.nextSiblingElement();
  }

  return list;
}

KJotsPage KJotsResource::getPage( const Akonadi::Item& item, QSet<QByteArray> parts ) const
{
  QString fileUrl = getFileUrl( item );
  KUrl pageUrl( fileUrl );
  QFile pageFile( pageUrl.toLocalFile() );

  KJotsPage page;
  QDomDocument doc;
  doc.setContent( &pageFile );

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
          page.setContent( QString( e.text() ) );
        }
      }
      n = n.nextSibling();
    }
  }

  return page;
}

bool KJotsResource::addContentEntry( const Collection &parent, const QString &contentFilename ) const
{
  QString destinationFile = getFileUrl( parent );
  KUrl destBook( destinationFile );
  QFile destBookFile( destBook.toLocalFile() );

  QDomDocument destBookDoc;
  destBookDoc.setContent( &destBookFile );
  destBookFile.close();
  if ( !destBookFile.open( QFile::WriteOnly ) )
  {
    return false;
  }
  QDomElement rootElement = destBookDoc.documentElement();

  QDomElement contents = rootElement.firstChildElement( QLatin1String( "Contents" ) );

  QDomElement newElement;
  if ( contentFilename.endsWith( QLatin1String( ".kjbook" ) ) )
    newElement = destBookDoc.createElement( QLatin1String( "KJotsBook" ) );
  else
    newElement = destBookDoc.createElement( QLatin1String( "KJotsPage" ) );

  newElement.setAttribute( QLatin1String( "filename" ), contentFilename );
  contents.appendChild( newElement );

  destBookFile.write( destBookDoc.toString().toUtf8() );
  destBookFile.close();

  return true;
}

bool KJotsResource::removeContentEntry( const Collection &parent, const QString &contentFilename ) const
{
  QString fileUrl = getFileUrl( parent );
  KUrl bookUrl( fileUrl );
  QFile bookFile( bookUrl.toLocalFile() );

  QDomDocument bookDoc;
  bookDoc.setContent( &bookFile );
  bookFile.close();
  if ( !bookFile.open( QFile::WriteOnly ) )
  {
    return false;
  }

  QDomElement rootElement = bookDoc.documentElement();

  QDomElement contents = rootElement.firstChildElement( QLatin1String( "Contents" ) );
  QDomElement pageElement = contents.firstChildElement( QLatin1String( "KJotsPage" ) );
  while ( !pageElement.isNull() )
  {
    if ( pageElement.attribute( QLatin1String( "filename" ) ) == contentFilename )
    {
      contents.removeChild( pageElement );
    }
  }
  bookFile.write( bookDoc.toString().toUtf8() );
  bookFile.close();

  return true;
}

QDomDocument KJotsResource::getDomDocument( const KJotsPage &page ) const
{
  QDomDocument pageDoc;
  QDomElement pageElement = pageDoc.createElement( QLatin1String( "KJotsPage" ) );
  pageDoc.appendChild( pageElement );

  QDomElement titleElement = pageDoc.createElement( QLatin1String( "Title" ) );
  titleElement.appendChild( pageDoc.createTextNode( page.title() ) );
  pageElement.appendChild( titleElement );

  QDomElement contentElement = pageDoc.createElement( QLatin1String( "Content" ) );
  contentElement.appendChild( pageDoc.createCDATASection( page.content() ) );
  pageElement.appendChild( contentElement );

  return pageDoc;
}


KJotsResource::KJotsResource( const QString &id )
    : ResourceBase( id )
{
  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->itemFetchScope().fetchAttribute<CollectionChildOrderAttribute>();
  changeRecorder()->itemFetchScope().fetchAttribute<EntityDisplayAttribute>();
  setHierarchicalRemoteIdentifiersEnabled(true);

  m_rootDataPath = KStandardDirs::locateLocal( "data", QLatin1String( "kjots-port/" ) );

  AttributeFactory::registerAttribute<CollectionChildOrderAttribute>();
}


KJotsResource::~KJotsResource()
{
}

void KJotsResource::retrieveCollections()
{
  Collection::List list;

  Collection resourceRootCollection;
  resourceRootCollection.setRemoteId( QLatin1String( "/kjots.bookshelf" ) );
  resourceRootCollection.setParentCollection( Collection::root() );
  // The bookshelf can not contain pages.
  resourceRootCollection.setContentMimeTypes( QStringList()
                                              << Collection::mimeType() );

  list << getDescendantCollections(resourceRootCollection);
  list << resourceRootCollection;
  collectionsRetrieved( list );

}

void KJotsResource::retrieveItems( const Akonadi::Collection &collection )
{
  Item::List items = getContainedItems( collection );
  itemsRetrieved( items );
}

bool KJotsResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Item newItem = item;

  KJotsPage page = getPage( item, parts );

  newItem.setPayload<KJotsPage>( page );

  EntityDisplayAttribute *displayAttribute = new EntityDisplayAttribute();
  displayAttribute->setDisplayName( page.title() );
  displayAttribute->setIconName( QLatin1String( "text-x-generic" ) );
  newItem.addAttribute( displayAttribute );

  itemRetrieved( newItem );
  return true;
}

void KJotsResource::aboutToQuit()
{
  // TODO: any cleanup you need to do while there is still an active
  // event loop. The resource will terminate after this method returns
}

void KJotsResource::configure( WId windowId )
{
  Q_UNUSED( windowId );


  // This resource doesn't need to be configured.

  synchronize();
}

void KJotsResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  KTemporaryFile tempFile;
  tempFile.setPrefix( m_rootDataPath );
  tempFile.setSuffix( QLatin1String( ".kjpage" ) );
  tempFile.setAutoRemove( false );

  if ( !tempFile.open() )
    return;

  KJotsPage page;
  page = item.payload<KJotsPage>();

  QDomDocument pageDoc = getDomDocument(page);

  QString remoteId = tempFile.fileName().split( QLatin1String(  "/" ) ).last();

  tempFile.write( pageDoc.toByteArray() );
  tempFile.close();

  if ( !addContentEntry( collection, remoteId ) )
    return;

  Item newItem = item;
  newItem.setRemoteId( remoteId );

//   EntityDisplayAttribute *eda = new EntityDisplayAttribute;
//   eda->setDisplayName( page.title() );
//   eda->setIconName( QLatin1String( "text-x-generic" ) );
//   newItem.addAttribute(eda);

  changeCommitted( newItem );
}

void KJotsResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  QString fileUrl = getFileUrl( item );
  KUrl pageUrl( fileUrl );
  QFile pageFile( pageUrl.toLocalFile() );

  if ( !item.hasPayload<KJotsPage>() )
    return;

  KJotsPage page = item.payload<KJotsPage>();

  if ( !pageFile.open( QFile::WriteOnly ) )
    return;

  QDomDocument pageDoc = getDomDocument( page );

  pageFile.write( pageDoc.toByteArray() );
  pageFile.close();

  changeCommitted( item );
}

void KJotsResource::itemRemoved( const Akonadi::Item &item )
{
  QFile::remove( item.remoteId() );

  Collection parentBook = item.parentCollection();

  removeContentEntry( parentBook, item.remoteId() );

  changeCommitted( item );

}

void KJotsResource::itemMoved( const Akonadi::Item &item,
                               const Akonadi::Collection &source,
                               const Akonadi::Collection &destination )
{
  if ( !entityMoved( item, source, destination ) )
    return;

  changeCommitted( item );
}

void KJotsResource::collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent )
{
  KTemporaryFile tempFile;
  tempFile.setPrefix( m_rootDataPath );
  tempFile.setSuffix( QLatin1String( ".kjbook" ) );
  tempFile.setAutoRemove( false );

  if ( !tempFile.open() )
    return;

  QDomDocument bookDoc;

  QDomElement bookElement = bookDoc.createElement( QLatin1String( "KJotsBook" ) );
  bookDoc.appendChild( bookElement );

  QDomElement titleElement = bookDoc.createElement( QLatin1String( "Title" ) );
  bookElement.appendChild( titleElement );
  QDomElement contentElement = bookDoc.createElement( QLatin1String( "Contents" ) );
  bookElement.appendChild( contentElement );

  tempFile.write( bookDoc.toString().toUtf8() );

  QString remoteId = tempFile.fileName().split( QLatin1String(  "/" ) ).last();

  tempFile.close();

  if ( !addContentEntry( parent, remoteId ) )
    return;

  Collection newCollection = collection;
  newCollection.setRemoteId( remoteId );

  changeCommitted( newCollection );
}

void KJotsResource::collectionRemoved( const Akonadi::Collection &collection )
{
  QFile::remove( collection.remoteId() );

  Collection parentBook = collection.parentCollection();

  removeContentEntry( parentBook, collection.remoteId() );

  changeCommitted( collection );
}

void KJotsResource::collectionChanged( const Akonadi::Collection &collection )
{
  QString bookFilename = getFileUrl( collection );
  KUrl bookUrl( bookFilename );
  QFile bookFile( bookUrl.toLocalFile() );

  QDomDocument bookDoc;
  bookDoc.setContent( &bookFile );
  bookFile.close();
  if ( !bookFile.open( QFile::WriteOnly ) )
  {
    return;
  }

  QDomElement root = bookDoc.documentElement();
  QDomElement titleElement = root.firstChildElement( QLatin1String( "Title" ) );
  titleElement.clear();

  QString title = collection.name();

  if (collection.hasAttribute<EntityDisplayAttribute>())
    title = collection.attribute<EntityDisplayAttribute>()->displayName();

  // TODO: Handle the CollectionChildOrderAttribute

  titleElement.appendChild( bookDoc.createTextNode( title ) );

  changeCommitted( collection );
}


void KJotsResource::collectionMoved( const Akonadi::Collection &collection,
                                     const Akonadi::Collection &source,
                                     const Akonadi::Collection &destination )
{
  if ( !entityMoved( collection, source, destination ) )
    return;

  changeCommitted( collection );
}

bool KJotsResource::entityMoved( const Akonadi::Entity &entity,
                                     const Akonadi::Collection &source,
                                     const Akonadi::Collection &destination )
{
  // TODO: Maybe undo the remove if the add fails.
  if ( removeContentEntry( source, entity.remoteId() ) && addContentEntry( destination, entity.remoteId() ) )
    return true;

  return false;
}

AKONADI_RESOURCE_MAIN( KJotsResource )

#include "kjotsresource.moc"



