/****************************************************************************** * *
 *
 *  File : rulelisteditorlbb.h
 *  Created on Tue 04 Aug 2009 02:31:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filtering Framework
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This rulelist is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This rulelist is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the editoried warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this rulelist; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#ifndef _AKONADI_FILTER_UI_RULELISTEDITORLBB_H_
#define _AKONADI_FILTER_UI_RULELISTEDITORLBB_H_

#include <akonadi/filter/ui/config-akonadi-filter-ui.h>

#include <QtGui/QSplitter>

class QListWidget;
class QLabel;
class QPushButton;

namespace Akonadi
{
namespace Filter
{

class ComponentFactory;

namespace Action
{
  class RuleList;
} // namespace Action

namespace UI
{

class EditorFactory;
class ExpandingScrollArea;

class AKONADI_FILTER_UI_EXPORT RuleListEditorLBB : public QSplitter
{
  Q_OBJECT

public:
  RuleListEditorLBB( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory );
  virtual ~RuleListEditorLBB();

protected:
  QListWidget * mListWidget;
  ExpandingScrollArea * mScrollArea;
  QLabel * mEmptyEditor;
  QPushButton * mNewRuleButton;
  QPushButton * mDeleteRuleButton;
  QPushButton * mMoveRuleUpButton;
  QPushButton * mMoveRuleDownButton;

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

  bool autoExpand();
  void setAutoExpand( bool b );

  void fillFromRuleList( Action::RuleList * ruleList );
  bool commitStateToRuleList( Action::RuleList * ruleList );
private:
  void activateEditor( QWidget * editor );
  void reindexItems();
private Q_SLOTS:
  void slotNewRuleButtonClicked();
  void slotDeleteRuleButtonClicked();
  void slotMoveRuleUpButtonClicked();
  void slotMoveRuleDownButtonClicked();
  void slotListWidgetSelectionChanged();
  void slotRuleChanged();
};

} // namespace UI

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_UI_RULELISTEDITOR_H_
