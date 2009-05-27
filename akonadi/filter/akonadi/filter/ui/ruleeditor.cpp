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

#include "conditioneditor.h"
#include "actioneditor.h"

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
  ConditionEditor * mConditionEditor;
  QList< ActionEditor * > mActionEditorList;
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

  mPrivate->mConditionEditor = new ConditionEditor( this, factory );
  mPrivate->mLayout->insertWidget( mPrivate->mLayout->count() - 1, mPrivate->mConditionEditor );

  ActionEditor * actionEditor = new ActionEditor( this, factory );
  mPrivate->mLayout->insertWidget( mPrivate->mLayout->count() - 1, actionEditor );
  mPrivate->mActionEditorList.append( actionEditor );
}

RuleEditor::~RuleEditor()
{
  qDeleteAll( mPrivate->mActionEditorList );
  delete mPrivate;
}

void RuleEditor::fillFromRule( Rule * rule )
{
  mPrivate->mConditionEditor->fillFromCondition( rule->condition() );

  QList< Action::Base * > * actionList = rule->actionList();

  // kill the excess of action editors
  while( mPrivate->mActionEditorList.count() > actionList->count() )
  {
    delete mPrivate->mActionEditorList.last();
    mPrivate->mActionEditorList.removeLast();
  }

  QList< ActionEditor * >::Iterator editorIterator = mPrivate->mActionEditorList.begin();
  QList< Action::Base * >::Iterator actionIterator = actionList->begin();

  // fill the existing action editors
  while( editorIterator != mPrivate->mActionEditorList.end() )
  {
    Q_ASSERT( actionIterator != actionList->end() );
    ( *editorIterator )->fillFromAction( *actionIterator );
    ++editorIterator;
    ++actionIterator;
  }

  ActionEditor * actionEditor;

  // add new ones, if needed
  while( actionIterator != actionList->end() )
  {
    actionEditor = new ActionEditor( this, mFactory );
    mPrivate->mLayout->insertWidget( mPrivate->mLayout->count() - 1, actionEditor );
    mPrivate->mActionEditorList.append( actionEditor );
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
