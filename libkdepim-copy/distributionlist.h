/*
    This file is part of libkdepim.
    Copyright (c) 2004-2005 David Faure <faure@kde.org>

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

#ifndef DISTRIBUTIONLIST_H
#define DISTRIBUTIONLIST_H

#include <kabc/addressee.h>

namespace KABC {
class AddressBook;
}

namespace KPIM {

/**
 * @short Distribution list of email addresses
 *
 * This class represents a list of email addresses. Each email address is
 * associated with an address book entry. If the address book entry changes, the
 * entry in the distribution list is automatically updated.
 *
 * This should go into kdelibs in KDE4.
 *
 * @author David Faure <faure@kde.org>
 */
class DistributionList : public KABC::Addressee
{
  public:
    /**
     * @short Distribution List Entry
     *
     * This class represents an entry of a distribution list. It consists of an
     * addressee and an email address. If the email address is null, the
     * preferred email address of the addressee is used.
     */
    struct Entry
    {
      typedef QList<Entry> List;

      Entry() {}
      Entry( const Addressee &_addressee, const QString &_email ) :
          addressee( _addressee ), email( _email ) {}

      Addressee addressee;
      QString email;
    };

    typedef QList<DistributionList> List;

    /**
     * Create a distribution list.
     */
    DistributionList();
    /**
     * Create a distribution list from an addressee object
     * (this is a kind of down-cast)
     */
    DistributionList( const KABC::Addressee& addr );

    /**
     * Destructor.
     */
    ~DistributionList() {}

    /// HACK: reimplemented from Addressee, but it's NOT virtual there
    void setName( const QString &name );

    /// HACK: reimplemented from Addressee, but it's NOT virtual there
    QString name() const { return formattedName(); }

    /**
      Insert an entry into this distribution list. If the entry already exists
      nothing happens.
    */
    void insertEntry( const Addressee &, const QString &email=QString() );

    /**
      Remove an entry from this distribution list. If the entry doesn't exist
      nothing happens.
    */
    void removeEntry( const Addressee &, const QString &email=QString() );

    /// Overload, used by resources to avoid looking up the addressee
    void insertEntry( const QString& uid, const QString& email=QString() );
    /// Overload, used by resources to avoid looking up the addressee
    void removeEntry( const QString& uid, const QString& email=QString() );


    /**
      Return list of email addresses, which belong to this distributon list.
      These addresses can be directly used by e.g. a mail client.
      @param book necessary to look up entries
    */
    QStringList emails( KABC::AddressBook* book ) const;

    /**
      Return list of entries belonging to this distribution list. This function
      is mainly useful for a distribution list editor.
      @param book necessary to look up entries
    */
    Entry::List entries( KABC::AddressBook* book ) const;

    // KDE4: should be a method of Addressee
    static bool isDistributionList( const KABC::Addressee& addr );

    // KDE4: should be a method of AddressBook
    static DistributionList findByName( KABC::AddressBook* book,
                                        const QString& name,
                                        bool caseSensitive = true );
    // KDE4: should be a method of AddressBook
    // A bit slow (but no more than findByName).
    // From KAddressbook, use Core::distributionLists() instead.
    static QList<DistributionList> allDistributionLists( KABC::AddressBook* book );


  private:
    // can't have any data here, use Addressee's methods instead
};

}

#endif /* DISTRIBUTIONLIST_H */

