/****************************************************************************** * *
 *
 *  File : rulelisteditorlbb.cpp
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

#include "rulelisteditorlbb.h"
#include "rulelisteditor.h"

#include "ruleeditor.h"
#include "expandingscrollarea.h"
#include "editorfactory.h"

#include "../rulelist.h"
#include "../rule.h"

#include <QtCore/QTimer>

#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>

#include <KDebug>
#include <KLocale>
#include <KIconLoader>
#include <KMessageBox>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

class RuleListEditorLBBListWidgetItem : public QListWidgetItem
{
protected:
  RuleEditor * mEditor;
  QString mUserDescription;
  int mIndex;
public:
  RuleListEditorLBBListWidgetItem( QListWidget * parent, RuleEditor * editor, const QString &userDescription, int index )
    : QListWidgetItem( parent ), mEditor( editor ), mUserDescription( userDescription ), mIndex( index )
  {
    setIcon( SmallIcon( QLatin1String( "application-x-executable" ) ) );
    updateText();
  }

  RuleEditor * editor()
  {
    return mEditor;
  }

  void setIndex( int idx )
  {
    mIndex = idx;
    updateText();
  }

  int index()
  {
    return mIndex;
  }

  const QString & userDescription()
  {
    return mUserDescription;
  }

  void setUserDescription( const QString &text )
  {
    mUserDescription = text;
    updateText();
  }

  QString description()
  {
    return mUserDescription.isEmpty() ? mEditor->ruleDescription() : mUserDescription;
  }

  void updateText()
  {
    setText( i18n( "Rule %1: %2", mIndex + 1, description() ) );
  }
};


RuleListEditorLBB::RuleListEditorLBB( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory )
  : QSplitter( Qt::Vertical, parent ), mComponentFactory( componentfactory ), mEditorFactory( editorComponentFactory )
{
  QWidget * base = new QWidget( this );

  QGridLayout * g = new QGridLayout( base );

  mListWidget = new QListWidget( base );
  mListWidget->setSelectionMode( QListWidget::SingleSelection );
  mListWidget->setMinimumSize( 200, 120 );

  mPreviousCurrentItem = 0;

  connect( mListWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( slotListWidgetSelectionChanged() ) );

  g->addWidget( mListWidget, 0, 0, 5, 1 );


  mNewRuleButton = new QPushButton( base );
  mNewRuleButton->setIcon( KIcon( QLatin1String( "list-add" ) ) );
  mNewRuleButton->setText( i18n( "Add Rule" ) );

  connect( mNewRuleButton, SIGNAL( clicked() ), this, SLOT( slotNewRuleButtonClicked() ) );

  g->addWidget( mNewRuleButton, 0, 1 );


  mDeleteRuleButton = new QPushButton( base );
  mDeleteRuleButton->setIcon( KIcon( QLatin1String( "list-remove" ) ) );
  mDeleteRuleButton->setText( i18n( "Delete Rule" ) );

  connect( mDeleteRuleButton, SIGNAL( clicked() ), this, SLOT( slotDeleteRuleButtonClicked() ) );

  g->addWidget( mDeleteRuleButton, 1, 1 );


  mMoveRuleUpButton = new QPushButton( base );
  mMoveRuleUpButton->setIcon( KIcon( QLatin1String( "arrow-up" ) ) );
  mMoveRuleUpButton->setText( i18n( "Move Up" ) );

  connect( mMoveRuleUpButton, SIGNAL( clicked() ), this, SLOT( slotMoveRuleUpButtonClicked() ) );

  g->addWidget( mMoveRuleUpButton, 2, 1 );


  mMoveRuleDownButton = new QPushButton( base );
  mMoveRuleDownButton->setIcon( KIcon( QLatin1String( "arrow-down" ) ) );
  mMoveRuleDownButton->setText( i18n( "Move Down" ) );

  connect( mMoveRuleDownButton, SIGNAL( clicked() ), this, SLOT( slotMoveRuleDownButtonClicked() ) );

  g->addWidget( mMoveRuleDownButton, 3, 1 );


  g->setColumnStretch( 0, 1 );
  g->setRowStretch( 4, 1 );
  g->setMargin( 0 );

  addWidget( base );


  base = new QWidget( this );

  g = new QGridLayout( base );

  mDescriptionLabel = new QLabel( base );
  mDescriptionLabel->setText( QString::fromAscii( " %1" ).arg( i18n( "Rule:" ) ) );
  mDescriptionLabel->setEnabled( false );

  g->addWidget( mDescriptionLabel, 0, 0, 1, 1 );

  mDescriptionLineEdit = new QLineEdit( base );
  mDescriptionLineEdit->setEnabled( false );

  connect( mDescriptionLineEdit, SIGNAL( textEdited( const QString & ) ), this, SLOT( slotDescriptionLineEditTextEdited( const QString & ) ) );

  mDescriptionLineEdit->setToolTip(
      i18n(
          "<p>" \
            "Put here a description of your filtering rule. " \
            "If you leave the field empty a description will be automatically generated." \
          "</p>"
        )
    );

  g->addWidget( mDescriptionLineEdit, 0, 1, 1, 1 );

  mScrollArea = new ExpandingScrollArea( this );
  mScrollArea->setAutoExpand( false );

  g->addWidget( mScrollArea, 1, 0, 1, 2 );

  g->setColumnStretch( 1, 1 );
  g->setRowStretch( 1, 1 );
  g->setMargin( 0 );

  addWidget( base );

  //mScrollArea->setFrameStyle( QFrame::NoFrame );

  QList< int > initialSizes;
  initialSizes << 120 << 380;

  setSizes( initialSizes );

  mEmptyEditor = 0;

  mHeartbeatTimer = new QTimer( this );
  connect( mHeartbeatTimer, SIGNAL( timeout() ), this, SLOT( slotHeartbeat() ) );

  mHeartbeatTimer->start( 2000 );
}

RuleListEditorLBB::~RuleListEditorLBB()
{
}

bool RuleListEditorLBB::autoExpand()
{
  return mScrollArea->autoExpand();
}

void RuleListEditorLBB::setAutoExpand( bool b )
{
  mScrollArea->setAutoExpand( b );
}

void RuleListEditorLBB::slotDescriptionLineEditTextEdited( const QString & )
{
  RuleListEditorLBBListWidgetItem * item = static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->currentItem() );
  if( !item )
    return;

  Q_ASSERT( static_cast< QListWidgetItem * >( item ) == mListWidget->item( item->index() ) );

  item->setUserDescription( mDescriptionLineEdit->text().trimmed() );

  item->updateText();  
}

void RuleListEditorLBB::activateEditor( QWidget * editor )
{
  if( mScrollArea->widget() )
  {
    QWidget * old = mScrollArea->takeWidget(); // ownership still remains, but it's not deleted on setWidget()
    old->hide();
  }

  if( !editor )
  {
    if( !mEmptyEditor )
    {
      mEmptyEditor = new QLabel( mScrollArea );
      mEmptyEditor->setText(
          i18n(
              "A <b>filter</b> is composed of a sequence of <b>rules</b>.<br>" \
              "Each rule contains a <b>condition</b> and a sequence of <b>actions</b>.<br>" \
              "If the condition matches then the actions are executed one after another.<br>" \
              "If a terminal action is encountered then filter application terminates.<br>" \
              "If the condition does not match, or no terminal action is executed,<br>" \
              "then control is given to the next rule.<br>" \
              "<br>" \
              "Now click \"Add Rule\" above to start editing the filter."
            )
        );
      mEmptyEditor->setTextFormat( Qt::RichText );
      mEmptyEditor->setAlignment( Qt::AlignCenter );
    }
    editor = mEmptyEditor;
  }

  int count = mListWidget->count();

  RuleListEditorLBBListWidgetItem * thatItem = 0;

  for( int i = 0;i < count; i++ )
  {
    RuleListEditorLBBListWidgetItem * item = static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->item( i ) );
    Q_ASSERT( item );

    if( static_cast< QWidget * >( item->editor() ) == editor )
      thatItem = item;
    else
      item->editor()->hide();
  }

  mScrollArea->setWidget( editor );
  mScrollArea->setWidgetResizable( true );

  editor->show();
  editor->raise();

  if( thatItem )
  {
    thatItem->updateText();
    if( thatItem != static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->currentItem() ) )
      mListWidget->setCurrentItem( thatItem );
  }

  updateDescriptionEdit();
}


void RuleListEditorLBB::fillFromRuleList( Action::RuleList * ruleList )
{
  mPreviousCurrentItem = 0;

  mListWidget->clear();

  const QList< Rule * > * rules = ruleList->ruleList();
  Q_ASSERT( rules );

  RuleEditor * first = 0;

  int index = 0;

  foreach( Rule * rule, *rules )
  {
    RuleEditor * ruleEditor = mEditorFactory->createRuleEditor( mScrollArea, mComponentFactory );

    Q_ASSERT( ruleEditor );

    connect( ruleEditor, SIGNAL( ruleChanged() ), this, SLOT( slotRuleChanged() ) );

    if( !first )
      first = ruleEditor;

    ruleEditor->hide(); // explicitly hidden

    ruleEditor->fillFromRule( rule );

    RuleListEditorLBBListWidgetItem * item = new RuleListEditorLBBListWidgetItem( mListWidget, ruleEditor, rule->description(), index );
    Q_UNUSED( item );

    index++;
  }

  activateEditor( first );
}

bool RuleListEditorLBB::commitStateToRuleList( Action::RuleList * ruleList )
{
  Q_ASSERT( ruleList );
  ruleList->clear();

  int count = mListWidget->count();
  for( int i = 0;i < count; i++ )
  {
    RuleListEditorLBBListWidgetItem * item = static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->item( i ) );
    Q_ASSERT( item );
    Q_UNUSED( item );

    if( item->editor()->isEmpty() )
      continue; // senseless empty rule

    Rule * rule = item->editor()->commitState( ruleList );

    if( !rule )
    {
      kDebug() << "Failed to create rule";
      activateEditor( item->editor() );
      return false; // error already shown
    }

    rule->setDescription( item->userDescription() );

    ruleList->addRule( rule );
  }

  return true;
}

void RuleListEditorLBB::reindexItems()
{
  int count = mListWidget->count();
  for( int i = 0;i < count; i++ )
  {
    RuleListEditorLBBListWidgetItem * item = static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->item( i ) );
    Q_ASSERT( item );
    item->setIndex( i );
  }

  updateDescriptionEdit();
}

void RuleListEditorLBB::slotNewRuleButtonClicked()
{
  RuleEditor * ruleEditor = mEditorFactory->createRuleEditor( mScrollArea, mComponentFactory );
  Q_ASSERT( ruleEditor );
  connect( ruleEditor, SIGNAL( ruleChanged() ), this, SLOT( slotRuleChanged() ) );

  RuleListEditorLBBListWidgetItem * item = new RuleListEditorLBBListWidgetItem( mListWidget, ruleEditor, QString(), mListWidget->count() );
  Q_UNUSED( item );

  activateEditor( ruleEditor );
}

void RuleListEditorLBB::slotDeleteRuleButtonClicked()
{
  RuleListEditorLBBListWidgetItem * item = static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->currentItem() );
  if( !item )
    return;

  int idx = item->index();

  RuleEditor * editor = item->editor();
  Q_ASSERT( editor );


  if(
     KMessageBox::questionYesNo(
         this,
         i18n( "Do you really want to delete the specified rule?" ),
         i18n( "Confirm Rule Deletion")
       ) != KMessageBox::Yes
    )
     return;

  if( mPreviousCurrentItem == item )
    mPreviousCurrentItem = 0;

  delete item;
  delete editor;

  reindexItems();

  if( idx < mListWidget->count() )
    item = static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->item( idx ) );
  else if ( mListWidget->count() > 0 )
    item = static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->item( mListWidget->count() - 1 ) );
  else
    item = 0;

  if( item )
    activateEditor( item->editor() );
  else
    activateEditor( 0 );
}

void RuleListEditorLBB::slotMoveRuleUpButtonClicked()
{
  RuleListEditorLBBListWidgetItem * item = static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->currentItem() );
  if( !item )
    return;

  Q_ASSERT( static_cast< QListWidgetItem * >( item ) == mListWidget->item( item->index() ) );

  if( item->index() < 1 )
    return;

  mListWidget->takeItem( item->index() );
  mListWidget->insertItem( item->index() - 1, item );

  reindexItems();

  activateEditor( item->editor() );
}

void RuleListEditorLBB::slotMoveRuleDownButtonClicked()
{
  RuleListEditorLBBListWidgetItem * item = static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->currentItem() );
  if( !item )
    return;

  Q_ASSERT( static_cast< QListWidgetItem * >( item ) == mListWidget->item( item->index() ) );

  if( item->index() >= ( mListWidget->count() - 1 ) )
    return;

  mListWidget->takeItem( item->index() );
  mListWidget->insertItem( item->index() + 1, item );

  reindexItems();

  activateEditor( item->editor() );
}

void RuleListEditorLBB::slotListWidgetSelectionChanged()
{
  RuleListEditorLBBListWidgetItem * item = static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->currentItem() );
  if( !item )
    return;

  if( mPreviousCurrentItem )
    mPreviousCurrentItem->updateText();

  activateEditor( item->editor() ); 
}

void RuleListEditorLBB::slotRuleChanged()
{
  RuleEditor * editor = dynamic_cast< RuleEditor * >( sender() );
  if( !editor )
    return; // hum

  int count = mListWidget->count();

  RuleListEditorLBBListWidgetItem * thatItem = 0;

  for( int i = 0;i < count; i++ )
  {
    RuleListEditorLBBListWidgetItem * item = static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->item( i ) );
    Q_ASSERT( item );

    if( item->editor() == editor )
      thatItem = item;
  }

  if( !thatItem )
    return;

  thatItem->updateText();

  updateDescriptionEdit();
}

void RuleListEditorLBB::updateDescriptionEdit()
{
  RuleListEditorLBBListWidgetItem * item = static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->currentItem() );
  if( !item )
  {
    mDescriptionLineEdit->setText( QString() );
    mDescriptionLineEdit->setEnabled( false );
    mDescriptionLabel->setText( QString::fromAscii( " %1" ).arg( i18n( "Rule:" ) ) );
    mDescriptionLabel->setEnabled( false );
    return;
  }

  mDescriptionLabel->setText( QString::fromAscii( " %1" ).arg( i18n( "Rule %1:", item->index() + 1 ) ) );
  mDescriptionLabel->setEnabled( true );

  if( mDescriptionLineEdit->hasFocus() )
    return; // don't auto-update while the user is typing

  mDescriptionLineEdit->setText( item->description() );
  mDescriptionLineEdit->setEnabled( true );
}

void RuleListEditorLBB::slotHeartbeat()
{
  RuleListEditorLBBListWidgetItem * item = static_cast< RuleListEditorLBBListWidgetItem * >( mListWidget->currentItem() );
  if( !item )
  {
    mPreviousCurrentItem = 0;
    return;
  }

  item->updateText();

  updateDescriptionEdit();
}


} // namespace UI

} // namespace Filter

} // namespace Akonadi

