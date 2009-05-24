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

RuleEditor::RuleEditor( QWidget * parent, Factory * factory )
  : QWidget( parent ), mFactory( factory )
{
  setSizePolicy( QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );

  QGridLayout * g = new QGridLayout( this );
  g->setMargin( 2 );

  mConditionEditor = new ConditionEditor( this, factory );
  g->addWidget( mConditionEditor, 0, 0 );

  mActionEditor = new ActionEditor( this, factory );
  g->addWidget( mActionEditor, 1, 0 );

  g->setRowStretch( 2, 1 );
}

RuleEditor::~RuleEditor()
{
}

void RuleEditor::fillFromRule( Rule * rule )
{
  mConditionEditor->fillFromCondition( rule->condition() );
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
