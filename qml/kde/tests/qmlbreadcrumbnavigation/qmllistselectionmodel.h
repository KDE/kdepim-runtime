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

#ifndef QMLLISTSELECTIONMODEL_H
#define QMLLISTSELECTIONMODEL_H

#include <QItemSelectionModel>

class QMLListSelectionModel : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QList<int> selection READ selection NOTIFY selectionChanged)
public:
  enum SelectionFlag {
      NoUpdate       = 0x0000,
      Clear          = 0x0001,
      Select         = 0x0002,
      Deselect       = 0x0004,
      Toggle         = 0x0008,
      Current        = 0x0010,
      Rows           = 0x0020,
      Columns        = 0x0040,
      SelectCurrent  = Select | Current,
      ToggleCurrent  = Toggle | Current,
      ClearAndSelect = Clear | Select
  };
  //Q_DECLARE_FLAGS(SelectionFlags, SelectionFlag)

  explicit QMLListSelectionModel(QItemSelectionModel *selectionModel, QObject* parent = 0);
  explicit QMLListSelectionModel(QAbstractItemModel *model, QObject* parent = 0);

  QItemSelectionModel* selectionModel() const;

  QList<int> selection() const;


public slots:
  void clearSelection();
  void select(int row, int command);

signals:
  void selectionChanged();

private:
  QItemSelectionModel * const m_selectionModel;
};

#endif