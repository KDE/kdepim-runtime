/****************************************************************************** * *
 *
 *  File : actioneditor.h
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

#ifndef _AKONADI_FILTER_UI_ACTIONEDITOR_H_
#define _AKONADI_FILTER_UI_ACTIONEDITOR_H_

#include <akonadi/filter/ui/config-akonadi-filter-ui.h>

#include <QtGui/QWidget>

namespace Akonadi
{
namespace Filter
{

namespace Action
{
  class Base;
} // namespace Action

class Component;
class ComponentFactory;

namespace UI
{

class EditorFactory;

class AKONADI_FILTER_UI_EXPORT ActionEditor : public QWidget
{
  Q_OBJECT
public:
  ActionEditor( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory );
  virtual ~ActionEditor();

protected:
  ComponentFactory * mComponentFactory;
  EditorFactory * mEditorFactory;

public:
  ComponentFactory * componentFactory()
  {
    return mComponentFactory;
  }

  EditorFactory * editorFactory()
  {
    return mEditorFactory;
  }

  virtual void fillFromAction( Action::Base * action ) = 0;
  virtual Action::Base * commitState( Component * parent ) = 0;

protected:
  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;

}; // class ActionEditor

} // namespace UI

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_UI_ACTIONEDITOR_H_
