/*
    Copyright (c) 2009 Andras Mantia <amantia@kde.org>
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#ifndef ADDRESSBOOKHANDLER_H
#define ADDRESSBOOKHANDLER_H

#include "kolabhandler.h"

namespace KABC{
  class AddressBook;
  class Addressee;
  class ContactGroup;
}

namespace Kolab {
  class Contact;
  class DistributionList;
}

/**
	@author Andras Mantia <amantia@kde.org>
*/
class AddressBookHandler : public KolabHandler {
public:
    AddressBookHandler();

    virtual ~AddressBookHandler();

    virtual Akonadi::Item::List translateItems(const Akonadi::Item::List & addrs);
    virtual void toKolabFormat(const Akonadi::Item& item, Akonadi::Item &imapItem);
    virtual QStringList contentMimeTypes();

private:
    bool addresseFromKolab(MessagePtr data, KABC::Addressee &addressee);

    bool contactGroupFromKolab(MessagePtr data, KABC::ContactGroup &contactGroup);

    void contactToKolabFormat(const Kolab::Contact &contact, Akonadi::Item &imapItem);

    void distListToKolabFormat(const Kolab::DistributionList &distList, Akonadi::Item &imapItem);

    KABC::AddressBook* mAddressBook;
};

#endif
