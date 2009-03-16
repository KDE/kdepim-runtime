/*
    This file is part of kdepim.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KRES_AKONADI_SUBRESOURCEBASE_H
#define KRES_AKONADI_SUBRESOURCEBASE_H

#include <akonadi/item.h>
#include <akonadi/collection.h>

#include <QtCore/QHash>
#include <QtCore/QObject>

class KConfigGroup;

class IdArbiterBase
{
  public:
    virtual ~IdArbiterBase();

    QString arbitrateOriginalId( const QString &originalId );

    QString removeArbitratedId( const QString &arbitratedId );

    QString mapToOriginalId( const QString &arbitratedId ) const;

    void clear();

  protected:
    typedef QHash<QString, QString> IdMapping;
    IdMapping mOriginalToArbitrated;
    IdMapping mArbitratedToOriginal;

  protected:
    QString mapToArbitratedId( const QString &originalId ) const;

    virtual QString createArbitratedId() const = 0;
};

class SubResourceBase : public QObject
{
  Q_OBJECT

  public:
    typedef QHash<Akonadi::Item::Id, Akonadi::Item> ItemsByItemId;
    typedef QHash<QString, Akonadi::Item> ItemsByKResId;
    typedef QHash<Akonadi::Item::Id, QString> KResIdByItemId;

    explicit SubResourceBase( const Akonadi::Collection &collection );

    virtual ~SubResourceBase();

    void setIdArbiter( IdArbiterBase *idArbiter );

    QString label() const;

    void setActive( bool active );

    bool isActive() const;

    bool isWritable() const;

    QString subResourceIdentifier() const;

    void readConfig( const KConfigGroup &config );

    void writeConfig( KConfigGroup &config ) const;

    void changeCollection( const Akonadi::Collection &collection );

    void addItem( const Akonadi::Item &item );

    void changeItem( const Akonadi::Item &item );

    void removeItem( const Akonadi::Item &item );

    bool hasMappedItem( const QString &kresId ) const;

    Akonadi::Item mappedItem( const QString &kresId ) const;

    Akonadi::Collection collection() const;

  protected:
    Akonadi::Collection mCollection;
    bool mActive;

    ItemsByItemId mItems;

    IdArbiterBase *mIdArbiter;

    ItemsByKResId mMappedItems;
    KResIdByItemId mMappedIds;

  protected:
    virtual void readTypeSpecificConfig( const KConfigGroup &config );

    virtual void writeTypeSpecificConfig( KConfigGroup &config ) const;

    virtual void collectionChanged( const Akonadi::Collection &collection );

    virtual void itemAdded( const Akonadi::Item &item ) = 0;

    virtual void itemChanged( const Akonadi::Item &item ) = 0;

    virtual void itemRemoved( const Akonadi::Item &item ) = 0;

    static QString label( const Akonadi::Collection &collection );

    static bool isWritable( const Akonadi::Collection &collection );
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
