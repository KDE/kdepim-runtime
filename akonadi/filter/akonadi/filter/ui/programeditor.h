/****************************************************************************** * *
 *
 *  File : programeditor.h
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

#ifndef _AKONADI_FILTER_UI_PROGRAMEDITOR_H_
#define _AKONADI_FILTER_UI_PROGRAMEDITOR_H_

#include "config-akonadi-filter-ui.h"

#include "rulelisteditor.h"

namespace Akonadi
{
namespace Filter
{

class ComponentFactory;
class Program;

namespace UI
{

class EditorFactory;

/**
 * \class ProgramEditor
 *
 * The toplevel editor for the filter rules. It edits an entire filtering program.
 *
 * The ProgramEditor is customizable by the means of the EditorFactory
 * object that you pass in the constructor. The generated filter itself
 * is customizable via the ComponentFactory that you again pass in the constructor.
 */
class AKONADI_FILTER_UI_EXPORT ProgramEditor : public RuleListEditor
{
  Q_OBJECT
public:
  /**
   * Create an instance of the ProgramEditor widget with the specified
   * parent widget (which may be zero), the specified filter ComponentFactory and
   * the specified EditorFactory. Both factories must be non-null and their
   * existence must be guaranteed by the caller for the entire lifetime
   * of the ProgramEditor instance.
   */
  ProgramEditor( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory );

  /**
   * Destroys the program editor.
   */
  virtual ~ProgramEditor();

public:

  /**
   * Fills this editor from the specified Program (which may not be null).
   * The ownership of the Program object is NOT transferred.
   */
  void fillFromProgram( Program * program );
  
  /**
   * Commits the current state of the editor into a Program object.
   * The Program components will be created by using the factory you
   * have specified in the constructor.
   *
   * Returns the newly created Program object on success and 0 on failure.
   * A failure usually means that the user created some kind of error in the filter
   * (usually a bad parameter) and he must take some action in order to make the
   * filtering Program usable. In this case a message box has been shown with an indication
   * of the error and you should not close the editor (so the user can fix his mistake
   * and try to commit again).
   *
   * The ownership of the returned Program object is transferred to the caller.
   */
  virtual Program * commit();

}; // class ProgramEditor

} // namespace UI

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_UI_PROGRAMEDITOR_H_
