/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#ifndef AMAZING_COMPLETER_H
#define AMAZING_COMPLETER_H

#include <QObject>
#include <QWidget>
#include <QAbstractItemModel>

#include "akonadi_next_export.h"

class QAbstractItemView;

class AmazingCompleterPrivate;

class AKONADI_NEXT_EXPORT AmazingCompleter : public QObject
{
  Q_OBJECT
public:

  enum ViewHandler { Popup, NoPopup };

  AmazingCompleter(/* QAbstractItemModel *model, */ QObject *parent = 0);

  void setWidget(QWidget *widget);

  void setView(QAbstractItemView *view, ViewHandler = Popup);

  void setModel(QAbstractItemModel *model);

  /**
  The data from this is put into the lineedit
  */
  void setCompletionRole(int role);

  /**
  This role is passed to match().
  */
  void setMatchingRole(int role);

  void setMinimumLength(int length);

public slots:
  void setCompletionPrefixString(const QString &matchData);
  void setCompletionPrefix(const QVariant &matchData);

  void sourceRowsInserted(const QModelIndex &parent, int start, int end);

protected:
  virtual void connectModelToView(QAbstractItemModel *model, QAbstractItemView *view);

private:
  Q_DECLARE_PRIVATE(AmazingCompleter)

  AmazingCompleterPrivate *d_ptr;

};

#endif
