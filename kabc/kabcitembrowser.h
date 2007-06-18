/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#ifndef KABCITEMBROWSER_H
#define KABCITEMBROWSER_H

#include <libakonadi/akonadi_export.h>
#include <libakonadi/itembrowser.h>

namespace Akonadi {
class Item;
}

class AKONADI_KABC_EXPORT KABCItemBrowser : public Akonadi::ItemBrowser
{
  public:
    KABCItemBrowser( QWidget *parent = 0 );
    virtual ~KABCItemBrowser();

  protected:
    virtual QString itemToRichText( const Akonadi::Item &item );

  private:
    class Private;
    Private* const d;

    Q_DISABLE_COPY( KABCItemBrowser )
};

#endif
