/*
    This file is part of libkabc.
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

#ifndef KABC_RESOURCEAKONADI_SUBRESOURCE_H
#define KABC_RESOURCEAKONADI_SUBRESOURCE_H

#include "subresourcebase.h"

namespace Akonadi {
  class Collection;
}

namespace KABC {
  class Addressee;
  class ContactGroup;
}

class IdArbiter : public IdArbiterBase
{
  protected:
    QString createArbitratedId() const;
};

class SubResource : public SubResourceBase
{
  Q_OBJECT

  public:
    explicit SubResource( const Akonadi::Collection &collection );

    ~SubResource();

    static QStringList supportedMimeTypes();

    void setCompletionWeight( int weight );

    int completionWeight() const;

  Q_SIGNALS:
    void subResourceChanged( const QString &subResourceIdentifier );

    void addresseeAdded( const KABC::Addressee &addressee, const QString &subResourceIdentifier );

    void addresseeChanged( const KABC::Addressee &addressee, const QString &subResourceIdentifier );

    void addresseeRemoved( const QString &uid, const QString &subResourceIdentifier );

    void contactGroupAdded( const KABC::ContactGroup &contactGroup, const QString &subResourceIdentifier );

    void contactGroupChanged( const KABC::ContactGroup &contactGroup, const QString &subResourceIdentifier );

    void contactGroupRemoved( const QString &uid, const QString &subResourceIdentifier );

  protected:
    int mCompletionWeight;

  protected:
    void readTypeSpecificConfig( const KConfigGroup &config );

    void writeTypeSpecificConfig( KConfigGroup &config ) const;

    void collectionChanged( const Akonadi::Collection &collection );

    void itemAdded( const Akonadi::Item &item );

    void itemChanged( const Akonadi::Item &item );

    void itemRemoved( const Akonadi::Item &item );
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
