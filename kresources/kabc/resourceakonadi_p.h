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

#ifndef KABC_RESOURCEAKONADI_P_H
#define KABC_RESOURCEAKONADI_P_H

#include "subresource.h"
#include "resourceakonadi.h"
#include "sharedresourceprivate.h"

namespace KABC {
  class Addressee;
  class ContactGroup;
  class DistributionList;
}

class KConfigGroup;

class KABC::ResourceAkonadi::Private : public SharedResourcePrivate<SubResource>
{
  Q_OBJECT

  public:
    explicit Private( ResourceAkonadi *parent );

    Private( const KConfigGroup &config, ResourceAkonadi *parent );

    bool insertAddressee( const KABC::Addressee &addressee );

    void removeAddressee( const KABC::Addressee &addressee );

    bool insertDistributionList( KABC::DistributionList *list );

    void removeDistributionList( KABC::DistributionList *list );

    QMap<QString, QString> uidToResourceMap() const;

  public:
    ResourceAkonadi *mParent;

  protected:
    bool mInternalDataChange;

  protected:
    bool openResource();

    bool closeResource();

    void subResourceAdded( SubResourceBase *subResource );

    void subResourceRemoved( SubResourceBase *subResource );

    void loadingResult( bool ok, const QString &errorString );

    void savingResult( bool ok, const QString &errorString );

    const SubResourceBase *storeSubResourceFromUser( const QString &uid, const QString &mimeType );

    Akonadi::Item createItem( const QString &kresId );

    Akonadi::Item updateItem( const Akonadi::Item &item, const QString &kresId, const QString &originalId );

  protected Q_SLOTS:
    void subResourceChanged( const QString &subResourceIdentifier );

    void addresseeAdded( const KABC::Addressee &addressee, const QString &subResourceIdentifier );

    void addresseeChanged( const KABC::Addressee &addressee, const QString &subResourceIdentifier );

    void addresseeRemoved( const QString &uid, const QString &subResourceIdentifier );

    void contactGroupAdded( const KABC::ContactGroup &contactGroup, const QString &subResourceIdentifier );

    void contactGroupChanged( const KABC::ContactGroup &contactGroup, const QString &subResourceIdentifier );

    void contactGroupRemoved( const QString &uid, const QString &subResourceIdentifier );

  private:
    KABC::DistributionList *distListFromContactGroup( const KABC::ContactGroup &contactGroup ) const;

    KABC::ContactGroup contactGroupFromDistList( const KABC::DistributionList* list ) const;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
