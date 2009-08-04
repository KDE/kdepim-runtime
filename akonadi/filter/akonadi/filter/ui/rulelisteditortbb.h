/****************************************************************************** * *
 *
 *  File : rulelisteditortbb.h
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

#ifndef _AKONADI_FILTER_UI_RULELISTEDITORTBB_H_
#define _AKONADI_FILTER_UI_RULELISTEDITORTBB_H_

#include <akonadi/filter/ui/config-akonadi-filter-ui.h>

#include <akonadi/filter/ui/expandingscrollarea.h>

#include <QtGui/QWidget>
#include <QtGui/QScrollArea>

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

class EditorFactory;
class RuleEditor;

class RuleListEditorTBBHeader : public QWidget
{
  Q_OBJECT

public:
  RuleListEditorTBBHeader( QWidget * parent );
  virtual ~RuleListEditorTBBHeader();

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
  void activated( RuleListEditorTBBHeader * header );

};

class RuleListEditorTBBItemHeader : public RuleListEditorTBBHeader
{
  Q_OBJECT

public:
  RuleListEditorTBBItemHeader( QWidget *parent );
  virtual ~RuleListEditorTBBItemHeader();

protected:
  QLineEdit * mDescriptionEdit;
  QToolButton * mMoveUpButton;
  QToolButton * mMoveDownButton;
  QToolButton * mDeleteButton;
public:
  void setDescription( const QString &description );
  QString description();
  void setMoveUpEnabled( bool b );
  void setMoveDownEnabled( bool b );
protected:
  virtual bool eventFilter( QObject *o, QEvent *e );
Q_SIGNALS:
  void moveUpRequest( RuleListEditorTBBItemHeader * header );
  void moveDownRequest( RuleListEditorTBBItemHeader * header );
  void deleteRequest( RuleListEditorTBBItemHeader * header );
protected slots:
  void moveUpButtonClicked();
  void moveDownButtonClicked();
  void deleteButtonClicked();

};



class RuleListEditorTBBPrivate;
class RuleListEditorTBBItem;

class AKONADI_FILTER_UI_EXPORT RuleListEditorTBB : public ExpandingScrollArea
{
  Q_OBJECT

public:
  RuleListEditorTBB( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory );
  virtual ~RuleListEditorTBB();


protected:
  RuleListEditorTBBPrivate * mPrivate;
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
  RuleListEditorTBBItem * addRuleEditor( RuleEditor * editor );
  RuleListEditorTBBItem * findItemByRuleEditor( RuleEditor * editor );
  RuleListEditorTBBItem * findItemByHeader( RuleListEditorTBBItemHeader *header );

  void setCurrentItem( RuleListEditorTBBItem *item );
  void reLayout();
protected slots:
  void addRuleHeaderActivated( RuleListEditorTBBHeader *header );
  void itemHeaderActivated( RuleListEditorTBBHeader *header );
  void itemHeaderDeleteRequest( RuleListEditorTBBItemHeader *header );
  void itemHeaderMoveUpRequest( RuleListEditorTBBItemHeader *header );
  void itemHeaderMoveDownRequest( RuleListEditorTBBItemHeader *header );
  void slotRuleChanged();
};

} // namespace UI

} // namespace Filter

} // namespace Akonadi






#endif //!_AKONADI_FILTER_UI_RULELISTEDITORTBB_H_
