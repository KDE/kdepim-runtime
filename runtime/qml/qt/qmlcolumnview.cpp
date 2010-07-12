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

#include "qmlcolumnview.h"

#include "akonadi/akonadi_next/checkableitemproxymodel.h"
#include <akonadi/akonadi_next/kproxyitemselectionmodel.h>

using namespace Qt;

QmlColumnView::QmlColumnView(QDeclarativeItem* parent)
  : DeclarativeWidgetWrapper< QColumnView >(parent)
{
  m_checkableProxyModel = new CheckableItemProxyModel( this );
  m_widget->setModel(m_checkableProxyModel);
}

QObject* QmlColumnView::model() const
{
  return m_checkableProxyModel->sourceModel();
}

void QmlColumnView::setModel(QObject* model)
{
  QAbstractItemModel *_model = qobject_cast<QAbstractItemModel *>(model);
  if (!_model)
    return;
  m_checkableProxyModel->setSourceModel(_model);
}

QObject* QmlColumnView::selectionModel() const
{
  return m_widget->selectionModel();
}

void QmlColumnView::setSelectionModel(QObject* model)
{
  QItemSelectionModel *_model = qobject_cast<QItemSelectionModel*>(model);
  if (!_model)
    return;

  Future::KProxyItemSelectionModel *proxySelectionModel = new Future::KProxyItemSelectionModel(m_checkableProxyModel->sourceModel(), _model);

  Q_ASSERT(proxySelectionModel->model() == m_checkableProxyModel->sourceModel());
  m_checkableProxyModel->setSelectionModel(proxySelectionModel);
}

#include "qmlcolumnview.moc"
