/*
    Copyright (c) 2008 Bruno Virlet <bvirlet@kdemail.net>

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

#ifndef KCALITEMBROWSER_H
#define KCALITEMBROWSER_H

#include "akonadi-kcal_export.h"
#include "itembrowser.h"

namespace Akonadi {
class Item;

class AKONADI_KCAL_EXPORT KCalItemBrowser : public ItemBrowser
{
  public:
    KCalItemBrowser( QWidget *parent = 0 );
    virtual ~KCalItemBrowser();

  protected:
    virtual QString itemToRichText( const Item &item );

  private:
    class Private;
    Private* const d;

    Q_DISABLE_COPY( KCalItemBrowser )
};

}

#endif

