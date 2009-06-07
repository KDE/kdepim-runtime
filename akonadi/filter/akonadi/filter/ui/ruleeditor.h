/****************************************************************************** * *
 *
 *  File : ruleeditor.h
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

#ifndef _AKONADI_FILTER_UI_RULEEDITOR_H_
#define _AKONADI_FILTER_UI_RULEEDITOR_H_

#include "config-akonadi-filter-ui.h"

#include <QWidget>

namespace Akonadi
{
namespace Filter
{

class Component;
class ComponentFactory;
class Rule;

namespace UI
{

class EditorFactory;
class ActionSelector;
class RuleEditorPrivate;

class AKONADI_FILTER_UI_EXPORT RuleEditor : public QWidget
{
  friend class ActionSelector;
  Q_OBJECT
public:
  RuleEditor( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory );
  virtual ~RuleEditor();

protected:
  ComponentFactory * mComponentFactory;
  EditorFactory * mEditorFactory;
  RuleEditorPrivate * mPrivate;

public:
  virtual void fillFromRule( Rule * rule );
  virtual Rule * commitState( Component * parent );

protected:
  void childActionSelectorTypeChanged( ActionSelector * child );
  void setSelectorCount( int count );
  void fixupVisibleSelectorList();

  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;

}; // class RuleEditor

} // namespace UI

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_UI_RULEEDITOR_H_
