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

#ifndef AKONADI_XMLCOMPARATOR_H
#define AKONADI_XMLCOMPARATOR_H

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/xml/xmldocument.h>

#include <QtCore/QMetaEnum>
#include <QtCore/QObject>
#include <QtCore/QTextStream>

#include <boost/bind.hpp>
#include <algorithm>


/**
  Compares a Akonadi collection sub-tree with reference data supplied in an XML file.
*/
class XmlOperations : public QObject
{
  Q_OBJECT
  Q_ENUMS( CollectionField ItemField )

  public:
    XmlOperations( QObject *parent = 0 );
    ~XmlOperations();

    enum CollectionField {
      None = 0,
      RemoteId = 1,
      Name = 2,
      ContentMimeType = 4
    };

    enum ItemField {
      ItemNone = 0,
      ItemRemoteId = 1,
      ItemMimeType = 2,
      ItemFlags = 4,
      ItemPayload = 8
    };

    Q_DECLARE_FLAGS( CollectionFields, CollectionField )
    Q_DECLARE_FLAGS( ItemFields, ItemField )

    void setCollectionKey( CollectionField field );
    void ignoreCollectionField( CollectionField field );
    void setItemKey( ItemField field );
    void ignoreItemField( ItemField field );

  public slots:
    void setRootCollections( const QString &resourceId );
    void setRootCollections( const Akonadi::Collection::List &roots );
    void setXmlFile( const QString &fileName );

    Akonadi::Item getItemByRemoteId(const QString& rid);
    Akonadi::Collection getCollectionByRemoteId(const QString& rid);

    void setCollectionKey( const QString &fieldName );
    void ignoreCollectionField( const QString &fieldName );
    void setItemKey( const QString &fieldName );
    void ignoreItemField( const QString &fieldName );

    void setNormalizeRemoteIds( bool enable );

    bool compare();
    void assertEqual();

    QString lastError() const;

    bool compareCollections( const Akonadi::Collection::List &cols, const Akonadi::Collection::List &refCols );
    bool compareCollection( const Akonadi::Collection &col, const Akonadi::Collection &refCol );
    bool compareItems( const Akonadi::Item::List &items, const Akonadi::Item::List &refItems );
    bool compareItem( const Akonadi::Item &item, const Akonadi::Item &refItem );
    bool compareAttributes( const Akonadi::Entity &entity, const Akonadi::Entity &refEntity );
    bool hasItem(const Akonadi::Item& _item, const Akonadi::Collection& _col);
    bool hasItem(const Akonadi::Item& _item, const QString& rid);

  private:
    template <typename T, typename P>
    bool compareValue( const Akonadi::Collection &col, const Akonadi::Collection &refCol,
                       T (P::*property)() const, CollectionField propertyType );
    template <typename T, typename P>
    bool compareValue( const Akonadi::Item& item, const Akonadi::Item& refItem,
                       T (P::*property)() const, ItemField propertyType );
    template <typename T> bool compareValue( const T& value, const T& refValue );

    template <typename T, typename E, typename P>
    void sortEntityList( QList<E> &list, T ( P::*property)() const ) const;


    Akonadi::Collection normalizeCollection( const Akonadi::Collection &in ) const;
    Akonadi::Item normalizeItem( const Akonadi::Item &in ) const;

  private:
    Akonadi::Collection::List mRoots;
    Akonadi::XmlDocument mDocument;
    QString mErrorMsg;
    CollectionFields mCollectionFields;
    CollectionField mCollectionKey;
    ItemFields mItemFields;
    ItemField mItemKey;
    bool mNormalizeRemoteIds;
};


template <typename T, typename P>
bool XmlOperations::compareValue( const Akonadi::Collection& col, const Akonadi::Collection& refCol,
                                  T (P::*property)() const,
                                  CollectionField propertyType )
{
  if ( mCollectionFields & propertyType ) {
    const bool result = compareValue<T>( ((col).*(property))(), ((refCol).*(property))() );
    if ( !result ) {
      const QMetaEnum me = metaObject()->enumerator( metaObject()->indexOfEnumerator( "CollectionField" ) );
      mErrorMsg.prepend( QString::fromLatin1( "Collection with remote id '%1' differs in property '%2':\n" )
      .arg( col.remoteId() ).arg( me.valueToKey( propertyType ) ) );
    }
    return result;
  }
  return true;
}

template <typename T, typename P>
bool XmlOperations::compareValue( const Akonadi::Item& item, const Akonadi::Item& refItem,
                                  T (P::*property)() const,
                                  ItemField propertyType )
{
  if ( mItemFields & propertyType ) {
    const bool result = compareValue<T>( ((item).*(property))(), ((refItem).*(property))() );
    if ( !result ) {
      const QMetaEnum me = metaObject()->enumerator( metaObject()->indexOfEnumerator( "ItemField" ) );
      mErrorMsg.prepend( QString::fromLatin1( "Item with remote id '%1' differs in property '%2':\n" )
        .arg( item.remoteId() ).arg( me.valueToKey( propertyType ) ) );
    }
    return result;
  }
  return true;
}

template <typename T>
bool XmlOperations::compareValue(const T& value, const T& refValue )
{
  if ( value == refValue )
    return true;
  QTextStream ts( &mErrorMsg );
  ts << " Actual: " << value << endl << " Expected: " << refValue;
  return false;
}

template <typename T, typename E, typename P>
void XmlOperations::sortEntityList( QList<E> &list, T ( P::*property)() const ) const
{
  std::sort( list.begin(), list.end(), boost::bind( property, _1 ) < boost::bind( property, _2 ) );
}


#endif
