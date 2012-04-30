/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef GOOGLE_CALENDAR_TASKLISTEDITOR_H
#define GOOGLE_CALENDAR_TASKLISTEDITOR_H

#include <QtGui/QDialog>

#include <libkgoogle/objects/tasklist.h>

namespace Ui {
  class TaskListEditor;
}

class TasklistEditor : public QDialog
{
  Q_OBJECT

  public:
    explicit TasklistEditor( KGoogle::Objects::TaskList *taskList = 0 );

    virtual ~TasklistEditor();

  Q_SIGNALS:
    void accepted( KGoogle::Objects::TaskList *taskList );

  private Q_SLOTS:
    void accepted();

  private:
    KGoogle::Objects::TaskList *m_taskList;
    Ui::TaskListEditor *m_ui;
};

#endif // TASKLISTEDITOR_H
