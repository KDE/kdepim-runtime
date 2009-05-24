/****************************************************************************** * *
 *
 *  File : actioneditor.cpp
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

#include "actioneditor.h"

#include <akonadi/filter/action.h>
#include <akonadi/filter/factory.h>

#include <QLayout>

#include <KComboBox>
#include <KLocale>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

#define INDENT 20

ActionEditor::ActionEditor( QWidget * parent, Factory * factory )
  : QWidget( parent ), mFactory( factory )
{

  QGridLayout * g = new QGridLayout( this );
  g->setMargin( 1 );

  mTypeComboBox = new KComboBox( false, this );

  g->addWidget( mTypeComboBox, 0, 0, 1, 2 );

  mTypeComboBox->addItem( i18n( "then stop processing here" ) );
  mTypeComboBox->addItem( i18n( "then execute the following rule list (sub-program)" ) );
  mTypeComboBox->addItem( i18n( "then move the item in the collection" ) );
  mTypeComboBox->addItem( i18n( "then delete the item" ) );
  mTypeComboBox->addItem( i18n( "then tag the item" ) );

  g->setColumnMinimumWidth( 0, INDENT );
}

ActionEditor::~ActionEditor()
{
}

void ActionEditor::fillFromAction( Action::Base * action )
{
}

bool ActionEditor::commitToAction( Action::Base * action )
{
  return false;
}


} // namespace UI

} // namespace Filter

} // namespace Akonadi
