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

#include "coolcombobox.h"

#include <QLayout>

#include <KLocale>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

#define INDENT 20

class ActionEditorPrivate
{
public:
  Private::CoolComboBox * mTypeComboBox;
};

ActionEditor::ActionEditor( QWidget * parent, Factory * factory )
  : QWidget( parent ), mFactory( factory )
{
  mPrivate = new ActionEditorPrivate;

  QGridLayout * g = new QGridLayout( this );
  g->setMargin( 1 );

  mPrivate->mTypeComboBox = new Private::CoolComboBox( false, this );
  mPrivate->mTypeComboBox->setOverlayColor( Qt::yellow );

  g->addWidget( mPrivate->mTypeComboBox, 0, 0, 1, 2 );

  mPrivate->mTypeComboBox->addItem( i18n( "then continue processing" ) );
  mPrivate->mTypeComboBox->addItem( i18n( "then stop processing here" ) );
  mPrivate->mTypeComboBox->addItem( i18n( "then execute the following rule list (sub-program)" ) );
  mPrivate->mTypeComboBox->addItem( i18n( "then move the item in the collection" ) );
  mPrivate->mTypeComboBox->addItem( i18n( "then delete the item" ) );
  mPrivate->mTypeComboBox->addItem( i18n( "then tag the item" ) );

  g->setColumnMinimumWidth( 0, INDENT );
}

ActionEditor::~ActionEditor()
{
  delete mPrivate;
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
