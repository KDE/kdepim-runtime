/*
 This file is part of Akonadi.

 Copyright (c) 2009 Till Adam <adam@kde.org>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 USA.
 */

#ifndef JOBTRACKERMODEL_H_
#define JOBTRACKERMODEL_H_

#include <QtCore/QAbstractItemModel>

class JobTrackerModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  JobTrackerModel( const char *name, QObject *parent );
  virtual ~JobTrackerModel();

  /* QAIM API */
  virtual QModelIndex index(int, int, const QModelIndex&) const;
  virtual QModelIndex parent(const QModelIndex&) const;
  virtual int rowCount(const QModelIndex&) const;
  virtual int columnCount(const QModelIndex&) const;
  virtual QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
  bool isEnabled() const;
public slots:
  void resetTracker();
  void setEnabled(bool);

private:
  class Private;
  Private * const d;

};

#endif /* JOBTRACKERMODEL_H_ */
