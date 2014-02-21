/*
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 *   Copyright (C) 2014 by Heena Mahour <heena393@gmail.com>
 */

#ifndef TODOTREEVIEW_H
#define TODOTREEVIEW_H

#include <QTreeView>

class QModelIndex;
class QMouseEvent;

class TodoTreeView : public QTreeView
{
    Q_OBJECT

public:
    TodoTreeView(QWidget* parent = 0);
    ~TodoTreeView();

    QModelIndex indexAtCursor();
    QString summaryAtCursor();

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

signals:
    void tooltipUpdated(QString);

private:
    QString tip;
    QModelIndex idx;
};

#endif
