/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "xmloperations.h"
#include "global.h"
#include "test.h"

#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

#include <KDebug>

#include <QDir>
#include <QFileInfo>
#include <QStringList>

using namespace Akonadi;

template <typename T> QTextStream& operator<<( QTextStream &s, const QSet<T> &set )
{
  s << '{';
  foreach ( const T &element, set )
    s << element << ", ";
  s << '}';
  return s;
}

QTextStream& operator<<( QTextStream &s, const QStringList &list )
{
  s << '(' << list.join( ", " ) << ')';
  return s;
}

XmlOperations::XmlOperations(QObject* parent) :
  QObject( parent ),
  mCollectionFields( 0xFF ),
  mCollectionKey( RemoteId ),
  mItemFields( 0xFF ),
  mItemKey( ItemRemoteId ),
  mNormalizeRemoteIds( false )
{
}

XmlOperations::~XmlOperations()
{
}

Item XmlOperations::getItemByRemoteId(const QString& rid)
{
  return mDocument.itemByRemoteId( rid, true);
} 

Collection XmlOperations::getCollectionByRemoteId(const QString& rid)
{
  return mDocument.collectionByRemoteId(rid);
}

void XmlOperations::setRootCollections(const QString& resourceId)
{
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::FirstLevel, this );
  job->setResource( resourceId );
  if ( job->exec() )
    setRootCollections( job->collections() );
  else
    mErrorMsg = job->errorText();
}

void XmlOperations::setRootCollections(const Collection::List& roots)
{
  mRoots = roots;
}

void XmlOperations::setXmlFile(const QString& fileName)
{
  if ( QFileInfo( fileName ).isAbsolute() )
    mDocument.loadFile( fileName );
  else
    mDocument.loadFile( Global::basePath() + QDir::separator() + fileName );
}

QString XmlOperations::lastError() const
{
  return mErrorMsg;
}

void XmlOperations::setCollectionKey(XmlOperations::CollectionField field)
{
  mCollectionKey = field;
}

void XmlOperations::setCollectionKey(const QString& fieldName)
{
  const QMetaEnum me = metaObject()->enumerator( metaObject()->indexOfEnumerator( "CollectionField" ) );
  setCollectionKey( static_cast<CollectionField>( me.keyToValue( fieldName.toLatin1() ) ) );
}

void XmlOperations::ignoreCollectionField(XmlOperations::CollectionField field)
{
  mCollectionFields = mCollectionFields & ~field;
}

void XmlOperations::ignoreCollectionField(const QString& fieldName)
{
  const QMetaEnum me = metaObject()->enumerator( metaObject()->indexOfEnumerator( "CollectionField" ) );
  ignoreCollectionField( static_cast<CollectionField>( me.keyToValue( fieldName.toLatin1() ) ) );
}

void XmlOperations::setItemKey(XmlOperations::ItemField field)
{
  mItemKey = field;
}

void XmlOperations::setItemKey(const QString& _fieldName)
{
  QString fieldName = _fieldName;
  if ( !fieldName.startsWith( "Item" ) )
    fieldName.prepend( "Item" );
  const QMetaEnum me = metaObject()->enumerator( metaObject()->indexOfEnumerator( "ItemField" ) );
  setItemKey( static_cast<ItemField>( me.keyToValue( fieldName.toLatin1() ) ) );
}

void XmlOperations::ignoreItemField(XmlOperations::ItemField field)
{
  mItemFields = mItemFields & ~field;
}

void XmlOperations::ignoreItemField(const QString& _fieldName)
{
  QString fieldName = _fieldName;
  if ( !fieldName.startsWith( "Item" ) )
    fieldName.prepend( "Item" );
  const QMetaEnum me = metaObject()->enumerator( metaObject()->indexOfEnumerator( "ItemField" ) );
  ignoreItemField( static_cast<ItemField>( me.keyToValue( fieldName.toLatin1() ) ) );
}

void XmlOperations::setNormalizeRemoteIds(bool enable)
{
  mNormalizeRemoteIds = enable;
}

bool XmlOperations::compare()
{
  if ( !mDocument.isValid() ) {
    mErrorMsg = mDocument.lastError();
    return false;
  }

  if ( mRoots.isEmpty() ) {
    if ( !mErrorMsg.isEmpty() )
      mErrorMsg = QLatin1String( "No root collections specified." );
    return false;
  }

  const Collection::List docRoots = mDocument.childCollections( QString() );
  return compareCollections( mRoots, docRoots );
}

void XmlOperations::assertEqual()
{
  if ( !compare() )
    Test::instance()->fail( lastError() );
}

static QString normalizeRemoteId( const QString &in )
{
  QString out( in );
  if ( in.startsWith( Global:: basePath() ) ) {
    out = in.mid( Global::basePath().length() );
    if ( out.startsWith( QDir::separator() ) )
      out = out.mid( 1 );
  }
  return out;
}

Collection XmlOperations::normalizeCollection( const Collection &in ) const
{
  Collection out( in );
  if ( mNormalizeRemoteIds )
    out.setRemoteId( normalizeRemoteId( in.remoteId() ) );
  QStringList l = in.contentMimeTypes();
  std::sort( l.begin(), l.end() );
  out.setContentMimeTypes( l );
  return out;
}

Item XmlOperations::normalizeItem( const Item& in ) const
{
  Item out( in );
  if ( mNormalizeRemoteIds )
    out.setRemoteId( normalizeRemoteId( in.remoteId() ) );
  return out;
}

bool XmlOperations::compareCollections(const Collection::List& _cols, const Collection::List& _refCols)
{
  Collection::List cols;
  foreach ( const Collection &c, _cols )
    cols.append( normalizeCollection( c ) );
  Collection::List refCols;
  foreach ( const Collection &c, _refCols )
    refCols.append( normalizeCollection( c ) );

  switch ( mCollectionKey ) {
    case RemoteId:
      sortEntityList( cols, &Collection::remoteId );
      sortEntityList( refCols, &Collection::remoteId );
      break;
    case Name:
      sortEntityList( cols, &Collection::name );
      sortEntityList( refCols, &Collection::name );
      break;
    case None:
      break;
    default:
      Q_ASSERT( false );
  }

  for ( int i = 0; i < cols.count(); ++i ) {
    const Collection col = cols.at( i );
    if ( refCols.count() <= i ) {
      mErrorMsg = QString::fromLatin1( "Additional collection with remote id '%1' was found." ).arg( col.remoteId() );
      return false;
    }

    const Collection refCol = refCols.at( i );
    if ( !compareCollection( col, refCol ) )
      return false;
  }

  if ( refCols.count() != cols.count() ) {
    const Collection refCol = refCols.at( cols.count() );
    mErrorMsg = QString::fromLatin1( "Collection with remote id '%1' is missing." ).arg( refCol.remoteId() );
    return false;
  }

  return true;
}

bool XmlOperations::compareCollection(const Collection& col, const Collection& refCol)
{
  // compare the two collections
  if ( !compareValue( col, refCol, &Collection::remoteId, RemoteId ) ||
       !compareValue( col, refCol, &Collection::contentMimeTypes, ContentMimeType ) ||
       !compareValue( col, refCol, &Collection::name, Name ) )
    return false;

  if ( !compareAttributes( col, refCol ) )
    return false;

  // compare child items
  ItemFetchJob *ijob = new ItemFetchJob( col, this );
  ijob->fetchScope().fetchAllAttributes( true );
  ijob->fetchScope().fetchFullPayload( true );
  if ( !ijob->exec() ) {
    mErrorMsg = ijob->errorText();
    return false;
  }
  const Item::List items = ijob->items();
  const Item::List refItems = mDocument.items( refCol, true );
  if ( !compareItems( items, refItems ) )
    return false;

  // compare child collections
  CollectionFetchJob *cjob = new CollectionFetchJob( col, CollectionFetchJob::FirstLevel, this );
  if ( !cjob->exec() ) {
    mErrorMsg = cjob->errorText();
    return false;
  }
  const Collection::List cols = cjob->collections();
  const Collection::List refCols = mDocument.childCollections( refCol.remoteId() );
  return compareCollections( cols, refCols );
}

bool XmlOperations::hasItem(const Item& _item, const Collection& _col)
{
  ItemFetchJob *ijob = new ItemFetchJob( _col, this );
  ijob->fetchScope().fetchAllAttributes( true );
  ijob->fetchScope().fetchFullPayload( true );
  if ( !ijob->exec() ) {
    mErrorMsg = ijob->errorText();
    return false;
  }
  const Item::List items = ijob->items();
  
  for( int i = 0; i < items.count(); ++i) { 
    if( _item == items.at(i) ) {
      return true;
    }
  }
  return false;
} 

bool XmlOperations::hasItem(const Item& _item, const QString& rid)
{
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::FirstLevel, this );
  Collection::List colist;

  if ( job->exec() )
    colist = job->collections() ;
  foreach( const Collection &collection, colist ) {
     if(rid == collection.remoteId()) {
       return hasItem(_item, collection);
     }
  }
  return false;
}


bool XmlOperations::compareItems(const Item::List& _items, const Item::List& _refItems)
{
  Item::List items;
  foreach ( const Item &i, _items )
    items.append( normalizeItem( i ) );
  Item::List refItems;
  foreach ( const Item &i, _refItems )
    refItems.append( normalizeItem( i ) );

  switch ( mItemKey ) {
    case ItemRemoteId:
      sortEntityList( items, &Item::remoteId );
      sortEntityList( refItems, &Item::remoteId );
      break;
    case ItemNone:
      break;
    default:
      Q_ASSERT( false );
  }

  for ( int i = 0; i < items.count(); ++i ) {
    const Item item = items.at( i );
    if ( refItems.count() <= i ) {
      mErrorMsg = QString::fromLatin1( "Additional item with remote id '%1' was found." ).arg( item.remoteId() );
      return false;
    }

    const Item refItem = refItems.at( i );
    if ( !compareItem( item, refItem ) )
      return false;
  }

  if ( refItems.count() != items.count() ) {
    const Item refItem = refItems.at( items.count() );
    mErrorMsg = QString::fromLatin1( "Item with remote id '%1' is missing." ).arg( refItem.remoteId() );
    return false;
  }

  return true;
}

bool XmlOperations::compareItem(const Item& item, const Item& refItem)
{
  if ( !compareValue( item, refItem, &Item::remoteId, ItemRemoteId ) ||
       !compareValue( item, refItem, &Item::mimeType, ItemMimeType ) ||
       !compareValue( item, refItem, &Item::flags, ItemFlags ) ||
       !compareValue( item, refItem, &Item::payloadData, ItemPayload ) )
    return false;

  return compareAttributes( item, refItem );;
}

bool XmlOperations::compareAttributes(const Entity& entity, const Entity& refEntity)
{
  Attribute::List attrs = entity.attributes();
  Attribute::List refAttrs = refEntity.attributes();
  std::sort( attrs.begin(), attrs.end(), boost::bind( &Attribute::type, _1 ) < boost::bind( &Attribute::type, _2 ) );
  std::sort( refAttrs.begin(), refAttrs.end(), boost::bind( &Attribute::type, _1 ) < boost::bind( &Attribute::type, _2 ) );

  for ( int i = 0; i < attrs.count(); ++i ) {
    Attribute* attr = attrs.at( i );
    if ( refAttrs.count() <= i ) {
      mErrorMsg = QString::fromLatin1( "Additional attribute of type '%1' for object with remote id '%1' was found." )
        .arg( QString::fromLatin1( attr->type() ) ).arg( entity.remoteId() );
      return false;
    }

    Attribute* refAttr = refAttrs.at( i );
    if ( attr->type() != refAttr->type() ) {
      mErrorMsg = QString::fromLatin1( "Object with remote id '%1' misses attribute of type '%2'." )
        .arg( entity.remoteId() ).arg( QString::fromLatin1( refAttr->type() ) );
      return false;
    }

    bool result = compareValue( attr->serialized(), refAttr->serialized() );
    if ( !result ) {
      mErrorMsg.prepend( QString::fromLatin1( "Object with remote id '%1' differs in attribute '%2':\n" )
        .arg( entity.remoteId() ).arg( QString::fromLatin1( attr->type() ) ) );
      return false;
    }
  }

  if ( refAttrs.count() != attrs.count() ) {
    Attribute* refAttr = refAttrs.at( attrs.count() );
    mErrorMsg = QString::fromLatin1( "Object with remote id '%1' misses attribute of type '%2'." )
      .arg( entity.remoteId() ).arg( QString::fromLatin1( refAttr->type() ) );;
    return false;
  }

  return true;
}

#include "xmloperations.moc"
