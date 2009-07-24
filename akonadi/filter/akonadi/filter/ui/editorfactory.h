/****************************************************************************** * *
 *
 *  File : editorfactory.h
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

#ifndef _AKONADI_FILTER_UI_EDITORFACTORY_H_
#define _AKONADI_FILTER_UI_EDITORFACTORY_H_

#include <akonadi/filter/ui/config-akonadi-filter-ui.h>

class QWidget;

#include <QtCore/QString>

namespace Akonadi
{
namespace Filter
{

class CommandDescriptor;
class ComponentFactory;

namespace UI
{

class ActionEditor;
class RuleEditor;
class RuleListEditor;

class AKONADI_FILTER_UI_EXPORT EditorFactory
{
public:
  EditorFactory();
  virtual ~EditorFactory();

protected:

  /**
   * The description of the "default" action that the filtering
   * engine will perform when the end of a filter is reached
   * without "hitting" any real action. This will usually be a no-op
   * or something non destructive anyway ("leave message on server"
   * or just "stop processing"...).
   */ 
  QString mDefaultActionDescription;

public:

  /**
   * Use this to provide a description of the default action performed by your
   * filtering engine when the end of a filter is reached without actually
   * hitting an explicit action. This will usually be a no-op
   * or something non destructive anyway ("leave message on server"
   * or just "stop processing"...).
   *
   * If you don't provide a description then i18n( "stop processing here" )
   * will be used.
   */
  void setDefaultActionDescription( const QString &defaultActionDescription )
  {
    mDefaultActionDescription = defaultActionDescription;
  }

  /**
   * Returns the description of the default action. See setDefaultActionDescription()
   * for more informations.
   */
  const QString & defaultActionDescription()
  {
    return mDefaultActionDescription;
  }

  virtual RuleEditor * createRuleEditor( QWidget * parent, ComponentFactory * componentFactory );
  virtual RuleListEditor * createRuleListEditor( QWidget * parent, ComponentFactory * componentFactory );
  virtual ActionEditor * createCommandActionEditor( QWidget * parent, const CommandDescriptor * command, ComponentFactory * componentFactory );

}; // class EditorFactory

} // namespace UI

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_UI_EDITORFACTORY_H_
