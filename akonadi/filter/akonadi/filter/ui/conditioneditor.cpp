/****************************************************************************** * *
 *
 *  File : conditioneditor.cpp
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

#include "conditioneditor.h"

#include <akonadi/filter/condition.h>

#include <KComboBox>
#include <KLocale>

#include <QLayout>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

#define INDENT 20

ConditionEditor::ConditionEditor( QWidget * parent, Condition::Base * condition )
  : QWidget( parent ), mCondition( condition )
{
  QGridLayout * g = new QGridLayout( this );
  g->setMargin( 1 );

  mTypeComboBox = new KComboBox( false, this );

  g->addWidget( mTypeComboBox, 0, 0, 1, 2 );

  mTypeComboBox->addItem( i18n( "if the following condition is not met" ) );
  mTypeComboBox->addItem( i18n( "if the value of the field" ) );
  mTypeComboBox->addItem( i18n( "if the size of the field" ) );
  mTypeComboBox->addItem( i18n( "if the field exists" ) );
  mTypeComboBox->addItem( i18n( "if all of the following conditions are met" ) );
  mTypeComboBox->addItem( i18n( "if any of the following conditions is met" ) );

  g->setColumnMinimumWidth( 0, INDENT );
  
}

ConditionEditor::~ConditionEditor()
{
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi
