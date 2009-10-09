/****************************************************************************** * *
 *
 *  File : rulelisteditortbb.cpp
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

#include "rulelisteditortbb.h"
#include "rulelisteditor.h"

#include "../rulelist.h"
#include "../componentfactory.h"

#include "ruleeditor.h"
#include "expandingscrollarea.h"
#include "editorfactory.h"

#include <QtCore/QEvent>
#include <QtCore/QList>
#include <QtCore/QTimer>

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

class RuleListEditorTBBItem
{
public:
  RuleListEditorTBBItemHeader * mHeader;
  ExpandingScrollArea * mScrollArea;
  RuleEditor * mRuleEditor;
  bool mDescriptionModifiedByUser;

  void updateDescription()
  {
    if( mDescriptionModifiedByUser )
      return; // don't touch
    if( mHeader->descriptionEditHasFocus() )
      return; // don't touch again
    mHeader->setDescription( mRuleEditor->ruleDescription() );
  }
};

class RuleListEditorTBBPrivate
{
public:
  QVBoxLayout * mLayout;
  QList< RuleListEditorTBBItem * > mItemList;
  RuleListEditorTBBHeader * mAddRuleHeader;
  QWidget * mFiller;
};





RuleListEditorTBBHeader::RuleListEditorTBBHeader( QWidget *parent )
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

RuleListEditorTBBHeader::~RuleListEditorTBBHeader()
{

}

void RuleListEditorTBBHeader::addWidgetToLayout( QWidget * widget )
{
  mLayout->addWidget( widget );
}


QString RuleListEditorTBBHeader::text()
{
  return mTextLabel->text();
}

void RuleListEditorTBBHeader::setText( const QString &text )
{
  mTextLabel->setText( text );
}

void RuleListEditorTBBHeader::setIcon( const QPixmap &pixmap )
{
  mIconLabel->setPixmap( pixmap );
}

QSize RuleListEditorTBBHeader::sizeHint() const
{
  return mLayout->minimumSize();
}

QSize RuleListEditorTBBHeader::minimumSizeHint() const
{
  return mLayout->minimumSize();
}


void RuleListEditorTBBHeader::paintEvent( QPaintEvent *e )
{
  QWidget::paintEvent( e );

  QPainter p( this );

  qDrawShadeLine( &p, 0, 0, width()+1, 0, palette(), true, 1, 1 );

  QLinearGradient g( 0, 0, 0, height() - 4 );
  g.setColorAt( 0.0, palette().color( QPalette::Light ) );
  g.setColorAt( 1.0, palette().color( QPalette::Button ) );

  p.fillRect( 0, 1, width(), height() - 3 , QBrush( g ) );

}


void RuleListEditorTBBHeader::mouseReleaseEvent( QMouseEvent *e )
{
  QWidget::mouseReleaseEvent( e );

  kDebug() << "Mouse released";

  emit activated( this );
}


RuleListEditorTBBItemHeader::RuleListEditorTBBItemHeader( QWidget *parent )
  : RuleListEditorTBBHeader( parent )
{
  mDescriptionEdit = new QLineEdit( this );
  mDescriptionEdit->setFrame( false );
  mDescriptionEdit->setBackgroundRole( QPalette::Button ); // this doesn't work (style ignores this)

  connect( mDescriptionEdit, SIGNAL( textEdited( const QString & ) ), this, SLOT( slotDescriptionEditTextEdited( const QString & ) ) );

  QPalette pal = palette();
  pal.setColor( QPalette::Base, pal.color( QPalette::Button ) );
  mDescriptionEdit->setPalette( pal );
  mDescriptionEdit->installEventFilter( this );
  addWidgetToLayout( mDescriptionEdit );

  mMoveDownButton = new QToolButton( this );
  mMoveDownButton->setIcon( SmallIcon( QLatin1String( "arrow-down" ) ) );
  mMoveDownButton->setAutoRaise( true );
  mMoveDownButton->setToolTip( i18n( "Move this rule down" ) );
  connect( mMoveDownButton, SIGNAL( clicked() ), this, SLOT( moveDownButtonClicked() ) );
  addWidgetToLayout( mMoveDownButton );

  mMoveUpButton = new QToolButton( this );
  mMoveUpButton->setIcon( SmallIcon( QLatin1String( "arrow-up" ) ) );
  mMoveUpButton->setAutoRaise( true );
  mMoveUpButton->setToolTip( i18n( "Move this rule up" ) );
  connect( mMoveUpButton, SIGNAL( clicked() ), this, SLOT( moveUpButtonClicked() ) );
  addWidgetToLayout( mMoveUpButton );

  mDeleteButton = new QToolButton( this );
  mDeleteButton->setIcon( SmallIcon( QLatin1String( "edit-delete" ) ) );
  mDeleteButton->setAutoRaise( true );
  mDeleteButton->setToolTip( i18n( "Delete this rule" ) );
  connect( mDeleteButton, SIGNAL( clicked() ), this, SLOT( deleteButtonClicked() ) );
  addWidgetToLayout( mDeleteButton );
  
}

RuleListEditorTBBItemHeader::~RuleListEditorTBBItemHeader()
{

}

bool RuleListEditorTBBItemHeader::descriptionEditHasFocus()
{
  return mDescriptionEdit->hasFocus();
}

void RuleListEditorTBBItemHeader::setDescription( const QString &description )
{
  mDescriptionEdit->setText( description );
}

QString RuleListEditorTBBItemHeader::description()
{
  return mDescriptionEdit->text();
}

void RuleListEditorTBBItemHeader::setMoveUpEnabled( bool b )
{
  mMoveUpButton->setEnabled( b );
}

void RuleListEditorTBBItemHeader::setMoveDownEnabled( bool b )
{
  mMoveDownButton->setEnabled( b );
}

void RuleListEditorTBBItemHeader::moveUpButtonClicked()
{
  emit moveUpRequest( this );
}

void RuleListEditorTBBItemHeader::moveDownButtonClicked()
{
  emit moveDownRequest( this );
}

void RuleListEditorTBBItemHeader::deleteButtonClicked()
{
  emit deleteRequest( this );
}

void RuleListEditorTBBItemHeader::slotDescriptionEditTextEdited( const QString & )
{
  emit descriptionEdited( this );
}

bool RuleListEditorTBBItemHeader::eventFilter( QObject *o, QEvent *e )
{
  if( o == static_cast< QObject * >( mDescriptionEdit ) )
  {
    if( e->type() == QEvent::MouseButtonRelease )
      emit activated( this );
  }
  
  return QObject::eventFilter( o, e );
}






RuleListEditorTBB::RuleListEditorTBB( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory )
  : ExpandingScrollArea( parent ), mComponentFactory( componentfactory ), mEditorFactory( editorComponentFactory )
{
  setFrameStyle( QFrame::NoFrame );

  mCurrentItem = 0;

  mBase = new QFrame( this );
  mBase->setFrameStyle( QFrame::NoFrame );
  setWidget( mBase );
  setWidgetResizable( true );

  mPrivate = new RuleListEditorTBBPrivate;
  mPrivate->mLayout = 0;
  mPrivate->mFiller = new QWidget( mBase );
  mPrivate->mFiller->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
  mPrivate->mAddRuleHeader = new RuleListEditorTBBHeader( mBase );
  mPrivate->mAddRuleHeader->setText( i18n( "Add Rule" ) );
  mPrivate->mAddRuleHeader->setIcon( SmallIcon( QLatin1String( "list-add" ) ) );
  connect( mPrivate->mAddRuleHeader, SIGNAL( activated( RuleListEditorTBBHeader * ) ), this, SLOT( addRuleHeaderActivated( RuleListEditorTBBHeader * ) ) );

  setBackgroundRole( QPalette::Button );

  reLayout();

  mHeartbeatTimer = new QTimer( this );
  connect( mHeartbeatTimer, SIGNAL( timeout() ), this, SLOT( slotHeartbeat() ) );

  mHeartbeatTimer->start( 2000 );
}

RuleListEditorTBB::~RuleListEditorTBB()
{
  qDeleteAll( mPrivate->mItemList );
  delete mPrivate;
}

void RuleListEditorTBB::fillFromRuleList( Action::RuleList * ruleList )
{
  const QList< Rule * > * rules = ruleList->ruleList();
  Q_ASSERT( rules );

  mCurrentItem = 0;

  foreach( Rule * rule, *rules )
  {
    RuleEditor * ruleEditor = mEditorFactory->createRuleEditor( mBase, mComponentFactory );
    Q_ASSERT( ruleEditor );

    ruleEditor->fillFromRule( rule );
    //ruleEditor->setBackgroundRole( QPalette::ToolTipBase );
    addRuleEditor( ruleEditor, rule->description() );
  }

  reLayout();
}

bool RuleListEditorTBB::commitStateToRuleList( Action::RuleList * ruleList )
{
  Q_ASSERT( ruleList );

  ruleList->clear();

  RuleListEditorTBBItem * item;

  foreach( item, mPrivate->mItemList )
  {
    if( item->mRuleEditor->isEmpty() )
      continue; // senseless empty rule

    Rule * rule = item->mRuleEditor->commitState( ruleList );
    if( !rule )
    {
      kDebug() << "Failed to create rule";
      setCurrentItem( item );
      return false; // error already shown
    }

    if( item->mDescriptionModifiedByUser )
      rule->setDescription( item->mHeader->description() );

    ruleList->addRule( rule );
  }

  return true;
}

RuleListEditorTBBItem * RuleListEditorTBB::findItemByRuleEditor( RuleEditor * editor )
{
  foreach( RuleListEditorTBBItem * it, mPrivate->mItemList )
  {
    if( it->mRuleEditor == editor )
      return it;
  }
  return 0;
}

RuleListEditorTBBItem * RuleListEditorTBB::findItemByHeader( RuleListEditorTBBItemHeader *header )
{
  foreach( RuleListEditorTBBItem * it, mPrivate->mItemList )
  {
    if( it->mHeader == header )
      return it;
  }
  return 0;
}


RuleListEditorTBBItem * RuleListEditorTBB::addRuleEditor( RuleEditor * editor, const QString &ruleDescription )
{
  Q_ASSERT( editor );

  RuleListEditorTBBItem * item = new RuleListEditorTBBItem;
  item->mHeader = new RuleListEditorTBBItemHeader( mBase );
  connect( item->mHeader, SIGNAL( activated( RuleListEditorTBBHeader * ) ), this, SLOT( itemHeaderActivated( RuleListEditorTBBHeader * ) ) );
  item->mHeader->setIcon( SmallIcon( QLatin1String( "application-x-executable" ) ) );

  connect( item->mHeader, SIGNAL( moveUpRequest( RuleListEditorTBBItemHeader * ) ), this, SLOT( itemHeaderMoveUpRequest( RuleListEditorTBBItemHeader * ) ) );
  connect( item->mHeader, SIGNAL( moveDownRequest( RuleListEditorTBBItemHeader * ) ), this, SLOT( itemHeaderMoveDownRequest( RuleListEditorTBBItemHeader * ) ) );
  connect( item->mHeader, SIGNAL( deleteRequest( RuleListEditorTBBItemHeader * ) ), this, SLOT( itemHeaderDeleteRequest( RuleListEditorTBBItemHeader * ) ) );
  connect( item->mHeader, SIGNAL( descriptionEdited( RuleListEditorTBBItemHeader * ) ), this, SLOT( itemHeaderDescriptionEdited( RuleListEditorTBBItemHeader * ) ) );

  item->mScrollArea = new ExpandingScrollArea( mBase );
  item->mScrollArea->setWidget( editor );
  item->mScrollArea->setWidgetResizable( true );
  item->mScrollArea->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );
  item->mScrollArea->setFrameStyle( QFrame::NoFrame );
  item->mScrollArea->hide();

  item->mRuleEditor = editor;

  connect( item->mRuleEditor, SIGNAL( ruleChanged() ), this, SLOT( slotRuleChanged() ) );

  mPrivate->mItemList.append( item );

  if( ruleDescription.isEmpty() )
  {
    item->mHeader->setDescription( editor->ruleDescription() );
    item->mDescriptionModifiedByUser = false;
  } else {
    item->mHeader->setDescription( ruleDescription );
    item->mDescriptionModifiedByUser = true;
  }

  item->mHeader->show();

  reLayout();

  return item;
}

void RuleListEditorTBB::slotRuleChanged()
{
  RuleEditor * editor = dynamic_cast< RuleEditor * >( sender() );
  if( !editor )
    return;

  foreach( RuleListEditorTBBItem * item, mPrivate->mItemList )
  {
    if( item->mRuleEditor == editor )
    {
      item->updateDescription();
      return;
    }
  }
}

void RuleListEditorTBB::reLayout()
{
  if( mPrivate->mLayout )
    delete mPrivate->mLayout;

  mPrivate->mLayout = new QVBoxLayout( mBase );
  mPrivate->mLayout->setMargin( 0 );

  RuleListEditorTBBItem * item;

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


void RuleListEditorTBB::setCurrentItem( RuleListEditorTBBItem *item )
{
  if( mCurrentItem )
    mCurrentItem->updateDescription();

  mCurrentItem = 0;

  foreach( RuleListEditorTBBItem * it, mPrivate->mItemList )
  {
    if( it == item )
    {
      it->mScrollArea->show();
      mCurrentItem = item;
    } else {
      it->mScrollArea->hide();
    }
  }

  // we have at least one item: no filler is needed
  mPrivate->mLayout->removeWidget( mPrivate->mFiller );
}

void RuleListEditorTBB::addRuleHeaderActivated( RuleListEditorTBBHeader *header )
{
  Q_UNUSED( header );
  addRuleEditor( new RuleEditor( mBase, mComponentFactory, mEditorFactory ), QString() );
}

void RuleListEditorTBB::itemHeaderDescriptionEdited( RuleListEditorTBBItemHeader *header )
{
  RuleListEditorTBBItem * item = findItemByHeader( static_cast< RuleListEditorTBBItemHeader * >( header ) );
  Q_ASSERT( item );

  item->mDescriptionModifiedByUser = !header->description().isEmpty();
}

void RuleListEditorTBB::itemHeaderActivated( RuleListEditorTBBHeader *header )
{
  RuleListEditorTBBItem * item = findItemByHeader( static_cast< RuleListEditorTBBItemHeader * >( header ) );
  Q_ASSERT( item );
  setCurrentItem( item );
}

void RuleListEditorTBB::itemHeaderDeleteRequest( RuleListEditorTBBItemHeader *header )
{
  RuleListEditorTBBItem * item = findItemByHeader( header );
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

  mCurrentItem = 0;

  delete item->mHeader;
  delete item->mRuleEditor;
  delete item->mScrollArea;

  mPrivate->mItemList.removeAll( item );

  reLayout();
}

void RuleListEditorTBB::itemHeaderMoveUpRequest( RuleListEditorTBBItemHeader *header )
{
  RuleListEditorTBBItem * item = findItemByHeader( header );
  Q_ASSERT( item );

  int idx = mPrivate->mItemList.indexOf( item );
  Q_ASSERT( idx >= 0 );

  if( idx == 0 )
    return;

  mPrivate->mItemList.removeAt( idx );
  mPrivate->mItemList.insert( idx - 1, item );

  reLayout();
}

void RuleListEditorTBB::itemHeaderMoveDownRequest( RuleListEditorTBBItemHeader *header )
{
  RuleListEditorTBBItem * item = findItemByHeader( header );
  Q_ASSERT( item );

  int idx = mPrivate->mItemList.indexOf( item );
  Q_ASSERT( idx >= 0 );

  if( idx == ( mPrivate->mItemList.count() - 1 ) )
    return;

  mPrivate->mItemList.removeAt( idx );
  mPrivate->mItemList.insert( idx + 1, item );

  reLayout();
}

void RuleListEditorTBB::slotHeartbeat()
{
  if( !mCurrentItem )
    return;

  mCurrentItem->updateDescription();
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi
