/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "collectionaclpage.h"

#include <akonadi/collection.h>

using namespace Akonadi;

CollectionAclPage::CollectionAclPage(QWidget * parent) :
    CollectionPropertiesPage( parent )
{
  setPageTitle( i18n( "ACL" ) );
  ui.setupUi( this );
}

void CollectionAclPage::load(const Collection & col)
{
  Collection::Rights rights = col.rights();
  ui.changeItem->setChecked( rights & Collection::CanChangeItem );
  ui.createItem->setChecked( rights & Collection::CanCreateItem );
  ui.deleteItem->setChecked( rights & Collection::CanDeleteItem );
  ui.changeCollection->setChecked( rights & Collection::CanChangeCollection );
  ui.createCollection->setChecked( rights & Collection::CanCreateCollection );
  ui.deleteCollection->setChecked( rights & Collection::CanDeleteCollection );
}

void CollectionAclPage::save(Collection & col)
{
  Collection::Rights rights = Collection::ReadOnly;
  if ( ui.changeItem->isChecked() )
    rights |= Collection::CanChangeItem;
  if ( ui.createItem->isChecked() )
    rights |= Collection::CanCreateItem;
  if ( ui.deleteItem->isChecked() )
    rights |= Collection::CanDeleteItem;
  if ( ui.changeCollection->isChecked() )
    rights |= Collection::CanChangeCollection;
  if ( ui.createCollection->isChecked() )
    rights |= Collection::CanCreateCollection;
  if ( ui.deleteCollection->isChecked() )
    rights |= Collection::CanDeleteCollection;
  col.setRights( rights );
}

#include "collectionaclpage.moc"
