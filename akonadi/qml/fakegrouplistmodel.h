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

#ifndef FAKE_EVENT_GROUP_LIST_MODEL_H
#define FAKE_EVENT_GROUP_LIST_MODEL_H

#include <QAbstractListModel>

class EventGroupModel;

class FakeEventWrapper : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged)
  Q_PROPERTY(QString location READ location NOTIFY locationChanged)
  Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)

public:
  FakeEventWrapper(const QVariantList data, QObject* parent = 0);

  QString summary() const;
  QString location() const;
  QString description() const;

signals:
  void summaryChanged();
  void locationChanged();
  void descriptionChanged();

private:
  QVariantList m_data;
};

class FakeEventGroupListModel : public QAbstractListModel
{
  Q_OBJECT

public:
  explicit FakeEventGroupListModel(QObject* parent = 0);

  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;

private:
  void initData();

private slots:
  void sourceRowsAboutToBeInserted();
  void sourceRowsInserted();

private:
  QList<EventGroupModel *> m_eventGroups;
};

#endif


