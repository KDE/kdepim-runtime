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

#ifndef EVENT_GROUP_MODEL_H
#define EVENT_GROUP_MODEL_H

#include <QAbstractListModel>
#include <KCal/Incidence>

class EventWrapper;

class EventGroupModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY rowCountChanged)

public:
  enum Roles {
    EventWrapperRole
  };

  EventGroupModel(QObject* parent = 0);

  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;

  void addIncidence( KCal::Incidence::Ptr incidence );

signals:
  void rowCountChanged();

private:
  QList<EventWrapper*> m_events;
};

#endif


