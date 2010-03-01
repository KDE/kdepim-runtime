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

#include "eventgroupmodel.h"

EventGroupModel::EventGroupModel(QObject* parent)
  : QAbstractListModel(parent)
{
  QHash<int, QByteArray> _roleNames = roleNames();
  _roleNames.insert(EventWrapperRole, "data");
  setRoleNames( _roleNames );
}

QVariant EventGroupModel::data(const QModelIndex& index, int role) const
{
  if (role == EventWrapperRole)
  {
    return QVariant::fromValue( qobject_cast<QObject *>( m_events.at( index.row() ) ) );
  }

  if ( role == Qt::DisplayRole )
  {
    QObject *event = m_events.at( index.row() );
    return event->property("summary");
  }
  return QVariant();
}

int EventGroupModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent)
  return m_events.size();
}

void EventGroupModel::addIncidence(QObject *eventWrapper)
{
  eventWrapper->setParent(this);
  m_events.append( eventWrapper );
}


