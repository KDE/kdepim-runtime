/*
  This file is part of the Grantlee template system.

  Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 3 only, as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License version 3 for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "akonaditemplateresource.h"
#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>

using namespace Akonadi;

TemplateResource::TemplateResource( const QString& id ): ResourceBase( id )
{
  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().fetchFullPayload();

  m_rootDir = QDir( "/home/kde-devel/kde/src/playground/pim/akonadi/example_mail/themes/" );
}

TemplateResource::~TemplateResource()
{

}

void TemplateResource::configure( WId windowId )
{
//Akonadi::AgentBase::configure(windowId);

  synchronize();
}


void TemplateResource::aboutToQuit()
{
  Akonadi::AgentBase::aboutToQuit();
}

void TemplateResource::retrieveCollections()
{
  Collection::List list;

  Collection rootCol;
  rootCol.setRemoteId( m_rootDir.canonicalPath() );
  rootCol.setParent( Collection::root() );
  rootCol.setName( "m_rootDir" );
  rootCol.setContentMimeTypes( QStringList() << Collection::mimeType()
                               << "text/x-vnd.grantlee-template" );

  list << rootCol;

  const QStringList dirs = m_rootDir.entryList( QDir::AllDirs );
  foreach( const QString& dir, dirs ) {
    if ( dir.startsWith( "." ) )
      continue;
    Collection col;
    col.setRemoteId( dir );
    col.setParent( rootCol );
    col.setName( dir );
    // Add recursive...
    list << col;
  }

  collectionsRetrieved( list );
}

void TemplateResource::retrieveItems( const Akonadi::Collection& col )
{
  QDir dir( m_rootDir.canonicalPath() + "/" + col.remoteId() );
  const QStringList files = dir.entryList(QStringList() << "*.html", QDir::Files );
  Item::List list;
  foreach(const  QString& filename, files ) {
    Item item;
    item.setRemoteId( filename );
    item.setMimeType( "text/x-vnd.grantlee-template" );
    list << item;
  }
  itemsRetrieved( list );
}

bool TemplateResource::retrieveItem( const Akonadi::Item& item, const QSet< QByteArray >& parts )
{
  QFile file( m_rootDir.canonicalPath() + "/default/" + item.remoteId() );
  if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    return false;
  Item retrievedItem = item;
  retrievedItem.setPayloadFromData( file.readAll() );

  itemRetrieved( retrievedItem );
  return true;
}

void TemplateResource::collectionAdded( const Akonadi::Collection& collection, const Akonadi::Collection& parent )
{
  Akonadi::AgentBase::Observer::collectionAdded( collection, parent );
}


void TemplateResource::collectionChanged( const Akonadi::Collection& collection )
{
  Akonadi::AgentBase::Observer::collectionChanged( collection );
}

void TemplateResource::collectionRemoved( const Akonadi::Collection& collection )
{
  Akonadi::AgentBase::Observer::collectionRemoved( collection );
}

void TemplateResource::collectionMoved( const Akonadi::Collection& collection, const Akonadi::Collection& source, const Akonadi::Collection& destination )
{

}

void TemplateResource::itemAdded( const Akonadi::Item& item, const Akonadi::Collection& collection )
{
  Akonadi::AgentBase::Observer::itemAdded( item, collection );
}

void TemplateResource::itemChanged( const Akonadi::Item& item, const QSet< QByteArray >& parts )
{
  // item.ancestorEntities() ...

  Item newItem = item;
  changeCommitted( newItem );
}

void TemplateResource::itemMoved( const Akonadi::Item& item, const Akonadi::Collection& source, const Akonadi::Collection& destination )
{

}

void TemplateResource::itemRemoved( const Akonadi::Item& item )
{
  Akonadi::AgentBase::Observer::itemRemoved( item );
}


AKONADI_RESOURCE_MAIN( TemplateResource )

#include "akonaditemplateresource.moc"

