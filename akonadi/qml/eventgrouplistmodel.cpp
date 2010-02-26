/*
* This file is part of Akonadi
*
* Copyright 2010 Stephen Kelly <steveire@gmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#include "eventgrouplistmodel.h"

#include "eventgroupmodel.h"
#include "eventwrapper.h"
#include <akonadi/entitytreemodel.h>

#include <KCal/Incidence>

enum Role
{
  EventGroupRole = Akonadi::EntityTreeModel::UserRole
};

EventGroupListModel::EventGroupListModel(QObject* parent)
  : QAbstractListModel(parent), m_sourceModel(0)
{

  extractGroups();
}

QVariant EventGroupListModel::data(const QModelIndex& index, int role) const
{
  if (role == EventGroupRole)
  {
    return QVariant::fromValue( qobject_cast<QObject *>( m_eventGroups.at( index.row() ) ) );
  }
  if ( role == Qt::DisplayRole )
  {
    EventGroupModel *group = m_eventGroups.at( index.row() );
    int size = group->rowCount();

    if (size < 1)
      return "No events";

    QModelIndex first = group->index(0, 0);

    EventWrapper *event = qobject_cast<EventWrapper*>( first.data( EventGroupModel::EventWrapperRole ).value<QObject*>() );
    KCal::Incidence::Ptr incidence = event->incidence();
    QString summary = incidence->summary();
    if (size > 1)
    {
      // TODO: i18n
      return QString("%1, and %2 others").arg( summary).arg( size - 1 );
    }
    return summary;
  }
  return QVariant();
}

void EventGroupListModel::setSourceModel(QAbstractItemModel* sourceModel)
{
  m_sourceModel = sourceModel;

  connect(m_sourceModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)), SLOT(sourceRowsAboutToBeInserted()));
  connect(m_sourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(sourceRowsInserted()));
//   connect(m_sourceModel, SIGNAL(modelAboutToBeReset()), SLOT(sourceRowsAboutToBeInserted()));
//   connect(m_sourceModel, SIGNAL(modelReset()), SLOT(sourceRowsInserted()));

  QHash<int, QByteArray> _roleNames = sourceModel->roleNames();
  _roleNames.insert(EventGroupRole, "eventsGroup");
  setRoleNames( _roleNames );

  extractGroups();
}

void EventGroupListModel::sourceRowsAboutToBeInserted()
{
  emit beginResetModel();
  m_eventGroups.clear();
}

void EventGroupListModel::sourceRowsInserted()
{
  extractGroups();
  emit endResetModel();
  QModelIndex top = index(0, 0);
  QModelIndex bottom = index(rowCount() -1, 0);

  emit dataChanged(top, bottom);
}

void EventGroupListModel::extractGroups()
{
  int row = 0;
  m_eventGroups.clear();
  for (int i = 0; i < 30; ++i)
  {
    m_eventGroups.append( new EventGroupModel(this) );
//     m_eventGroups.last()->setI( i );
  }
  if (!m_sourceModel)
    return;
  QModelIndex index = m_sourceModel->index(row, 0);
  while (index.isValid())
  {
    Akonadi::Item item = index.data( Akonadi::EntityTreeModel::ItemRole ).value<Akonadi::Item>();
    Q_ASSERT( item.isValid() );
    Q_ASSERT( item.hasPayload<KCal::Incidence::Ptr>() );

    KCal::Incidence::Ptr incidence = item.payload<KCal::Incidence::Ptr>();
    m_eventGroups[ incidence->dtStart().dateTime().date().day() ]->addIncidence( new EventWrapper( incidence ) );

 /*
    if ( m_eventGroups.last()->matches( incidence ) )
      m_eventGroups.last()->addIncidence( incidence );
    else {
      EventGroup *group = new EventGroup(this);
      group->addIncidence( incidence );
      group->setStart( QDateTime( incidence->dtStart().dateTime().date() ) );
      group->setDays(1);
      m_eventGroups.append( group );
    }
*/
    index = m_sourceModel->index(index.row() + 1, 0);
  }
}

int EventGroupListModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent)
  // TODO: m_days
  return 30;
}

