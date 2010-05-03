/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

#include "breadcrumbnavigationcontext.h"

#include <QAbstractItemModel>
#include <QDeclarativeContext>

#include "kselectionproxymodel.h"
#include "kbreadcrumbselectionmodel.h"
#include "kproxyitemselectionmodel.h"
#include "kmodelindexproxymapper.h"

#include "breadcrumbnavigation.h"

class KBreadcrumbNavigationFactoryPrivate
{
  KBreadcrumbNavigationFactoryPrivate(KBreadcrumbNavigationFactory *qq)
    : q_ptr(qq),
      m_breadcrumbSelectionModel(0),
      m_selectionModel(0),
      m_childItemsSelectionModel(0),
      m_breadcrumbModel(0),
      m_selectedItemModel(0),
      m_unfilteredChildItemsModel(0),
      m_childItemsModel(0),
      m_breadcrumbDepth(-1),
      m_modelIndexProxyMapper(0)
  {

  }
  Q_DECLARE_PUBLIC(KBreadcrumbNavigationFactory)
  KBreadcrumbNavigationFactory * const q_ptr;

  QItemSelectionModel *m_breadcrumbSelectionModel;
  QItemSelectionModel *m_selectionModel;
  QItemSelectionModel *m_childItemsSelectionModel;

  QAbstractItemModel *m_breadcrumbModel;
  QAbstractItemModel *m_selectedItemModel;
  QAbstractItemModel *m_unfilteredChildItemsModel;
  QAbstractItemModel *m_childItemsModel;
  int m_breadcrumbDepth;
  KModelIndexProxyMapper *m_modelIndexProxyMapper;
};

KBreadcrumbNavigationFactory::KBreadcrumbNavigationFactory(QObject* parent)
  : QObject(parent), d_ptr(new KBreadcrumbNavigationFactoryPrivate(this))
{

}

void KBreadcrumbNavigationFactory::createBreadcrumbContext(QAbstractItemModel *model, QObject* parent)
{
  Q_D(KBreadcrumbNavigationFactory);

  d->m_selectionModel = new QItemSelectionModel( model, parent );

  KSelectionProxyModel *currentCollectionSelectionModel = new KSelectionProxyModel( d->m_selectionModel, parent );
  currentCollectionSelectionModel->setFilterBehavior( KSelectionProxyModel::ExactSelection );
  currentCollectionSelectionModel->setSourceModel( model );
  d->m_selectedItemModel = currentCollectionSelectionModel;

  KBreadcrumbSelectionModel *breadcrumbCollectionSelection
      = new KBreadcrumbSelectionModel( d->m_selectionModel, KBreadcrumbSelectionModel::Forward, parent );
  breadcrumbCollectionSelection->setIncludeActualSelection(false);
  breadcrumbCollectionSelection->setSelectionDepth( 2 );

  KBreadcrumbNavigationProxyModel *breadcrumbNavigationModel
      = new KBreadcrumbNavigationProxyModel( breadcrumbCollectionSelection, parent );
  breadcrumbNavigationModel->setSourceModel( model );
  breadcrumbNavigationModel->setFilterBehavior( KSelectionProxyModel::ExactSelection );
  d->m_breadcrumbModel = getBreadcrumbNavigationModel(breadcrumbNavigationModel);

  KProxyItemSelectionModel *proxyBreadcrumbCollectionSelection
      = new KProxyItemSelectionModel( d->m_breadcrumbModel, d->m_selectionModel, parent );

  d->m_breadcrumbSelectionModel = new KForwardingItemSelectionModel( d->m_breadcrumbModel,
                                                                     proxyBreadcrumbCollectionSelection,
                                                                     KForwardingItemSelectionModel::Reverse,
                                                                     parent );

  // Breadcrumbs done. (phew!)

  KForwardingItemSelectionModel *oneway = new KForwardingItemSelectionModel( model, d->m_selectionModel, parent );

  KNavigatingProxyModel *navigatingProxyModel = new KNavigatingProxyModel( oneway, parent );
  navigatingProxyModel->setSourceModel( model );
  d->m_unfilteredChildItemsModel = navigatingProxyModel;

  d->m_childItemsModel = getChildItemsModel(d->m_unfilteredChildItemsModel);

  d->m_childItemsSelectionModel = new KProxyItemSelectionModel( d->m_childItemsModel, d->m_selectionModel, parent );

  d->m_modelIndexProxyMapper = new KModelIndexProxyMapper(model, d->m_childItemsModel, this);

}

QItemSelectionModel* KBreadcrumbNavigationFactory::selectionModel() const
{
  Q_D(const KBreadcrumbNavigationFactory);
  return d->m_selectionModel;
}

QAbstractItemModel* KBreadcrumbNavigationFactory::selectedItemModel() const
{
  Q_D(const KBreadcrumbNavigationFactory);
  return d->m_selectedItemModel;
}

int KBreadcrumbNavigationFactory::breadcrumbDepth() const
{
  Q_D(const KBreadcrumbNavigationFactory);
  return d->m_breadcrumbDepth;
}

void KBreadcrumbNavigationFactory::setBreadcrumbDepth(int depth)
{
  Q_D(KBreadcrumbNavigationFactory);
  d->m_breadcrumbDepth = depth;
}

QAbstractItemModel* KBreadcrumbNavigationFactory::breadcrumbItemModel() const
{
  Q_D(const KBreadcrumbNavigationFactory);
  return d->m_breadcrumbModel;
}

QItemSelectionModel* KBreadcrumbNavigationFactory::breadcrumbSelectionModel() const
{
  Q_D(const KBreadcrumbNavigationFactory);
  return d->m_breadcrumbSelectionModel;
}

QAbstractItemModel* KBreadcrumbNavigationFactory::childItemModel() const
{
  Q_D(const KBreadcrumbNavigationFactory);
  return d->m_childItemsModel;
}

QAbstractItemModel* KBreadcrumbNavigationFactory::unfilteredChildItemModel() const
{
  Q_D(const KBreadcrumbNavigationFactory);
  return d->m_unfilteredChildItemsModel;
}

QAbstractItemModel* KBreadcrumbNavigationFactory::getBreadcrumbNavigationModel(QAbstractItemModel* model)
{
  return model;
}

QAbstractItemModel* KBreadcrumbNavigationFactory::getChildItemsModel(QAbstractItemModel* model)
{
  return model;
}

QItemSelectionModel* KBreadcrumbNavigationFactory::childSelectionModel() const
{
  Q_D(const KBreadcrumbNavigationFactory);
  return d->m_childItemsSelectionModel;
}

void KBreadcrumbNavigationFactory::selectBreadcrumb(int row)
{
  Q_D(KBreadcrumbNavigationFactory);
  if ( row < 0 )
  {
    d->m_selectionModel->clearSelection();
    return;
  }
  QModelIndex index = d->m_breadcrumbModel->index( row, 0 );
  d->m_breadcrumbSelectionModel->select( QItemSelection(index, index), QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect );
}

void KBreadcrumbNavigationFactory::selectChild(int row)
{
  Q_D(KBreadcrumbNavigationFactory);
  if ( row < 0 )
  {
    d->m_selectionModel->clearSelection();
    return;
  }
  const QModelIndex index = d->m_childItemsModel->index( row, 0 );
  d->m_childItemsSelectionModel->select( QItemSelection(index, index), QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect );
}

bool KBreadcrumbNavigationFactory::childCollectionHasChildren(int row)
{
  if ( row < 0 )
    return false;

  Q_D(KBreadcrumbNavigationFactory);

  static const int column = 0;
  const QModelIndex idx = d->m_modelIndexProxyMapper->mapRightToLeft(d->m_childItemsModel->index(row, column));
  if (!idx.isValid())
    return false;

  return idx.model()->rowCount( idx ) > 0;
}

#include "breadcrumbnavigationcontext.moc"
