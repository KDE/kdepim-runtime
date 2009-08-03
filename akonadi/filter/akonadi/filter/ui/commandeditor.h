/****************************************************************************** * *
 *
 *  File : commandeditor.h
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

#ifndef _AKONADI_FILTER_UI_COMMANDACTIONEDITOR_H_
#define _AKONADI_FILTER_UI_COMMANDACTIONEDITOR_H_

#include <akonadi/filter/ui/config-akonadi-filter-ui.h>

#include <akonadi/filter/ui/actioneditor.h>

namespace Akonadi
{
namespace Filter
{

class CommandDescriptor;

namespace UI
{

class EditorFactory;

class AKONADI_FILTER_UI_EXPORT CommandEditor : public ActionEditor
{
  Q_OBJECT
public:
  CommandEditor(
      QWidget * parent,
      const CommandDescriptor * commandDescriptor,
      ComponentFactory * componentFactory,
      EditorFactory * editorFactory
    );
  virtual ~CommandEditor();

protected:
  const CommandDescriptor * mCommandDescriptor;

public:
  const CommandDescriptor * commandDescriptor()
  {
    return mCommandDescriptor;
  }

}; // class CommandEditor

} // namespace UI

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_UI_ACTIONEDITOR_H_
