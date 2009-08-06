/****************************************************************************** * *
 *
 *  File : editorfactoryrfc822.cpp
 *  Created on Mon 03 Aug 2009 00:17:16 by Szymon Tomasz Stefanek
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

#include <akonadi/filter/ui/editorfactoryrfc822.h>

#include <akonadi/filter/ui/commandeditorsrfc822.h>

#include <akonadi/filter/componentfactoryrfc822.h>

#include "../commanddescriptor.h"

#include <KDebug>

#include <QtGui/QWidget>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

EditorFactoryRfc822::EditorFactoryRfc822()
{
}

EditorFactoryRfc822::~EditorFactoryRfc822()
{
}

CommandEditor * EditorFactoryRfc822::createCommandEditor( QWidget * parent, const CommandDescriptor * command, ComponentFactory * componentFactory )
{
  switch( command->id() )
  {
    case CommandRfc822MoveMessageToCollection:
    case CommandRfc822CopyMessageToCollection:
      return new CommandWithTargetCollectionEditor( parent, command, componentFactory, this );
    break;
    case CommandRfc822RunProgram:
    case CommandRfc822PipeThrough:
      return new CommandWithStringParamEditor( parent, command, componentFactory, this );
    break;
    case CommandRfc822DeleteMessage:
      return 0; // no special editor needed for this
    break;
    default:
      // fall through
    break;
  }
  return EditorFactory::createCommandEditor( parent, command, componentFactory );
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi

