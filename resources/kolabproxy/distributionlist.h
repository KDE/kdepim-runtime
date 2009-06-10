/*
    This file is part of Akonadi KolabProxy
    Copyright (c) 2009 <kevin.krammer@gmx.at>

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

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef KOLABDISTRIBUTIONLIST_H
#define KOLABDISTRIBUTIONLIST_H

#include "kolabbase.h"

namespace KABC {
  class Addressee;
  class AddressBook;
  class ContactGroup;
}

namespace Kolab {

class DistributionList : public KolabBase {
public:
  explicit DistributionList( const KABC::ContactGroup* contactGroup, KABC::AddressBook* addressBook );
  DistributionList( const QString& xml );
  ~DistributionList();

  void saveTo( KABC::ContactGroup* contactGroup );

  QString type() const { return "DistributionList"; }

  void setName( const QString& name );
  QString name() const;

  // Load the attributes of this class
  bool loadAttribute( QDomElement& );

  // Save the attributes of this class
  bool saveAttributes( QDomElement& ) const;

  // Load this note by reading the XML file
  bool loadXML( const QDomDocument& xml );

  // Serialize this note to an XML string
  QString saveXML() const;

  QString productID() const;

protected:
  void setFields( const KABC::ContactGroup*, KABC::AddressBook* );

private:
  void loadDistrListMember( const QDomElement& element );
  void saveDistrListMembers( QDomElement& element ) const;

  QString mName;

  struct Custom {
    QString app;
    QString name;
    QString value;
  };
  QList<Custom> mCustomList;

  struct Member {
    QString displayName;
    QString email;
  };
  QList<Member> mDistrListMembers;
};

}

#endif // KOLABDISTRIBUTIONLIST_H
// kate: space-indent on; indent-width 2; replace-tabs on;
