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
#include <akonadi/filter/ui/rulelisteditortbb.h>
#include <akonadi/filter/ui/rulelisteditorlbb.h>

#include <akonadi/filter/rulelist.h>
#include <akonadi/filter/componentfactory.h>

#include <akonadi/filter/ui/ruleeditor.h>
#include <akonadi/filter/ui/editorfactory.h>

#include <QtGui/QLayout>

#include <KDebug>
#include <KLocale>

namespace Akonadi
{
namespace Filter
{
namespace UI
{



RuleListEditor::RuleListEditor(
    QWidget * parent,
    ComponentFactory * componentfactory,
    EditorFactory * editorComponentFactory,
    EditorStyle style
  )
  : ActionEditor( parent, componentfactory, editorComponentFactory ),
  mStyle( style )
{
  QGridLayout * g = new QGridLayout( this );

  switch( style )
  {
    case ToolBoxBased:
      mToolBoxBasedEditor = new RuleListEditorTBB( this, componentfactory, editorComponentFactory );

      g->addWidget( mToolBoxBasedEditor, 0, 0 );
      g->setMargin( 0 );
    break;
    case ListBoxBased:
      mListBoxBasedEditor = new RuleListEditorLBB( this, componentfactory, editorComponentFactory );

      g->addWidget( mListBoxBasedEditor, 0, 0 );
      g->setMargin( 0 );
    break;
    default:
      Q_ASSERT( false ); // unhandled RuleListEditor style
    break;
  }
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

  switch( mStyle )
  {
    case ToolBoxBased:
      mToolBoxBasedEditor->fillFromRuleList( ruleList );  
    break;
    case ListBoxBased:
      mListBoxBasedEditor->fillFromRuleList( ruleList );  
    break;
    default:
      Q_ASSERT( false ); // unhandled RuleListEditor style
    break;
  }
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

  switch( mStyle )
  {
    case ToolBoxBased:
      return mToolBoxBasedEditor->commitStateToRuleList( ruleList );  
    break;
    case ListBoxBased:
      return mListBoxBasedEditor->commitStateToRuleList( ruleList );  
    break;
    default:
      Q_ASSERT( false ); // unhandled RuleListEditor style
    break;
  }
  return false;
}

bool RuleListEditor::autoExpand() const
{
  switch( mStyle )
  {
    case ToolBoxBased:
      return mToolBoxBasedEditor->autoExpand();
    break;
    case ListBoxBased:
      return mListBoxBasedEditor->autoExpand();
    break;
    default:
      Q_ASSERT( false ); // unhandled RuleListEditor style
    break;
  }
  return false;
}

void RuleListEditor::setAutoExpand( bool b )
{
  switch( mStyle )
  {
    case ToolBoxBased:
      mToolBoxBasedEditor->setAutoExpand( b );
    break;
    case ListBoxBased:
      mListBoxBasedEditor->setAutoExpand( b );
    break;
    default:
      Q_ASSERT( false ); // unhandled RuleListEditor style
    break;
  }
}


} // namespace UI

} // namespace Filter

} // namespace Akonadi
