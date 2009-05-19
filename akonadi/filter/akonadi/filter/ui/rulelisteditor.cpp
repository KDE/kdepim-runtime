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

#include "ruleeditor.h"

#include <QScrollArea>
#include <QToolBox>
#include <QResizeEvent>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

RuleListEditor::RuleListEditor( QWidget * parent, Action::RuleList * ruleList )
  : QWidget( parent ), mRuleList( ruleList )
{
  mScrollArea = new QScrollArea( this );
  mToolBox = new QToolBox( mScrollArea );
  mScrollArea->setWidget( mToolBox );
  mScrollArea->setWidgetResizable( true );
  for( int i = 0; i < 10; i++ )
  {
    RuleEditor * ruleEditor = new RuleEditor( mToolBox, 0 );
    //ruleEditor->setMinimumSize(120,300);
    mToolBox->addItem( ruleEditor, QString::fromAscii( "Test" ) );
  }
}

RuleListEditor::~RuleListEditor()
{
}

void RuleListEditor::resizeEvent( QResizeEvent * e )
{
  QWidget::resizeEvent( e );

#define MARGIN 2

  mScrollArea->setGeometry( MARGIN, MARGIN, width() - ( 2 * MARGIN ), height() - ( 2 * MARGIN ) );
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi
