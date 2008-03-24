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

#include "collectioninternalspage.h"

#include <akonadi/collection.h>

using namespace Akonadi;

CollectionInternalsPage::CollectionInternalsPage(QWidget * parent) :
    CollectionPropertiesPage( parent )
{
  setPageTitle( i18n( "Internals" ) );
  ui.setupUi( this );
}

void CollectionInternalsPage::load(const Akonadi::Collection & col)
{
  ui.idLabel->setText( QString::number( col.id() ) );
  ui.ridEdit->setText( col.remoteId() );
  ui.resourceLabel->setText( col.resource() );
  ui.contentTypes->setItems( col.contentTypes() );
}

void CollectionInternalsPage::save(Akonadi::Collection & col)
{
  col.setRemoteId( ui.ridEdit->text() );
  col.setContentTypes( ui.contentTypes->items() );
}

#include "collectioninternalspage.moc"
