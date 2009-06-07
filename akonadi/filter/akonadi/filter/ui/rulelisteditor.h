/****************************************************************************** * *
 *
 *  File : rulelisteditor.h
 *  Created on Fri 15 May 2009 04:53:16 by Szymon Tomasz Stefanek
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

#ifndef _AKONADI_FILTER_UI_RULELISTEDITOR_H_
#define _AKONADI_FILTER_UI_RULELISTEDITOR_H_

#include "config-akonadi-filter-ui.h"

#include "actioneditor.h"

#include <QWidget>
#include <QScrollArea>

#include "expandingscrollarea.h"

class QLabel;
class QLineEdit;
class QFrame;
class QHBoxLayout;
class QToolButton;

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

class RuleEditor;

class RuleListEditorHeader : public QWidget
{
  Q_OBJECT

public:
  RuleListEditorHeader( QWidget * parent );
  virtual ~RuleListEditorHeader();

protected:
  QString mText;
  QLabel * mIconLabel;
  QLabel * mTextLabel;

  QHBoxLayout * mLayout;
public:
  void setText( const QString &text );
  QString text();
  void setIcon( const QPixmap &pixmap );

protected:
  void addWidgetToLayout( QWidget * widget );
  virtual void mouseReleaseEvent( QMouseEvent *e );
  virtual void paintEvent( QPaintEvent *e );
  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;

Q_SIGNALS:
  void activated( RuleListEditorHeader * header );
  
};

class RuleListEditorItemHeader : public RuleListEditorHeader
{
  Q_OBJECT

public:
  RuleListEditorItemHeader( QWidget *parent );
  virtual ~RuleListEditorItemHeader();

protected:
  QLineEdit * mDescriptionEdit;
  QToolButton * mMoveUpButton;
  QToolButton * mMoveDownButton;
  QToolButton * mDeleteButton;
public:
  void setMoveUpEnabled( bool b );
  void setMoveDownEnabled( bool b );
protected:
  virtual bool eventFilter( QObject *o, QEvent *e );
Q_SIGNALS:
  void moveUpRequest( RuleListEditorItemHeader * header );
  void moveDownRequest( RuleListEditorItemHeader * header );
  void deleteRequest( RuleListEditorItemHeader * header );
protected slots:
  void moveUpButtonClicked();
  void moveDownButtonClicked();
  void deleteButtonClicked();

};



class RuleListEditorScrollAreaPrivate;
class RuleListEditorItem;

class AKONADI_FILTER_UI_EXPORT RuleListEditorScrollArea : public ExpandingScrollArea
{
  Q_OBJECT

public:
  RuleListEditorScrollArea( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory );
  virtual ~RuleListEditorScrollArea();


protected:
  RuleListEditorScrollAreaPrivate * mPrivate;
  QFrame * mBase;
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

  void fillFromRuleList( Action::RuleList * ruleList );
  bool commitStateToRuleList( Action::RuleList * ruleList );
private:
  RuleListEditorItem * addRuleEditor( RuleEditor * editor );
  RuleListEditorItem * findItemByRuleEditor( RuleEditor * editor );
  RuleListEditorItem * findItemByHeader( RuleListEditorItemHeader *header );

  void setCurrentItem( RuleListEditorItem *item );
  void reLayout();
protected slots:
  void addRuleHeaderActivated( RuleListEditorHeader *header );
  void itemHeaderActivated( RuleListEditorHeader *header );
  void itemHeaderDeleteRequest( RuleListEditorItemHeader *header );
  void itemHeaderMoveUpRequest( RuleListEditorItemHeader *header );
  void itemHeaderMoveDownRequest( RuleListEditorItemHeader *header );
};

class AKONADI_FILTER_UI_EXPORT RuleListEditor : public ActionEditor
{
  Q_OBJECT
public:
  RuleListEditor( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory );
  virtual ~RuleListEditor();
protected:
  RuleListEditorScrollArea * mScrollArea;
public:
  void setAutoExpand( bool b );
  bool autoExpand() const;
  virtual void fillFromAction( Action::Base * action );
  virtual void fillFromRuleList( Action::RuleList * ruleList );
  bool commitStateToRuleList( Action::RuleList * ruleList );
  virtual Action::Base * commitState( Component * parent );
};

} // namespace UI

} // namespace Filter

} // namespace Akonadi






#endif //!_AKONADI_FILTER_UI_RULELISTEDITOR_H_
