/****************************************************************************** * *
 *
 *  File : editorfactoryrfc822.h
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

#ifndef _AKONADI_FILTER_UI_EDITORFACTORYRFC822_H_
#define _AKONADI_FILTER_UI_EDITORFACTORYRFC822_H_

#include <akonadi/filter/ui/config-akonadi-filter-ui.h>

#include <akonadi/filter/ui/editorfactory.h>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

/**
 * This class provides the editors required by the actions supported by
 * the ComponentFactoryRfc822 class.
 */
class AKONADI_FILTER_UI_EXPORT EditorFactoryRfc822 : public EditorFactory
{
public:
  EditorFactoryRfc822();
  virtual ~EditorFactoryRfc822();

public:
  /**
   * Reimplemented from EditorFactory in order to return implementations
   * or RFC822 specific action editors.
   */
  virtual CommandEditor * createCommandEditor( QWidget * parent, const CommandDescriptor * command, ComponentFactory * componentFactory );

}; // class EditorFactoryRfc822

} // namespace UI

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_UI_EDITORFACTORYRFC822_H_
