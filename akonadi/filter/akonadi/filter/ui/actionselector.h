/****************************************************************************** * *
 *
 *  File : actionselector.h
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

#ifndef _AKONADI_FILTER_UI_ACTIONSELECTOR_H_
#define _AKONADI_FILTER_UI_ACTIONSELECTOR_H_

#include <akonadi/filter/ui/config-akonadi-filter-ui.h>

#include <QtGui/QWidget>

#include <akonadi/filter/action.h>

namespace Akonadi
{
namespace Filter
{

class Component;
class ComponentFactory;

namespace UI
{

class EditorFactory;
class RuleEditor;

class ActionSelectorPrivate;
class ActionDescriptor;

class AKONADI_FILTER_UI_EXPORT ActionSelector : public QWidget
{
  Q_OBJECT
public:
  ActionSelector( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory, RuleEditor * ruleEditor );
  virtual ~ActionSelector();

protected:
  ComponentFactory * mComponentFactory;
  EditorFactory * mEditorFactory;
  ActionSelectorPrivate * mPrivate;
  RuleEditor * mRuleEditor;

public:
  virtual void fillFromAction( Action::Base * action );
  virtual Action::Base * commitState( Component * parent );
  Action::ActionType currentActionType();
  bool currentActionIsTerminal();
protected slots:
  void typeComboBoxActivated( int index );
private:
  ActionDescriptor * descriptorForActiveType();
  void setupUIForActiveType();

}; // class ActionSelector

} // namespace UI

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_UI_ACTIONSELECTOR_H_
