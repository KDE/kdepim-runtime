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

#include <akonadi/filter/factory.h>
#include <akonadi/filter/rule.h>

#include <QLayout>

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

RuleEditor::RuleEditor( QWidget * parent, Factory * factory )
  : QWidget( parent ), mFactory( factory )
{
  mPrivate = new RuleEditorPrivate;

  setSizePolicy( QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );

  mPrivate->mLayout = new QVBoxLayout( this );
  mPrivate->mLayout->setMargin( 2 );

  mPrivate->mLayout->addStretch();

  mPrivate->mConditionSelector = new ConditionSelector( this, factory );
  mPrivate->mLayout->insertWidget( mPrivate->mLayout->count() - 1, mPrivate->mConditionSelector );

  ActionSelector * actionEditor = new ActionSelector( this, factory );
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
  while( mPrivate->mActionSelectorList.count() > actionList->count() )
  {
    delete mPrivate->mActionSelectorList.last();
    mPrivate->mActionSelectorList.removeLast();
  }

  QList< ActionSelector * >::Iterator editorIterator = mPrivate->mActionSelectorList.begin();
  QList< Action::Base * >::Iterator actionIterator = actionList->begin();

  // fill the existing action editors
  while( editorIterator != mPrivate->mActionSelectorList.end() )
  {
    Q_ASSERT( actionIterator != actionList->end() );
    ( *editorIterator )->fillFromAction( *actionIterator );
    ++editorIterator;
    ++actionIterator;
  }

  ActionSelector * actionEditor;

  // add new ones, if needed
  while( actionIterator != actionList->end() )
  {
    actionEditor = new ActionSelector( this, mFactory );
    mPrivate->mLayout->insertWidget( mPrivate->mLayout->count() - 1, actionEditor );
    mPrivate->mActionSelectorList.append( actionEditor );
    actionEditor->show();
    actionEditor->fillFromAction( *actionIterator );
    ++actionIterator;
  }
}

bool RuleEditor::commitToRule( Rule * rule )
{
  return false;
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
