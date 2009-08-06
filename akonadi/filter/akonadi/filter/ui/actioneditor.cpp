/****************************************************************************** * *
 *
 *  File : actioneditor.cpp
 *  Created on Fri 15 May 2009 04:53:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filtering Framework
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the editoried warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "actioneditor.h"
#include "editorfactory.h"

#include "../action.h"
#include "../componentfactory.h"

#include <QtGui/QLayout>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

ActionEditor::ActionEditor( QWidget * parent, ComponentFactory * componentFactory, EditorFactory * editorFactory )
  : QWidget( parent ), mComponentFactory( componentFactory ), mEditorFactory( editorFactory )
{
  setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
}

ActionEditor::~ActionEditor()
{
}

QSize ActionEditor::sizeHint() const
{
  if( layout() )
    return layout()->minimumSize();
  return QWidget::sizeHint();
}

QSize ActionEditor::minimumSizeHint() const
{
  if( layout() )
    return layout()->minimumSize();
  return QWidget::minimumSizeHint();
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi

