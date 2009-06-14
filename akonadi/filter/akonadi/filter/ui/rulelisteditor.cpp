/****************************************************************************** * *
 *
 *  File : rulelisteditor.cpp
 *  Created on Tue 19 May 2009 02:20:16 by Szymon Tomasz Stefanek
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

#include <akonadi/filter/ui/rulelisteditor.h>

#include <akonadi/filter/rulelist.h>
#include <akonadi/filter/componentfactory.h>

#include <akonadi/filter/ui/ruleeditor.h>
#include <akonadi/filter/ui/expandingscrollarea.h>
#include <akonadi/filter/ui/editorfactory.h>

#include <QtCore/QEvent>
#include <QtCore/QList>
#include <QtGui/QFrame>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtGui/QStyle>
#include <QtGui/QToolButton>

#include <KApplication>
#include <KDebug>
#include <KLocale>
#include <KIconLoader>
#include <KMessageBox>

// We're not using QToolBox because it doesn't allow us to customize the item headers...

namespace Akonadi
{
namespace Filter
{
namespace UI
{

class RuleListEditorItem
{
public:
  RuleListEditorItemHeader * mHeader;
  ExpandingScrollArea * mScrollArea;
  RuleEditor * mRuleEditor;
};

class RuleListEditorScrollAreaPrivate
{
public:
  QVBoxLayout * mLayout;
  QList< RuleListEditorItem * > mItemList;
  RuleListEditorHeader * mAddRuleHeader;
  QWidget * mFiller;
};





RuleListEditorHeader::RuleListEditorHeader( QWidget *parent )
  : QWidget( parent )
{
  setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
  setFocusPolicy( Qt::NoFocus );

  mLayout = new QHBoxLayout( this );
  mLayout->setMargin( 2 );

  mIconLabel = new QLabel( this );
  mIconLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
  mLayout->addWidget( mIconLabel );

  mTextLabel = new QLabel( this );
  mLayout->addWidget( mTextLabel );
}

RuleListEditorHeader::~RuleListEditorHeader()
{

}

void RuleListEditorHeader::addWidgetToLayout( QWidget * widget )
{
  mLayout->addWidget( widget );
}


QString RuleListEditorHeader::text()
{
  return mTextLabel->text();
}

void RuleListEditorHeader::setText( const QString &text )
{
  mTextLabel->setText( text );
}

void RuleListEditorHeader::setIcon( const QPixmap &pixmap )
{
  mIconLabel->setPixmap( pixmap );
}

QSize RuleListEditorHeader::sizeHint() const
{
  return mLayout->minimumSize();
}

QSize RuleListEditorHeader::minimumSizeHint() const
{
  return mLayout->minimumSize();
}


void RuleListEditorHeader::paintEvent( QPaintEvent *e )
{
  QWidget::paintEvent( e );

  QPainter p( this );

  qDrawShadeLine( &p, 0, 0, width()+1, 0, palette(), true, 1, 1 );

  QLinearGradient g( 0, 0, 0, height() - 4 );
  g.setColorAt( 0.0, palette().color( QPalette::Light ) );
  g.setColorAt( 1.0, palette().color( QPalette::Button ) );

  p.fillRect( 0, 1, width(), height() - 3 , QBrush( g ) );

}


void RuleListEditorHeader::mouseReleaseEvent( QMouseEvent *e )
{
  QWidget::mouseReleaseEvent( e );

  kDebug() << "Mouse released";

  emit activated( this );
}


RuleListEditorItemHeader::RuleListEditorItemHeader( QWidget *parent )
  : RuleListEditorHeader( parent )
{
  mDescriptionEdit = new QLineEdit( this );
  mDescriptionEdit->setFrame( false );
  mDescriptionEdit->setBackgroundRole( QPalette::Button ); // this doesn't work (style ignores this)
  QPalette pal = palette();
  pal.setColor( QPalette::Base, pal.color( QPalette::Button ) );
  mDescriptionEdit->setPalette( pal );
  mDescriptionEdit->installEventFilter( this );
  addWidgetToLayout( mDescriptionEdit );

  mMoveDownButton = new QToolButton( this );
  mMoveDownButton->setIcon( SmallIcon( "arrow-down" ) );
  mMoveDownButton->setAutoRaise( true );
  mMoveDownButton->setToolTip( i18n( "Move this rule down" ) );
  connect( mMoveDownButton, SIGNAL( clicked() ), this, SLOT( moveDownButtonClicked() ) );
  addWidgetToLayout( mMoveDownButton );

  mMoveUpButton = new QToolButton( this );
  mMoveUpButton->setIcon( SmallIcon( "arrow-up" ) );
  mMoveUpButton->setAutoRaise( true );
  mMoveUpButton->setToolTip( i18n( "Move this rule up" ) );
  connect( mMoveUpButton, SIGNAL( clicked() ), this, SLOT( moveUpButtonClicked() ) );
  addWidgetToLayout( mMoveUpButton );

  mDeleteButton = new QToolButton( this );
  mDeleteButton->setIcon( SmallIcon( "edit-delete" ) );
  mDeleteButton->setAutoRaise( true );
  mDeleteButton->setToolTip( i18n( "Delete this rule" ) );
  connect( mDeleteButton, SIGNAL( clicked() ), this, SLOT( deleteButtonClicked() ) );
  addWidgetToLayout( mDeleteButton );
  
}

RuleListEditorItemHeader::~RuleListEditorItemHeader()
{

}

void RuleListEditorItemHeader::setMoveUpEnabled( bool b )
{
  mMoveUpButton->setEnabled( b );
}

void RuleListEditorItemHeader::setMoveDownEnabled( bool b )
{
  mMoveDownButton->setEnabled( b );
}

void RuleListEditorItemHeader::moveUpButtonClicked()
{
  emit moveUpRequest( this );
}

void RuleListEditorItemHeader::moveDownButtonClicked()
{
  emit moveDownRequest( this );
}

void RuleListEditorItemHeader::deleteButtonClicked()
{
  emit deleteRequest( this );
}

bool RuleListEditorItemHeader::eventFilter( QObject *o, QEvent *e )
{
  if( o == static_cast< QObject * >( mDescriptionEdit ) )
  {
    if( e->type() == QEvent::MouseButtonRelease )
      emit activated( this );
  }
  
  return QObject::eventFilter( o, e );
}






RuleListEditorScrollArea::RuleListEditorScrollArea( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory )
  : ExpandingScrollArea( parent ), mComponentFactory( componentfactory ), mEditorFactory( editorComponentFactory )
{
  setFrameStyle( QFrame::NoFrame );

  mBase = new QFrame( this );
  mBase->setFrameStyle( QFrame::NoFrame );
  setWidget( mBase );
  setWidgetResizable( true );

  mPrivate = new RuleListEditorScrollAreaPrivate;
  mPrivate->mLayout = 0;
  mPrivate->mFiller = new QWidget( mBase );
  mPrivate->mFiller->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
  mPrivate->mAddRuleHeader = new RuleListEditorHeader( mBase );
  mPrivate->mAddRuleHeader->setText( i18n( "Add Rule" ) );
  mPrivate->mAddRuleHeader->setIcon( SmallIcon( "list-add" ) );
  connect( mPrivate->mAddRuleHeader, SIGNAL( activated( RuleListEditorHeader * ) ), this, SLOT( addRuleHeaderActivated( RuleListEditorHeader * ) ) );

  setBackgroundRole( QPalette::Button );

  reLayout();
}

RuleListEditorScrollArea::~RuleListEditorScrollArea()
{
  qDeleteAll( mPrivate->mItemList );
  delete mPrivate;
}

void RuleListEditorScrollArea::fillFromRuleList( Action::RuleList * ruleList )
{
  const QList< Rule * > * rules = ruleList->ruleList();
  Q_ASSERT( rules );

  foreach( Rule * rule, *rules )
  {
    RuleEditor * ruleEditor = mEditorFactory->createRuleEditor( mBase, mComponentFactory );
    Q_ASSERT( ruleEditor );

    ruleEditor->fillFromRule( rule );
    //ruleEditor->setBackgroundRole( QPalette::ToolTipBase );
    addRuleEditor( ruleEditor );
  }

  reLayout();
}

bool RuleListEditorScrollArea::commitStateToRuleList( Action::RuleList * ruleList )
{
  Q_ASSERT( ruleList );

  ruleList->clear();

  RuleListEditorItem * item;

  foreach( item, mPrivate->mItemList )
  {
    Rule * rule = item->mRuleEditor->commitState( ruleList );
    if( !rule )
    {
      kDebug() << "Failed to create rule";
      return false; // error already shown
    }
    ruleList->addRule( rule );
  }

  return true;
}

RuleListEditorItem * RuleListEditorScrollArea::findItemByRuleEditor( RuleEditor * editor )
{
  foreach( RuleListEditorItem * it, mPrivate->mItemList )
  {
    if( it->mRuleEditor == editor )
      return it;
  }
  return 0;
}

RuleListEditorItem * RuleListEditorScrollArea::findItemByHeader( RuleListEditorItemHeader *header )
{
  foreach( RuleListEditorItem * it, mPrivate->mItemList )
  {
    if( it->mHeader == header )
      return it;
  }
  return 0;
}


RuleListEditorItem * RuleListEditorScrollArea::addRuleEditor( RuleEditor * editor )
{
  Q_ASSERT( editor );

  RuleListEditorItem * item = new RuleListEditorItem;
  item->mHeader = new RuleListEditorItemHeader( mBase );
  connect( item->mHeader, SIGNAL( activated( RuleListEditorHeader * ) ), this, SLOT( itemHeaderActivated( RuleListEditorHeader * ) ) );
  item->mHeader->setIcon( SmallIcon( "application-x-executable" ) );

  connect( item->mHeader, SIGNAL( moveUpRequest( RuleListEditorItemHeader * ) ), this, SLOT( itemHeaderMoveUpRequest( RuleListEditorItemHeader * ) ) );
  connect( item->mHeader, SIGNAL( moveDownRequest( RuleListEditorItemHeader * ) ), this, SLOT( itemHeaderMoveDownRequest( RuleListEditorItemHeader * ) ) );
  connect( item->mHeader, SIGNAL( deleteRequest( RuleListEditorItemHeader * ) ), this, SLOT( itemHeaderDeleteRequest( RuleListEditorItemHeader * ) ) );

  item->mScrollArea = new ExpandingScrollArea( mBase );
  item->mScrollArea->setWidget( editor );
  item->mScrollArea->setWidgetResizable( true );
  item->mScrollArea->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );
  item->mScrollArea->setFrameStyle( QFrame::NoFrame );
  item->mScrollArea->hide();

  item->mRuleEditor = editor;

  mPrivate->mItemList.append( item );

  item->mHeader->show();

  reLayout();

  return item;
}

void RuleListEditorScrollArea::reLayout()
{
  if( mPrivate->mLayout )
    delete mPrivate->mLayout;

  mPrivate->mLayout = new QVBoxLayout( mBase );
  mPrivate->mLayout->setMargin( 0 );

  RuleListEditorItem * item;

  int idx = 1;

  foreach( item, mPrivate->mItemList )
  {
    mPrivate->mLayout->addWidget( item->mHeader );
    item->mHeader->setText( i18n( "Rule %1:", idx ) );
    mPrivate->mLayout->addWidget( item->mScrollArea );
    item->mHeader->setMoveUpEnabled( true );
    item->mHeader->setMoveDownEnabled( true );
    idx++;
  }

  mPrivate->mLayout->addWidget( mPrivate->mAddRuleHeader );

  if( mPrivate->mItemList.isEmpty() )
    mPrivate->mLayout->addWidget( mPrivate->mFiller );
  else {
    item = mPrivate->mItemList.first();
    Q_ASSERT( item );
    item->mHeader->setMoveUpEnabled( false );

    item = mPrivate->mItemList.last();
    Q_ASSERT( item );
    item->mHeader->setMoveDownEnabled( false );
    setCurrentItem( item );
  }
}


void RuleListEditorScrollArea::setCurrentItem( RuleListEditorItem *item )
{
  foreach( RuleListEditorItem * it, mPrivate->mItemList )
  {
    if( it == item )
    {
      it->mScrollArea->show();
    } else {
      it->mScrollArea->hide();
    }
  }

  // we have at least one item: no filler is needed
  mPrivate->mLayout->removeWidget( mPrivate->mFiller );
}

void RuleListEditorScrollArea::addRuleHeaderActivated( RuleListEditorHeader *header )
{
  addRuleEditor( new RuleEditor( mBase, mComponentFactory, mEditorFactory ) );
}

void RuleListEditorScrollArea::itemHeaderActivated( RuleListEditorHeader *header )
{
  RuleListEditorItem * item = findItemByHeader( static_cast< RuleListEditorItemHeader * >( header ) );
  Q_ASSERT( item );
  setCurrentItem( item );
}

void RuleListEditorScrollArea::itemHeaderDeleteRequest( RuleListEditorItemHeader *header )
{
  RuleListEditorItem * item = findItemByHeader( header );
  Q_ASSERT( item );

  //setCurrentItem( item );

  if(
     KMessageBox::questionYesNo(
         this,
         i18n( "Do you really want to delete the specified rule?" ),
         i18n( "Confirm Rule Deletion")
       ) != KMessageBox::Yes
    )
     return;

  delete item->mHeader;
  delete item->mRuleEditor;
  delete item->mScrollArea;

  mPrivate->mItemList.removeAll( item );

  reLayout();
}

void RuleListEditorScrollArea::itemHeaderMoveUpRequest( RuleListEditorItemHeader *header )
{
  RuleListEditorItem * item = findItemByHeader( header );
  Q_ASSERT( item );

  int idx = mPrivate->mItemList.indexOf( item );
  Q_ASSERT( idx >= 0 );

  if( idx == 0 )
    return;

  mPrivate->mItemList.removeAt( idx );
  mPrivate->mItemList.insert( idx - 1, item );

  reLayout();
}

void RuleListEditorScrollArea::itemHeaderMoveDownRequest( RuleListEditorItemHeader *header )
{
  RuleListEditorItem * item = findItemByHeader( header );
  Q_ASSERT( item );

  int idx = mPrivate->mItemList.indexOf( item );
  Q_ASSERT( idx >= 0 );

  if( idx == ( mPrivate->mItemList.count() - 1 ) )
    return;

  mPrivate->mItemList.removeAt( idx );
  mPrivate->mItemList.insert( idx + 1, item );

  reLayout();
}



RuleListEditor::RuleListEditor( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory )
  : ActionEditor( parent, componentfactory, editorComponentFactory )
{
  mScrollArea = new RuleListEditorScrollArea( this, componentfactory, editorComponentFactory );

  QGridLayout * g = new QGridLayout( this );
  g->addWidget( mScrollArea, 0, 0 );
  g->setMargin( 0 );
}

RuleListEditor::~RuleListEditor()
{
}

void RuleListEditor::fillFromAction( Action::Base * action )
{
  Q_ASSERT( action );
  Q_ASSERT( action->actionType() == Action::ActionTypeRuleList );
  fillFromRuleList( static_cast< Action::RuleList * >( action ) );
}

void RuleListEditor::fillFromRuleList( Action::RuleList * ruleList )
{
  Q_ASSERT( ruleList );
  mScrollArea->fillFromRuleList( ruleList );  
}

Action::Base * RuleListEditor::commitState( Component * parent )
{
  Action::RuleList * ruleList = componentFactory()->createRuleList( parent );
  Q_ASSERT( ruleList );

  if( !commitStateToRuleList( ruleList ) )
  {
    delete ruleList;
    return 0;
  }

  return ruleList;
}

bool RuleListEditor::commitStateToRuleList( Action::RuleList * ruleList )
{
  Q_ASSERT( ruleList );

  return mScrollArea->commitStateToRuleList( ruleList );
}

bool RuleListEditor::autoExpand() const
{
  return mScrollArea->autoExpand();
}

void RuleListEditor::setAutoExpand( bool b )
{
  mScrollArea->setAutoExpand( b );
}


} // namespace UI

} // namespace Filter

} // namespace Akonadi
