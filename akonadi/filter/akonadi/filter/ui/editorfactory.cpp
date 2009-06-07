/****************************************************************************** * *
 *
 *  File : editorfactory.cpp
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

#include "editorfactory.h"

#include "actioneditor.h"
#include "ruleeditor.h"
#include "rulelisteditor.h"

#include <akonadi/filter/commanddescriptor.h>

#include <KDebug>

#include <QWidget>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

EditorFactory::EditorFactory()
{
}

EditorFactory::~EditorFactory()
{
}

RuleEditor * EditorFactory::createRuleEditor( QWidget * parent, ComponentFactory * componentFactory )
{
  return new RuleEditor( parent, componentFactory, this );
}

RuleListEditor * EditorFactory::createRuleListEditor( QWidget * parent, ComponentFactory * componentFactory )
{
  return new RuleListEditor( parent, componentFactory, this );
}

ActionEditor * EditorFactory::createCommandActionEditor( QWidget * parent, const CommandDescriptor * command, ComponentFactory * componentFactory )
{
  switch( command->id() )
  {
    case StandardCommandLeaveMessageOnServer:
      // this command has no parameters
      Q_ASSERT( command->parameters()->isEmpty() );
      // no need for an editor
    break;
    default:
      kWarning() << "No idea on how to create ActionEditor for command" << command->keyword();
      // If you have implemented a custom command then you probably need
      // to reimplement createCommandActionEditor() and return an appropriate ActionEditor
      // instance. If you haven't... then well.. there is a bug somewhere in this module.
      Q_ASSERT( false );
    break;
  }
  return 0; // no editor for this command
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi

