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

#include "kviewstatemaintainer.h"

class KViewStateMaintainerBasePrivate
{
  KViewStateMaintainerBasePrivate(KSharedConfigPtr configPtr, const QString &name, KViewStateMaintainerBase *qq)
    : q_ptr(qq), m_view( 0 ), m_selectionModel( 0 ), m_configPtr(configPtr), m_name(name)
  {

  }

  Q_DECLARE_PUBLIC(KViewStateMaintainerBase)
  KViewStateMaintainerBase * const q_ptr;

  void _k_modelAboutToBeReset();
  void _k_modelReset();

  QAbstractItemView *m_view;
  QItemSelectionModel *m_selectionModel;

  const KSharedConfigPtr m_configPtr;
  const QString m_name;
};

void KViewStateMaintainerBasePrivate::_k_modelAboutToBeReset()
{
  Q_Q(KViewStateMaintainerBase);
  q->saveState();
}

void KViewStateMaintainerBasePrivate::_k_modelReset()
{
  Q_Q(KViewStateMaintainerBase);
  q->restoreState();
}

KViewStateMaintainerBase::KViewStateMaintainerBase(KSharedConfigPtr configPtr, const QString &name, QObject* parent)
  : QObject(parent), d_ptr(new KViewStateMaintainerBasePrivate(configPtr, name, this))
{

}

KConfigGroup KViewStateMaintainerBase::configGroup() const
{
  Q_D(const KViewStateMaintainerBase);
  return KConfigGroup(d->m_configPtr, d->m_name);
}

QItemSelectionModel* KViewStateMaintainerBase::selectionModel() const
{
  Q_D(const KViewStateMaintainerBase);
  return d->m_selectionModel;
}

void KViewStateMaintainerBase::setSelectionModel(QItemSelectionModel* selectionModel)
{
  Q_D(KViewStateMaintainerBase);
  d->m_selectionModel = selectionModel;

  if (d->m_view && d->m_view->model()) {
    disconnect(d->m_view->model(), SIGNAL(modelAboutToBeReset()), this, SLOT(_k_modelAboutToBeReset()));
    disconnect(d->m_view->model(), SIGNAL(modelReset()), this, SLOT(_k_modelReset()));
  }
  connect(d->m_selectionModel->model(), SIGNAL(modelAboutToBeReset()), SLOT(_k_modelAboutToBeReset()), Qt::UniqueConnection);
  connect(d->m_selectionModel->model(), SIGNAL(modelReset()), SLOT(_k_modelReset()), Qt::UniqueConnection);
}

QAbstractItemView* KViewStateMaintainerBase::view() const
{
  Q_D(const KViewStateMaintainerBase);
  return d->m_view;
}

void KViewStateMaintainerBase::setView(QAbstractItemView* view)
{
  Q_D(KViewStateMaintainerBase);
  d->m_view = view;

  if (d->m_selectionModel && d->m_selectionModel->model()) {
    disconnect(d->m_selectionModel->model(), SIGNAL(modelAboutToBeReset()), this, SLOT(_k_modelAboutToBeReset()));
    disconnect(d->m_selectionModel->model(), SIGNAL(modelReset()), this, SLOT(_k_modelReset()));
  }
  if (d->m_view && d->m_view->model()) {
    connect(d->m_view->model(), SIGNAL(modelAboutToBeReset()), SLOT(_k_modelAboutToBeReset()), Qt::UniqueConnection);
    connect(d->m_view->model(), SIGNAL(modelReset()), SLOT(_k_modelReset()), Qt::UniqueConnection);
  }
}

#include "kviewstatemaintainer.moc"
