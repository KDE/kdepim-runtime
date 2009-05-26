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

#include "rulelisteditor.h"

#include <akonadi/filter/rulelist.h>
#include <akonadi/filter/factory.h>

#include "ruleeditor.h"

#include <QResizeEvent>
#include <QLayout>
#include <QList>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QStyle>
#include <QEvent>
#include <QToolButton>

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
  QScrollArea * mScrollArea;
  RuleEditor * mRuleEditor;
};

class RuleListEditorPrivate
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



RuleListEditorItemScrollArea::RuleListEditorItemScrollArea( QWidget * parent )
  : QScrollArea( parent )
{
}

bool RuleListEditorItemScrollArea::eventFilter( QObject *o, QEvent *e )
{
  // QScrollArea monitors the child widget events and updates its scrollbars
  // when its resized.
  // We actually DON'T want the vertical scrollbar to appear in this area
  // so we monitor the event too and actually force the parent
  // to redo a layout (which will grow this QScrollArea size because of
  // the size policies and propagated size hints).

  if( o == widget() && e->type() == QEvent::Resize )
  {
    // This machinery is needed in order to trigger the parent geometry update
    updateGeometry();

    if( parentWidget()->layout() )
    {
      parentWidget()->layout()->invalidate();
      parentWidget()->layout()->update();
    }

    //parentWidget()->updateGeometry();
  }
  return QScrollArea::eventFilter( o, e );
}


QSize RuleListEditorItemScrollArea::sizeHint() const
{
  if( !widget() )
    return QScrollArea::sizeHint();
  return widget()->sizeHint();
}

QSize RuleListEditorItemScrollArea::minimumSizeHint() const
{
  if( !widget() )
    return QScrollArea::minimumSizeHint();
  return widget()->minimumSizeHint();
}


RuleListEditor::RuleListEditor( QWidget * parent, Factory * factory )
  : QScrollArea( parent ), mFactory( factory )
{
  mBase = new QFrame( this );
  mBase->setFrameStyle( QFrame::NoFrame );
  setWidget( mBase );
  setWidgetResizable( true );

  mPrivate = new RuleListEditorPrivate;
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

RuleListEditor::~RuleListEditor()
{
  qDeleteAll( mPrivate->mItemList );
  delete mPrivate;
}

void RuleListEditor::fillFromRuleList( Action::RuleList * ruleList )
{
  const QList< Rule * > * rules = ruleList->ruleList();
  Q_ASSERT( rules );

  foreach( Rule * rule, *rules )
  {
    RuleEditor * ruleEditor = new RuleEditor( mBase, mFactory );
    ruleEditor->fillFromRule( rule );
    //ruleEditor->setBackgroundRole( QPalette::ToolTipBase );
    addRuleEditor( ruleEditor );
  }

  reLayout();
}

RuleListEditorItem * RuleListEditor::findItemByRuleEditor( RuleEditor * editor )
{
  foreach( RuleListEditorItem * it, mPrivate->mItemList )
  {
    if( it->mRuleEditor == editor )
      return it;
  }
  return 0;
}

RuleListEditorItem * RuleListEditor::findItemByHeader( RuleListEditorItemHeader *header )
{
  foreach( RuleListEditorItem * it, mPrivate->mItemList )
  {
    if( it->mHeader == header )
      return it;
  }
  return 0;
}


RuleListEditorItem * RuleListEditor::addRuleEditor( RuleEditor * editor )
{
  Q_ASSERT( editor );

  RuleListEditorItem * item = new RuleListEditorItem;
  item->mHeader = new RuleListEditorItemHeader( mBase );
  connect( item->mHeader, SIGNAL( activated( RuleListEditorHeader * ) ), this, SLOT( itemHeaderActivated( RuleListEditorHeader * ) ) );
  item->mHeader->setIcon( SmallIcon( "application-x-executable" ) );

  connect( item->mHeader, SIGNAL( moveUpRequest( RuleListEditorItemHeader * ) ), this, SLOT( itemHeaderMoveUpRequest( RuleListEditorItemHeader * ) ) );
  connect( item->mHeader, SIGNAL( moveDownRequest( RuleListEditorItemHeader * ) ), this, SLOT( itemHeaderMoveDownRequest( RuleListEditorItemHeader * ) ) );
  connect( item->mHeader, SIGNAL( deleteRequest( RuleListEditorItemHeader * ) ), this, SLOT( itemHeaderDeleteRequest( RuleListEditorItemHeader * ) ) );

  item->mScrollArea = new RuleListEditorItemScrollArea( mBase );
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

void RuleListEditor::reLayout()
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


void RuleListEditor::setCurrentItem( RuleListEditorItem *item )
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

void RuleListEditor::addRuleHeaderActivated( RuleListEditorHeader *header )
{
  addRuleEditor( new RuleEditor( mBase, mFactory ) );
}

void RuleListEditor::itemHeaderActivated( RuleListEditorHeader *header )
{
  RuleListEditorItem * item = findItemByHeader( static_cast< RuleListEditorItemHeader * >( header ) );
  Q_ASSERT( item );
  setCurrentItem( item );
}

void RuleListEditor::itemHeaderDeleteRequest( RuleListEditorItemHeader *header )
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

void RuleListEditor::itemHeaderMoveUpRequest( RuleListEditorItemHeader *header )
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

void RuleListEditor::itemHeaderMoveDownRequest( RuleListEditorItemHeader *header )
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


} // namespace UI

} // namespace Filter

} // namespace Akonadi
