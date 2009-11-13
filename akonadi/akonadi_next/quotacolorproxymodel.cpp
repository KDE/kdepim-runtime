/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>


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

#include "quotacolorproxymodel.h"

#include <akonadi/entitytreemodel.h>
#include <akonadi/collectionquotaattribute.h>

#include <QtGui/QColor>

using namespace Akonadi;

/**
 * @internal
 */
class QuotaColorProxyModel::Private
{
  public:
    Private( QuotaColorProxyModel *parent )
      : mParent( parent ), mThreshold( 100.0 ), mColor( Qt::red )
    {
    }

    QuotaColorProxyModel *mParent;

    qreal mThreshold;
    QColor mColor;
};

QuotaColorProxyModel::QuotaColorProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ),
    d( new Private( this ) )
{
}

QuotaColorProxyModel::~QuotaColorProxyModel()
{
  delete d;
}

void QuotaColorProxyModel::setWarningThreshold( qreal threshold )
{
  d->mThreshold = threshold;
}

qreal QuotaColorProxyModel::warningThreshold() const
{
  return d->mThreshold;
}

void QuotaColorProxyModel::setWarningColor( const QColor &color )
{
  d->mColor = color;
}

QColor QuotaColorProxyModel::warningColor() const
{
  return d->mColor;
}

QVariant QuotaColorProxyModel::data( const QModelIndex & index, int role) const
{
  if ( role == Qt::ForegroundRole ) {
    const QModelIndex sourceIndex = mapToSource( index );
    const QModelIndex rowIndex = sourceIndex.sibling( sourceIndex.row(), 0 );
    Collection collection = sourceModel()->data( rowIndex, EntityTreeModel::CollectionRole ).value<Collection>();

    if ( collection.isValid() && collection.hasAttribute<CollectionQuotaAttribute>() ) {
      CollectionQuotaAttribute *quota = collection.attribute<CollectionQuotaAttribute>();

      if ( quota->currentValue() > -1 && quota->maximumValue() > 0 ) {
        qreal percentage = ( 100.0 * quota->currentValue() ) / quota->maximumValue();

        if ( percentage >= d->mThreshold ) {
          return d->mColor;
        }
      }
    }
  }

  return QSortFilterProxyModel::data( index, role );
}

#include "quotacolorproxymodel.moc"

