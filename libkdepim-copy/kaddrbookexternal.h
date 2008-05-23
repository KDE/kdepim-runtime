/*
  Simple Addressbook for KMail
  Copyright Stefan Taferner <taferner@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/
#ifndef KDEPIM_KADDRBOOKEXTERNAL_H
#define KDEPIM_KADDRBOOKEXTERNAL_H

#include "kdepim_export.h"
#include <kabc/addressee.h>
#include <QStringList>

class QWidget;

namespace KPIM {

class KDEPIM_EXPORT KAddrBookExternal
{
  public:
    static void addEmail( const QString &addr, QWidget *parent );
    static void addNewAddressee( QWidget * );
    static void openEmail( const QString &email, const QString &addr, QWidget *parent );
    static void openAddressBook( QWidget *parent );

    static bool addVCard( const KABC::Addressee &addressee, QWidget *parent );

    static QString expandDistributionList( const QString &listName );

  private:
    static bool addAddressee( const KABC::Addressee &addressee );
};

}

#endif
