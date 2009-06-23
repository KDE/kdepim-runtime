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

#include "collectionattributespage.h"

#include <akonadi/attributefactory.h>
#include <akonadi/collection.h>

#include <kdebug.h>
#include <klocale.h>

#include <QStandardItemModel>

using namespace Akonadi;

CollectionAttributePage::CollectionAttributePage(QWidget * parent) :
    CollectionPropertiesPage( parent ),
    mModel( 0 )
{
  setPageTitle( i18n( "Attributes" ) );
  ui.setupUi( this );

  connect( ui.addButton, SIGNAL(clicked()), SLOT(addAttribute()) );
  connect( ui.deleteButton, SIGNAL(clicked()), SLOT(delAttribute()) );
}

void CollectionAttributePage::load(const Collection & col)
{
  Attribute::List list = col.attributes();
  mModel = new QStandardItemModel( list.count(), 2 );
  QStringList labels;
  labels << i18n( "Attribute" ) << i18n( "Value" );
  mModel->setHorizontalHeaderLabels( labels );

  for ( int i = 0; i < list.count(); ++i ) {
    QModelIndex index = mModel->index( i, 0 );
    Q_ASSERT( index.isValid() );
    mModel->setData( index, QString::fromLatin1( list[i]->type() ) );
    mModel->item( i, 0 )->setEditable( false );
    index = mModel->index( i, 1 );
    Q_ASSERT( index.isValid() );
    mModel->setData( index, QString::fromLatin1( list[i]->serialized() ) );
    mModel->itemFromIndex( index )->setFlags( Qt::ItemIsEditable | mModel->flags( index ) );
  }
  ui.attrView->setModel( mModel );
  connect( mModel, SIGNAL(itemChanged(QStandardItem*)), SLOT(attributeChanged(QStandardItem*)) );
}

void CollectionAttributePage::save(Collection & col)
{
  foreach ( const QString &del, mDeleted )
    col.removeAttribute( del.toLatin1() );
  for ( int i = 0; i < mModel->rowCount(); ++i ) {
    const QModelIndex typeIndex = mModel->index( i, 0 );
    Q_ASSERT( typeIndex.isValid() );
    if ( !mChanged.contains( typeIndex.data().toString() ) )
      continue;
    const QModelIndex valueIndex = mModel->index( i, 1 );
    Q_ASSERT( valueIndex.isValid() );
    Attribute* attr = AttributeFactory::createAttribute( mModel->data( typeIndex ).toString().toLatin1() );
    Q_ASSERT( attr );
    attr->deserialize( mModel->data( valueIndex ).toString().toLatin1() );
    col.addAttribute( attr );
  }
}

void CollectionAttributePage::addAttribute()
{
  if ( ui.attrName->text().isEmpty() )
    return;
  const QString attr = ui.attrName->text();
  mChanged.insert( attr );
  mDeleted.remove( attr );
  const int row = mModel->rowCount();
  mModel->insertRow( row );
  QModelIndex index = mModel->index( row, 0 );
  Q_ASSERT( index.isValid() );
  mModel->setData( index, attr );
  ui.attrName->clear();
}

void CollectionAttributePage::delAttribute()
{
  QModelIndexList selection = ui.attrView->selectionModel()->selectedRows();
  if ( selection.count() != 1 )
    return;
  const QString attr = selection.first().data().toString();
  mChanged.remove( attr );
  mDeleted.insert( attr );
  mModel->removeRow( selection.first().row() );
}

void CollectionAttributePage::attributeChanged( QStandardItem *item )
{
  const QString attr = mModel->data( mModel->index( item->row(), 0 ) ).toString();
  mDeleted.remove( attr );
  mChanged.insert( attr );
}

#include "collectionattributespage.moc"
