/****************************************************************************** * *
 *
 *  File : commandeditor.cpp
 *  Created on Mon 03 Aug 2009 01:35:16 by Szymon Tomasz Stefanek
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

#include <akonadi/filter/ui/commandeditor.h>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

CommandEditor::CommandEditor(
    QWidget * parent,
    const CommandDescriptor * commandDescriptor,
    ComponentFactory * componentFactory,
    EditorFactory * editorFactory
  )
  : ActionEditor( parent, componentFactory, editorFactory ),
  mCommandDescriptor( commandDescriptor )
{
  Q_ASSERT( commandDescriptor );
}

CommandEditor::~CommandEditor()
{
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi

