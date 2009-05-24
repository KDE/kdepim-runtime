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
#include <akonadi/filter/factory.h>
#include <akonadi/filter/property.h>

#include <KComboBox>
#include <KLineEdit>
#include <KLocale>

#include <QLayout>
#include <QList>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

#define INDENT 20

class ConditionDescriptor
{
public:
  ConditionEditor::ConditionType mType;
  QString mText;
  const Property * mProperty;
};

class ConditionEditorPrivate
{
public:
  QList< ConditionDescriptor * > mConditionDescriptorList;
  KComboBox * mTypeComboBox;
  KComboBox * mFieldComboBox;
  KComboBox * mOperatorComboBox;
  KLineEdit * mValueLineEdit;
  QWidget * mChildConditionListBase;
  QList< ConditionEditor * > mChildConditionEditorList;
  QVBoxLayout * mChildConditionListLayout;  
};



ConditionEditor::ConditionEditor( QWidget * parent, Factory * factory )
  : QWidget( parent ), mFactory( factory )
{
  mPrivate = new ConditionEditorPrivate;

  QGridLayout * g = new QGridLayout( this );
  g->setMargin( 0 );

  mPrivate->mTypeComboBox = new KComboBox( false, this );
  connect( mPrivate->mTypeComboBox, SIGNAL( activated( int ) ), this, SLOT( typeComboBoxActivated( int ) ) );

  g->addWidget( mPrivate->mTypeComboBox, 0, 0, 1, 2 );

  ConditionDescriptor * d;

  d = new ConditionDescriptor;
  d->mType = ConditionTypeNone;
  d->mText = i18n( "<please select condition type>" );
  d->mProperty = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = ConditionTypeNot;
  d->mText = i18n( "if the following condition is not met" );
  d->mProperty = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = ConditionTypeAnd;
  d->mText = i18n( "if all of the following conditions are met" );
  d->mProperty = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = ConditionTypeOr;
  d->mText = i18n( "if any of the following conditions is met" );
  d->mProperty = 0;
  mPrivate->mConditionDescriptorList.append( d );

  const QList< const Property * > * props = mFactory->enumerateProperties();
  Q_ASSERT( props );

  foreach( const Property *prop, *props )
  {
    d = new ConditionDescriptor;
    d->mType = ConditionTypePropertyTest;
    d->mText = prop->longName();
    d->mProperty = prop;
    mPrivate->mConditionDescriptorList.append( d );
  }

  int idx = 0;

  foreach( d, mPrivate->mConditionDescriptorList )
  {
    mPrivate->mTypeComboBox->addItem( d->mText, idx );
    idx++;
  }


  QWidget * rightBase = new QWidget( this );
  rightBase->setFixedWidth( 350 );
  g->addWidget( rightBase, 0, 2, 1, 1 );

  QGridLayout * rightGrid = new QGridLayout( rightBase );
  rightGrid->setMargin( 0 );

  mPrivate->mFieldComboBox = new KComboBox( false, rightBase );
  mPrivate->mFieldComboBox->hide();
  rightGrid->addWidget( mPrivate->mFieldComboBox, 0, 0 );
  
  mPrivate->mOperatorComboBox = new KComboBox( false, rightBase );
  mPrivate->mOperatorComboBox->hide();
  rightGrid->addWidget( mPrivate->mOperatorComboBox, 0, 1 );

  mPrivate->mValueLineEdit = new KLineEdit( rightBase );
  mPrivate->mValueLineEdit->hide();
  rightGrid->addWidget( mPrivate->mValueLineEdit, 0, 2 );

  mPrivate->mChildConditionListBase = new QWidget( this );
  mPrivate->mChildConditionListBase->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

  mPrivate->mChildConditionListLayout = new QVBoxLayout( mPrivate->mChildConditionListBase );
  mPrivate->mChildConditionListLayout->setMargin( 0 );

  g->addWidget( mPrivate->mChildConditionListBase, 1, 1, 1, 2 );

  mPrivate->mChildConditionListBase->hide();

  g->setColumnMinimumWidth( 0, INDENT );
  g->setColumnStretch( 1, 1 );
  
}

ConditionEditor::~ConditionEditor()
{
  qDeleteAll( mPrivate->mConditionDescriptorList );
  delete mPrivate;
}

void ConditionEditor::setupUIForActiveType()
{
  int index = mPrivate->mTypeComboBox->currentIndex();
  if( index < 0 )
    return;

  QVariant data = mPrivate->mTypeComboBox->itemData( index );
  if( data.isNull() )
    return;

  bool ok;
  index = data.toInt( &ok );
  if( !ok )
    return;

  Q_ASSERT( ( index >= 0 ) && ( index < mPrivate->mConditionDescriptorList.count() ) );

  ConditionDescriptor * d = mPrivate->mConditionDescriptorList.at( index );
  Q_ASSERT( d );

  switch( d->mType )
  {
    case ConditionTypeNone:
      mPrivate->mFieldComboBox->hide();
      mPrivate->mOperatorComboBox->hide();
      mPrivate->mValueLineEdit->hide();
      mPrivate->mChildConditionListBase->hide();
    break;
    case ConditionTypePropertyTest:
    {
      mPrivate->mOperatorComboBox->clear();
      
      mPrivate->mFieldComboBox->show();
      mPrivate->mOperatorComboBox->show();
      mPrivate->mValueLineEdit->show();
      mPrivate->mChildConditionListBase->hide();
    }
    break;
    case ConditionTypeNot:
    case ConditionTypeAnd:
    case ConditionTypeOr:
      mPrivate->mFieldComboBox->hide();
      mPrivate->mOperatorComboBox->hide();
      mPrivate->mValueLineEdit->hide();

      if( mPrivate->mChildConditionEditorList.isEmpty() )
      {
        ConditionEditor * child = new ConditionEditor( mPrivate->mChildConditionListBase, mFactory );
        mPrivate->mChildConditionEditorList.append( child );
        mPrivate->mChildConditionListLayout->addWidget( child );
      }

      mPrivate->mChildConditionListBase->show();
    break;
    default:
      Q_ASSERT( false ); // unhandled condition type
    break;
  }

}

void ConditionEditor::typeComboBoxActivated( int )
{
  setupUIForActiveType();
}

void ConditionEditor::fillFromCondition( Condition::Base * condition )
{
  setupUIForActiveType();
}

bool ConditionEditor::commitToCondition( Condition::Base * condition )
{
  return false;
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi
