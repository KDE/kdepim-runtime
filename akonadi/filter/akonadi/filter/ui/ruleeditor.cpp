/****************************************************************************** * *
 *
 *  File : ruleeditor.cpp
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

#include "ruleeditor.h"

#include "conditionselector.h"
#include "actionselector.h"
#include "editorfactory.h"

#include "../componentfactory.h"
#include "../rule.h"

#include <QtCore/QTimer>

#include <QtGui/QLayout>

#include <KDebug>
#include <KLocale>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

class RuleEditorPrivate
{
public:
  ConditionSelector * mConditionSelector;
  QList< ActionSelector * > mActionSelectorList;
  QVBoxLayout * mLayout;
  bool mEmitRuleChangedPending;
};

RuleEditor::RuleEditor( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory )
  : QWidget( parent ), mComponentFactory( componentfactory ), mEditorFactory( editorComponentFactory )
{
  mPrivate = new RuleEditorPrivate;

  mPrivate->mEmitRuleChangedPending = false;

  setSizePolicy( QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );

  mPrivate->mLayout = new QVBoxLayout( this );
  mPrivate->mLayout->setMargin( 2 );

  mPrivate->mLayout->addStretch();

  mPrivate->mConditionSelector = new ConditionSelector( this, componentfactory, editorComponentFactory, this );
  mPrivate->mLayout->insertWidget( mPrivate->mLayout->count() - 1, mPrivate->mConditionSelector );

  ActionSelector * actionEditor = new ActionSelector( this, componentfactory, editorComponentFactory, this, true );
  mPrivate->mLayout->insertWidget( mPrivate->mLayout->count() - 1, actionEditor );
  mPrivate->mActionSelectorList.append( actionEditor );
}

RuleEditor::~RuleEditor()
{
  qDeleteAll( mPrivate->mActionSelectorList );
  delete mPrivate;
}

bool RuleEditor::isEmpty()
{
  if( !mPrivate->mConditionSelector->isEmpty() )
    return false;
  if( mPrivate->mActionSelectorList.count() > 0 )
    return false;
  ActionSelector * selector = mPrivate->mActionSelectorList.first();
  Q_ASSERT( selector );

  if( !selector->isEmpty() )
    return false;

  return true;
}


void RuleEditor::fillFromRule( Rule * rule )
{
  mPrivate->mConditionSelector->fillFromCondition( rule->condition() );

  QList< Action::Base * > * actionList = rule->actionList();

  // kill the excess of action editors
  int expectedCount = actionList->count();
  if( expectedCount < 1 )
    expectedCount = 1;

  setSelectorCount( expectedCount );

  QList< ActionSelector * >::Iterator editorIterator = mPrivate->mActionSelectorList.begin();
  QList< Action::Base * >::Iterator actionIterator = actionList->begin();

  // fill the existing action editors
  while( actionIterator != actionList->end() )
  {
    Q_ASSERT( editorIterator != mPrivate->mActionSelectorList.end() );
    ( *editorIterator )->fillFromAction( *actionIterator );
    ++editorIterator;
    ++actionIterator;
  }

  fixupVisibleSelectorList();
}

Rule * RuleEditor::commitState( Component * parent )
{
  Rule * rule = mComponentFactory->createRule( parent );
  Q_ASSERT( rule );

  if( mPrivate->mConditionSelector->currentConditionType() != Condition::ConditionTypeUnknown )
  {
    Condition::Base * condition = mPrivate->mConditionSelector->commitState( rule );
    if( !condition )
    {
      kDebug() << "Failed to create condition";
      delete rule;
      return 0;
    }

    rule->setCondition( condition );
  }

  rule->clearActionList(); // just to make sure

  foreach( ActionSelector * sel, mPrivate->mActionSelectorList )
  {
    if( sel->currentActionType() == Action::ActionTypeUnknown )
      continue; // a null action

    Action::Base * action = sel->commitState( rule );
    if( !action )
    {
      kDebug() << "Failed to create action";
      delete rule;
      return 0;
    }

    rule->addAction( action );
  }

  return rule;
}

void RuleEditor::setSelectorCount( int count )
{
  if( count == mPrivate->mActionSelectorList.count() )
    return;

  while( mPrivate->mActionSelectorList.count() > count )
  {
    delete mPrivate->mActionSelectorList.last();
    mPrivate->mActionSelectorList.removeLast();
  }

  ActionSelector * actionSelector;

  while( mPrivate->mActionSelectorList.count() < count )
  {
    actionSelector = new ActionSelector( this, mComponentFactory, mEditorFactory, this, mPrivate->mActionSelectorList.count() == 0 );
    mPrivate->mLayout->insertWidget( mPrivate->mLayout->count() - 1, actionSelector );
    mPrivate->mActionSelectorList.append( actionSelector );
    actionSelector->show();
  }

  delayedEmitRuleChanged();
}

void RuleEditor::fixupVisibleSelectorList()
{
  Q_ASSERT( mPrivate->mActionSelectorList.count() > 0 );
  QList< ActionSelector * >::Iterator editorIterator = mPrivate->mActionSelectorList.end();
  Q_ASSERT( editorIterator != mPrivate->mActionSelectorList.begin() );
  editorIterator--;

  // now pointing to the last
  if( !( *editorIterator )->currentActionIsTerminal() )
  {
    // need an additional selector here
    setSelectorCount( mPrivate->mActionSelectorList.count() + 1 );
    return;
  }

  int countToKill = 0;

  // last is "continue processing" or "stop processing"
  // check if there are duplicates around

  while( editorIterator != mPrivate->mActionSelectorList.begin() )
  {
    editorIterator--;
    if( !( *editorIterator )->currentActionIsTerminal() )
      break;
    countToKill++;
  }  

  bool ruleHasChanged = countToKill > 0;

  // kill the excess of editors then
  while( countToKill > 0 )
  {
    delete mPrivate->mActionSelectorList.last();
    mPrivate->mActionSelectorList.removeLast();
    countToKill--;    
  }

  // FIXME: it would be nice to check for "continue processing" and "stop processing"
  //        in the middle and move them to the end, eventually.

  if( ruleHasChanged )
    delayedEmitRuleChanged();
}

void RuleEditor::childActionSelectorTypeChanged( ActionSelector * )
{
  fixupVisibleSelectorList();
  delayedEmitRuleChanged();
}

void RuleEditor::childConditionSelectorContentsChanged()
{
  delayedEmitRuleChanged();
}

void RuleEditor::delayedEmitRuleChanged()
{
  if( mPrivate->mEmitRuleChangedPending )
    return;
  mPrivate->mEmitRuleChangedPending = true;
  QTimer::singleShot( 100, this, SLOT( emitRuleChanged() ) );
}

void RuleEditor::emitRuleChanged()
{
  emit ruleChanged();
  mPrivate->mEmitRuleChangedPending = false;
}

QSize RuleEditor::sizeHint() const
{
  return layout()->minimumSize();
}

QSize RuleEditor::minimumSizeHint() const
{
  return layout()->minimumSize();
}

QString RuleEditor::ruleDescription()
{
  if( isEmpty() )
    return i18n( "empty rule" );

  QString conditionDescription = mPrivate->mConditionSelector->conditionDescription();

  QString actionsDescription;
  if( mPrivate->mActionSelectorList.count() < 1 )
  {
    actionsDescription = i18n( "do nothing" );
  } else if( mPrivate->mActionSelectorList.count() == 1 )
  {
    ActionSelector * sel = mPrivate->mActionSelectorList.first();
    Q_ASSERT( sel );
    actionsDescription = sel->actionDescription();
  } else if( mPrivate->mActionSelectorList.count() == 2 )
  {
    ActionSelector * sel = mPrivate->mActionSelectorList[1];
    Q_ASSERT( sel );
    if( sel->isEmpty() )
    {
      sel = mPrivate->mActionSelectorList.first();
      Q_ASSERT( sel );
      actionsDescription = sel->actionDescription();
    } else {
      actionsDescription = i18n( "execute multiple actions" );
    }
  } else {
    actionsDescription = i18n( "execute multiple actions" );
  }

  if( conditionDescription.isEmpty() )
    return actionsDescription;

  if( conditionDescription == i18n( "false" ) ) // ok... this is a HACK :D
    return i18n( "never %1", actionsDescription );

  return i18n( "if %1 then %2", conditionDescription, actionsDescription );

}

} // namespace UI

} // namespace Filter

} // namespace Akonadi
