/*
  This file is part of libkdepim.

  Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

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

#ifndef KPIM_ADDRESSEE_EMAILSELECTION_H
#define KPIM_ADDRESSEE_EMAILSELECTION_H

#include "addresseeselector.h"
#include <QPixmap>

namespace KPIM {

class KDEPIM_EXPORT AddresseeEmailSelection : public Selection
{
  public:
    AddresseeEmailSelection();

    /**
      Returns the number of fields the selection offers.
     */
    virtual int fieldCount() const;

    /**
      Returns the title for the field specified by index.
     */
    virtual QString fieldTitle( int index ) const;

    /**
      Returns the number of items for the given addressee.
     */
    virtual int itemCount( const KABC::Addressee &addresse ) const;

    /**
      Returns the text that's used for the item specified by index.
     */
    virtual QString itemText( const KABC::Addressee &addresse, int index ) const;

    /**
      Returns the icon that's used for the item specified by index.
     */
    virtual QPixmap itemIcon( const KABC::Addressee &addresse, int index ) const;

    /**
      Returns whether the item specified by index is enabled.
     */
    virtual bool itemEnabled( const KABC::Addressee &addresse, int index ) const;

    /**
      Returns whether the item specified by index matches the passed pattern.
     */
    virtual bool itemMatches( const KABC::Addressee &addresse, int index,
                              const QString &pattern ) const;

    /**
      Returns whether the item specified by index equals the passed pattern.
     */
    virtual bool itemEquals( const KABC::Addressee &addresse, int index,
                             const QString &pattern ) const;

    /**
      Returns the text that's used for the given distribution list.
     */
    virtual QString distributionListText( const KABC::DistributionList *distributionList ) const;

    /**
      Returns the icon that's used for the given distribution list.
     */
    virtual QPixmap distributionListIcon( const KABC::DistributionList *distributionList ) const;

    /**
      Returns whether the given distribution list is enabled.
     */
    virtual bool distributionListEnabled( const KABC::DistributionList *distributionList ) const;

    /**
      Returns whether the given distribution list matches the passed pattern.
     */
    virtual bool distributionListMatches( const KABC::DistributionList *distributionList,
                                          const QString &pattern ) const;

    /**
      Returns the number of additional address books.
     */
    virtual int addressBookCount() const;

    /**
      Returns the title for an additional address book.
     */
    virtual QString addressBookTitle( int index ) const;

    /**
      Returns the content for an additional address book.
     */
    virtual KABC::Addressee::List addressBookContent( int index ) const;

    QStringList to() const;
    QStringList cc() const;
    QStringList bcc() const;

    KABC::Addressee::List toAddresses() const;
    KABC::Addressee::List ccAddresses() const;
    KABC::Addressee::List bccAddresses() const;

    QStringList toDistributionLists() const;
    QStringList ccDistributionLists() const;
    QStringList bccDistributionLists() const;

    void setSelectedTo( const QStringList &emails );
    void setSelectedCC( const QStringList &emails );
    void setSelectedBCC( const QStringList &emails );

  private:
    virtual void addSelectedAddressees( int fieldIndex,
                                        const KABC::Addressee &addressee,
                                        int itemIndex );
    virtual void addSelectedDistributionList( int fieldIndex,
                                              const KABC::DistributionList *list );

    QString email( const KABC::Addressee &addressee, int ) const;
    void setSelectedItem( int fieldIndex, const QStringList &emails );

    KABC::Addressee::List mToAddresseeList;
    KABC::Addressee::List mCcAddresseeList;
    KABC::Addressee::List mBccAddresseeList;

    QStringList mToEmailList;
    QStringList mCcEmailList;
    QStringList mBccEmailList;

    QStringList mToDistributionList;
    QStringList mCcDistributionList;
    QStringList mBccDistributionList;
};

}

#endif
