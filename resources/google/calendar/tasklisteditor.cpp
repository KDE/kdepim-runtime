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

#include "tasklisteditor.h"
#include "ui_tasklist_editor.h"

using namespace KGoogle::Objects;

TasklistEditor::TasklistEditor( TaskList *taskList ):
  QDialog(),
  m_taskList( taskList )
{
  m_ui = new ::Ui::TaskListEditor();
  m_ui->setupUi( this );

  if ( m_taskList ) {
    m_ui->nameEdit->setText( m_taskList->title() );
  }

  connect( m_ui->buttons, SIGNAL(accepted()),
           this, SLOT(accepted()) );
}

TasklistEditor::~TasklistEditor()
{
  delete m_ui;
}

void TasklistEditor::accepted()
{
  if ( !m_taskList ) {
    m_taskList = new KGoogle::Objects::TaskList;
  }

  m_taskList->setTitle( m_ui->nameEdit->text() );

  Q_EMIT accepted( m_taskList );
}
