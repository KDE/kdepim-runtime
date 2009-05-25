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
#include <akonadi/filter/function.h>

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
  Condition::Base::ConditionType mType;
  QString mText;
  const Function * mFunction;
};

class ConditionEditorPrivate
{
public:
  QList< ConditionDescriptor * > mConditionDescriptorList; // FIXME: This could be shared between all the editors with the same factory
  KComboBox * mTypeComboBox;
  KComboBox * mDataMemberComboBox;
  KComboBox * mOperatorComboBox;
  KLineEdit * mValueLineEdit;
  QWidget * mChildConditionListBase;
  QList< ConditionEditor * > mChildConditionEditorList;
  QVBoxLayout * mChildConditionListLayout;  
};



ConditionEditor::ConditionEditor( QWidget * parent, Factory * factory, ConditionEditor * parentConditionEditor )
  : QWidget( parent ), mFactory( factory )
{
  mParentConditionEditor = parentConditionEditor;

  mPrivate = new ConditionEditorPrivate;

  QGridLayout * g = new QGridLayout( this );
  g->setMargin( 0 );

  mPrivate->mTypeComboBox = new KComboBox( false, this );
  connect( mPrivate->mTypeComboBox, SIGNAL( activated( int ) ), this, SLOT( typeComboBoxActivated( int ) ) );

  g->addWidget( mPrivate->mTypeComboBox, 0, 0, 1, 2 );

  ConditionDescriptor * d;

  d = new ConditionDescriptor;
  d->mType = Condition::Base::ConditionTypeUnknown;
  d->mText = i18n( "<please select condition type>" );
  d->mFunction = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = Condition::Base::ConditionTypeNot;
  d->mText = i18n( "if the following condition is not met" );
  d->mFunction = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = Condition::Base::ConditionTypeAnd;
  d->mText = i18n( "if all of the following conditions are met" );
  d->mFunction = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = Condition::Base::ConditionTypeOr;
  d->mText = i18n( "if any of the following conditions is met" );
  d->mFunction = 0;
  mPrivate->mConditionDescriptorList.append( d );

  const QList< const Function * > * props = mFactory->enumerateFunctions();
  Q_ASSERT( props );

  foreach( const Function *prop, *props )
  {
    d = new ConditionDescriptor;
    d->mType = Condition::Base::ConditionTypePropertyTest;
    d->mText = prop->name();
    d->mFunction = prop;
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

  mPrivate->mDataMemberComboBox = new KComboBox( false, rightBase );
  mPrivate->mDataMemberComboBox->hide();
  rightGrid->addWidget( mPrivate->mDataMemberComboBox, 0, 0 );
  
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
    case Condition::Base::ConditionTypeUnknown:
      mPrivate->mDataMemberComboBox->hide();
      mPrivate->mOperatorComboBox->hide();
      mPrivate->mValueLineEdit->hide();
      mPrivate->mChildConditionListBase->hide();
    break;
    case Condition::Base::ConditionTypePropertyTest:
      mPrivate->mDataMemberComboBox->show();
      mPrivate->mOperatorComboBox->show();
      mPrivate->mValueLineEdit->show();
      mPrivate->mChildConditionListBase->hide();
    break;
    case Condition::Base::ConditionTypeNot:
    case Condition::Base::ConditionTypeAnd:
    case Condition::Base::ConditionTypeOr:
      mPrivate->mDataMemberComboBox->hide();
      mPrivate->mOperatorComboBox->hide();
      mPrivate->mValueLineEdit->hide();

      if( mPrivate->mChildConditionEditorList.count() == 0 )
      {
        ConditionEditor * child = new ConditionEditor( mPrivate->mChildConditionListBase, mFactory, this );
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

void ConditionEditor::childEditorTypeChanged( ConditionEditor * child )
{
  ConditionDescriptor * d = descriptorForActiveType();
  if(
      ( d->mType != Condition::Base::ConditionTypeAnd ) &&
      ( d->mType != Condition::Base::ConditionTypeOr )
    )
    return; // nothing to do here

  int idx = mPrivate->mChildConditionEditorList.indexOf( child );

  Q_ASSERT( idx >= 0 );

  if( idx == ( mPrivate->mChildConditionEditorList.count() - 1 ) )
  {
    if( !child->isEmpty() )
    {
      // need a new one
      child = new ConditionEditor( mPrivate->mChildConditionListBase, mFactory, this );
      mPrivate->mChildConditionEditorList.append( child );
      mPrivate->mChildConditionListLayout->addWidget( child );
    }
  } else {
    if( child->isEmpty() )
    {
      // if the following editors are empty too: kill them
      idx++;
      while( idx < mPrivate->mChildConditionEditorList.count() ) // note that it's count() that changes here, not idx
      {
        child = mPrivate->mChildConditionEditorList.at( idx );
        Q_ASSERT( child );
        if( child->isEmpty() )
        {
          delete child;
          mPrivate->mChildConditionEditorList.removeOne( child );
        } else {
          break;
        }
      }
    }
  }
}

void ConditionEditor::typeComboBoxActivated( int )
{
  setupUIForActiveType();

  ConditionDescriptor * d = descriptorForActiveType();
  Q_ASSERT( d );

  switch( d->mType )
  {
    case Condition::Base::ConditionTypeUnknown:
      // do nothing here
    break;
    case Condition::Base::ConditionTypeAnd:
    case Condition::Base::ConditionTypeOr:
    case Condition::Base::ConditionTypeNot:
      // do nothing here
    break;
    case Condition::Base::ConditionTypePropertyTest:
      fillPropertyTestControls( d );
    break;
    default:
      Q_ASSERT( false ); //unhandled condition type
    break;
  }

  if( mParentConditionEditor )
    mParentConditionEditor->childEditorTypeChanged( this );
}

ConditionDescriptor * ConditionEditor::descriptorForActiveType()
{
  int idx = mPrivate->mTypeComboBox->currentIndex();
  if( idx < 0 )
    idx = 0;
  Q_ASSERT( idx < mPrivate->mConditionDescriptorList.count() );
  return mPrivate->mConditionDescriptorList.at( idx );
}


bool ConditionEditor::isEmpty()
{
  ConditionDescriptor * d = descriptorForActiveType();
  Q_ASSERT( d );
  return d->mType == Condition::Base::ConditionTypeUnknown;
}

void ConditionEditor::reset()
{
  ConditionDescriptor * descriptor = 0;
  int idx = 0;

  foreach( ConditionDescriptor * d, mPrivate->mConditionDescriptorList )
  {
    if( d->mType == Condition::Base::ConditionTypeUnknown )
    {
      descriptor = d;
      break;
    }
    idx++;
  }

  Q_ASSERT( descriptor );
  Q_ASSERT( idx >= 0 );

   mPrivate->mTypeComboBox->setCurrentIndex( idx );
}

void ConditionEditor::fillPropertyTestControls( ConditionDescriptor * descriptor )
{
  mPrivate->mDataMemberComboBox->clear();

  QList< const DataMember * > dataMembers = mFactory->enumerateDataMembers( descriptor->mFunction->acceptableInputDataTypeMask() );

  foreach( const DataMember * dataMember, dataMembers )
  {
    mPrivate->mDataMemberComboBox->addItem( dataMember->name() );
  }

  mPrivate->mOperatorComboBox->clear();

  const QList< const Operator * > * operators = mFactory->enumerateOperators( descriptor->mFunction->outputDataType() );
  if( !operators )
  {
    // doesn't need an operator
    mPrivate->mOperatorComboBox->hide();
    mPrivate->mValueLineEdit->hide();
    return;
  }
  foreach( const Operator * op, *operators )
  {
    mPrivate->mOperatorComboBox->addItem( op->name() );
  }
}

void ConditionEditor::fillFromCondition( Condition::Base * condition )
{
  ConditionDescriptor * descriptor = 0;
  int idx = 0;

  switch( condition->conditionType() )
  {
    case Condition::Base::ConditionTypeAnd:
    case Condition::Base::ConditionTypeOr:
    case Condition::Base::ConditionTypeNot:
      foreach( ConditionDescriptor * d, mPrivate->mConditionDescriptorList )
      {
        if( d->mType == condition->conditionType() )
        {
          descriptor = d;
          break;
        }
        idx++;
      }
    break;
    case Condition::Base::ConditionTypePropertyTest:
      foreach( ConditionDescriptor * d, mPrivate->mConditionDescriptorList )
      {
        if(
            ( d->mType == Condition::Base::ConditionTypePropertyTest ) &&
            ( d->mFunction == static_cast< Condition::PropertyTest * >( condition )->function() )
          )
        {
          descriptor = d;
          break;
        }
        idx++;
      }
    break;
    default:
      Q_ASSERT( false ); //unhandled condition type
    break;
  }

  Q_ASSERT( descriptor ); // unhandled condition type
  Q_ASSERT( idx >= 0 ); // same as above

  mPrivate->mTypeComboBox->setCurrentIndex( idx );

  switch( condition->conditionType() )
  {
    case Condition::Base::ConditionTypeAnd:
    case Condition::Base::ConditionTypeOr:
    {
      const QList< Condition::Base * > * childConditions = static_cast< Condition::Multi * >( condition )->childConditions();
      Q_ASSERT( childConditions );

      int expectedEditorCount = childConditions->count() + 1;

      ConditionEditor * child;

      while( mPrivate->mChildConditionEditorList.count() > expectedEditorCount )
      {
        child = mPrivate->mChildConditionEditorList.last();
        Q_ASSERT( child );
        delete child;
        mPrivate->mChildConditionEditorList.removeLast();
      }
      
      while( mPrivate->mChildConditionEditorList.count() < expectedEditorCount )
      {
        child = new ConditionEditor( mPrivate->mChildConditionListBase, mFactory, this );
        mPrivate->mChildConditionEditorList.append( child );
        mPrivate->mChildConditionListLayout->addWidget( child );
      }

      QList< ConditionEditor * >::Iterator editorListIterator = mPrivate->mChildConditionEditorList.begin();
      QList< Condition::Base * >::ConstIterator conditionIterator = childConditions->begin();
      while( conditionIterator != childConditions->end() )
      {
        ( *editorListIterator )->fillFromCondition( *conditionIterator );
        ++conditionIterator;
        ++editorListIterator;
      }

      Q_ASSERT( editorListIterator != mPrivate->mChildConditionEditorList.end() ); // there must be exactly another one
      ( *editorListIterator )->reset();

      ++editorListIterator;
      Q_ASSERT( editorListIterator == mPrivate->mChildConditionEditorList.end() );

    }
    break;
    case Condition::Base::ConditionTypeNot:
    {
      ConditionEditor * child;
      if( mPrivate->mChildConditionEditorList.count() != 1 )
      {
        if( mPrivate->mChildConditionEditorList.count() == 0 )
        {
          child = new ConditionEditor( mPrivate->mChildConditionListBase, mFactory, this );
          mPrivate->mChildConditionEditorList.append( child );
          mPrivate->mChildConditionListLayout->addWidget( child );
        } else {
          while( mPrivate->mChildConditionEditorList.count() != 1 )
          {
            child = mPrivate->mChildConditionEditorList.last();
            Q_ASSERT( child );
            delete child;
            mPrivate->mChildConditionEditorList.removeLast();
          }
        }
      }
      child = mPrivate->mChildConditionEditorList.last();
      Q_ASSERT( child );
      child->fillFromCondition( static_cast< Condition::Not * >( condition )->childCondition() );
    }
    break;
    case Condition::Base::ConditionTypePropertyTest:
      fillPropertyTestControls( descriptor );
    break;
    default:
      Q_ASSERT( false ); //unhandled condition type
    break;
  }

  setupUIForActiveType();

}

bool ConditionEditor::commitToCondition( Condition::Base * condition )
{
  return false;
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi
