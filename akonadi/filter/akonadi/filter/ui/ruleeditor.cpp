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

#include <akonadi/filter/componentfactory.h>
#include <akonadi/filter/rule.h>

#include <QLayout>

#include <KDebug>

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
};

RuleEditor::RuleEditor( QWidget * parent, ComponentFactory * componentfactory, EditorFactory * editorComponentFactory )
  : QWidget( parent ), mComponentFactory( componentfactory ), mEditorFactory( editorComponentFactory )
{
  mPrivate = new RuleEditorPrivate;

  setSizePolicy( QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );

  mPrivate->mLayout = new QVBoxLayout( this );
  mPrivate->mLayout->setMargin( 2 );

  mPrivate->mLayout->addStretch();

  mPrivate->mConditionSelector = new ConditionSelector( this, componentfactory, editorComponentFactory );
  mPrivate->mLayout->insertWidget( mPrivate->mLayout->count() - 1, mPrivate->mConditionSelector );

  ActionSelector * actionEditor = new ActionSelector( this, componentfactory, editorComponentFactory, this );
  mPrivate->mLayout->insertWidget( mPrivate->mLayout->count() - 1, actionEditor );
  mPrivate->mActionSelectorList.append( actionEditor );
}

RuleEditor::~RuleEditor()
{
  qDeleteAll( mPrivate->mActionSelectorList );
  delete mPrivate;
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
  while( actionIterator == actionList->end() )
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
  while( mPrivate->mActionSelectorList.count() > count )
  {
    delete mPrivate->mActionSelectorList.last();
    mPrivate->mActionSelectorList.removeLast();
  }

  ActionSelector * actionSelector;

  while( mPrivate->mActionSelectorList.count() < count )
  {
    actionSelector = new ActionSelector( this, mComponentFactory, mEditorFactory, this );
    mPrivate->mLayout->insertWidget( mPrivate->mLayout->count() - 1, actionSelector );
    mPrivate->mActionSelectorList.append( actionSelector );
    actionSelector->show();
  }
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

  // kill the excess of editors then
  while( countToKill > 0 )
  {
    delete mPrivate->mActionSelectorList.last();
    mPrivate->mActionSelectorList.removeLast();
    countToKill--;    
  }

  // FIXME: it would be nice to check for "continue processing" and "stop processing"
  //        in the middle and move them to the end, eventually.
}

void RuleEditor::childActionSelectorTypeChanged( ActionSelector * )
{
  fixupVisibleSelectorList();
}


QSize RuleEditor::sizeHint() const
{
  return layout()->minimumSize();
}

QSize RuleEditor::minimumSizeHint() const
{
  return layout()->minimumSize();
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi
