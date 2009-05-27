/****************************************************************************** * *
 *
 *  File : conditionselector.cpp
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

#include "conditionselector.h"

#include <akonadi/filter/condition.h>
#include <akonadi/filter/factory.h>
#include <akonadi/filter/functiondescriptor.h>

#include "coolcombobox.h"
#include "extensionlabel.h"

#include <KLineEdit>
#include <KLocale>

#include <QLayout>
#include <QList>

static int gSpacing = -1; // 0 would be nice for several reasons, but it looks confusing in windows and cde styles... -1 means default
static int gIndent = 20;
static const qreal gSemiTransparentWidgetsOpacity = 0.32;

//#define NON_PROPERTY_TEST_CONDITION_BOXES_EXTEND_TO_THE_RIGHT 1
#define PROPERTY_TEST_CONDITION_WIDGETS_APPEAR_BELOW 1

namespace Akonadi
{
namespace Filter
{
namespace UI
{


class ConditionDescriptor
{
public:
  Condition::ConditionType mType;
  QString mText;
  QColor mColor;
  const FunctionDescriptor * mFunctionDescriptor;
};

class ConditionSelectorPrivate
{
public:
  QList< ConditionDescriptor * > mConditionDescriptorList; // FIXME: This could be shared between all the editors with the same factory
  Private::CoolComboBox * mTypeComboBox;
  KComboBox * mDataMemberDescriptorComboBox;
  Private::ExtensionLabel * mExtensionLabel;
  KComboBox * mOperatorDescriptorComboBox;
  KLineEdit * mValueLineEdit;
  QWidget * mChildConditionListBase;
  QWidget * mRightControlsBase;
  QList< ConditionSelector * > mChildConditionSelectorList;
  QVBoxLayout * mChildConditionListLayout;  
};



ConditionSelector::ConditionSelector( QWidget * parent, Factory * factory, ConditionSelector * parentConditionSelector )
  : QWidget( parent ), mFactory( factory )
{
  mParentConditionSelector = parentConditionSelector;

  mPrivate = new ConditionSelectorPrivate;

  QGridLayout * g = new QGridLayout( this );
  g->setMargin( 0 );
  if( gSpacing != -1 )
    g->setSpacing( gSpacing );

  mPrivate->mTypeComboBox = new Private::CoolComboBox( false, this );
  mPrivate->mTypeComboBox->setOpacity( gSemiTransparentWidgetsOpacity );
  connect( mPrivate->mTypeComboBox, SIGNAL( activated( int ) ), this, SLOT( typeComboBoxActivated( int ) ) );

  g->addWidget( mPrivate->mTypeComboBox, 0, 0, 1, 2 );

  ConditionDescriptor * d;

  d = new ConditionDescriptor;
  d->mType = Condition::ConditionTypeUnknown;
  d->mText = i18n( "<...select to activate condition...>" );
  d->mColor = palette().color( QPalette::Button );
  d->mFunctionDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = Condition::ConditionTypeNot;
  d->mText = i18n( "if the condition below is NOT met" );
  d->mColor = Qt::red;
  d->mFunctionDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = Condition::ConditionTypeAnd;
  d->mText = i18n( "if ALL of the conditions below are met" );
  d->mColor = Qt::blue;
  d->mFunctionDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = Condition::ConditionTypeOr;
  d->mText = i18n( "if ANY of the conditions below is met" );
  d->mColor = Qt::darkGreen;
  d->mFunctionDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  const QList< const FunctionDescriptor * > * props = mFactory->enumerateFunctions();
  Q_ASSERT( props );

  foreach( const FunctionDescriptor *prop, *props )
  {
    d = new ConditionDescriptor;
    d->mType = Condition::ConditionTypePropertyTest;
    d->mText = prop->name();
    d->mColor = QColor(); // no overlay
    d->mFunctionDescriptor = prop;
    mPrivate->mConditionDescriptorList.append( d );
  }

  d = new ConditionDescriptor;
  d->mType = Condition::ConditionTypeTrue;
  d->mText = i18n( "if true (so always)" );
  d->mColor = QColor( 0, 60, 0 );
  d->mFunctionDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = Condition::ConditionTypeFalse;
  d->mText = i18n( "if false (so never)" );
  d->mColor = QColor( 60, 0, 0 );
  d->mFunctionDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  int idx = 0;

  QColor clrText = palette().color( QPalette::Text );

  foreach( d, mPrivate->mConditionDescriptorList )
  {
    mPrivate->mTypeComboBox->addItem( d->mText, idx );
    if( d->mColor.isValid() )
    {
      QColor clrMerged = QColor::fromRgb(
          ( clrText.red() + d->mColor.red() ) / 2,
          ( clrText.green() + d->mColor.green() ) / 2,
          ( clrText.blue() + d->mColor.blue() ) / 2
        );
      mPrivate->mTypeComboBox->setItemData( idx, QVariant( clrMerged ), Qt::ForegroundRole );
    }
    idx++;
  }

  mPrivate->mRightControlsBase = new QWidget( this );
#ifndef PROPERTY_TEST_CONDITION_WIDGETS_APPEAR_BELOW
  mPrivate->mRightControlsBase->setFixedWidth( 450 );
  g->addWidget( mPrivate->mRightControlsBase, 0, 2, 1, 1 );
#endif

#ifdef NON_PROPERTY_TEST_CONDITION_BOXES_EXTEND_TO_THE_RIGHT
  mPrivate->mRightControlsBase->hide();
#endif //NON_PROPERTY_TEST_CONDITION_BOXES_EXTEND_TO_THE_RIGHT

  QGridLayout * rightGrid = new QGridLayout( mPrivate->mRightControlsBase );
  rightGrid->setMargin( 0 );
  if( gSpacing != -1 )
    rightGrid->setSpacing( gSpacing );

  mPrivate->mDataMemberDescriptorComboBox = new KComboBox( false, mPrivate->mRightControlsBase );
  mPrivate->mDataMemberDescriptorComboBox->hide();
  rightGrid->addWidget( mPrivate->mDataMemberDescriptorComboBox, 0, 0 );
#ifdef PROPERTY_TEST_CONDITION_WIDGETS_APPEAR_BELOW
  rightGrid->setColumnStretch( 0, 1 );
#endif
  
  mPrivate->mOperatorDescriptorComboBox = new KComboBox( false, mPrivate->mRightControlsBase );
  mPrivate->mOperatorDescriptorComboBox->hide();
#ifdef PROPERTY_TEST_CONDITION_WIDGETS_APPEAR_BELOW
  mPrivate->mOperatorDescriptorComboBox->setFixedWidth( 200 );
#endif
  rightGrid->addWidget( mPrivate->mOperatorDescriptorComboBox, 0, 1 );

  mPrivate->mValueLineEdit = new KLineEdit( mPrivate->mRightControlsBase );
  mPrivate->mValueLineEdit->hide();
#ifdef PROPERTY_TEST_CONDITION_WIDGETS_APPEAR_BELOW
  mPrivate->mValueLineEdit->setFixedWidth( 180 );
#endif
  rightGrid->addWidget( mPrivate->mValueLineEdit, 0, 2 );

  mPrivate->mExtensionLabel = new Private::ExtensionLabel( this );
  g->addWidget( mPrivate->mExtensionLabel, 1, 0, 1, 1 );

  mPrivate->mChildConditionListBase = new QWidget( this );
  mPrivate->mChildConditionListBase->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

  mPrivate->mChildConditionListLayout = new QVBoxLayout( mPrivate->mChildConditionListBase );
  mPrivate->mChildConditionListLayout->setMargin( 0 );
  if( gSpacing != -1 )
    mPrivate->mChildConditionListLayout->setSpacing( gSpacing );

#ifdef PROPERTY_TEST_CONDITION_WIDGETS_APPEAR_BELOW
  mPrivate->mChildConditionListLayout->addWidget( mPrivate->mRightControlsBase );
  mPrivate->mRightControlsBase->hide();
  g->addWidget( mPrivate->mChildConditionListBase, 1, 1, 1, 1 );
#else
  g->addWidget( mPrivate->mChildConditionListBase, 1, 1, 1, 2 );
#endif

  mPrivate->mChildConditionListBase->hide();

  g->setColumnMinimumWidth( 0, gIndent );
  g->setColumnStretch( 1, 1 );
}

ConditionSelector::~ConditionSelector()
{
  qDeleteAll( mPrivate->mConditionDescriptorList );
  delete mPrivate;
}

QSize ConditionSelector::sizeHint() const
{
  return layout()->minimumSize();
}

QSize ConditionSelector::minimumSizeHint() const
{
  return layout()->minimumSize();
}


void ConditionSelector::setupUIForActiveType()
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

  mPrivate->mTypeComboBox->setOverlayColor( d->mColor );

  switch( d->mType )
  {
    case Condition::ConditionTypeUnknown:
#if defined(NON_PROPERTY_TEST_CONDITION_BOXES_EXTEND_TO_THE_RIGHT) || defined(PROPERTY_TEST_CONDITION_WIDGETS_APPEAR_BELOW)
      mPrivate->mRightControlsBase->hide();
#endif //NON_PROPERTY_TEST_CONDITION_BOXES_EXTEND_TO_THE_RIGHT
      mPrivate->mTypeComboBox->setOpacity( gSemiTransparentWidgetsOpacity );
      mPrivate->mDataMemberDescriptorComboBox->hide();
      mPrivate->mOperatorDescriptorComboBox->hide();
      mPrivate->mValueLineEdit->hide();
      mPrivate->mExtensionLabel->hide();
      mPrivate->mChildConditionListBase->hide();
    break;
    case Condition::ConditionTypeTrue:
    case Condition::ConditionTypeFalse:
#if defined(NON_PROPERTY_TEST_CONDITION_BOXES_EXTEND_TO_THE_RIGHT) || defined(PROPERTY_TEST_CONDITION_WIDGETS_APPEAR_BELOW)
      mPrivate->mRightControlsBase->hide();
#endif //NON_PROPERTY_TEST_CONDITION_BOXES_EXTEND_TO_THE_RIGHT
      mPrivate->mTypeComboBox->setOpacity( 1.0 );
      mPrivate->mDataMemberDescriptorComboBox->hide();
      mPrivate->mOperatorDescriptorComboBox->hide();
      mPrivate->mValueLineEdit->hide();
      mPrivate->mExtensionLabel->hide();
      mPrivate->mChildConditionListBase->hide();
    break;
    case Condition::ConditionTypePropertyTest:
#if defined(NON_PROPERTY_TEST_CONDITION_BOXES_EXTEND_TO_THE_RIGHT) || defined(PROPERTY_TEST_CONDITION_WIDGETS_APPEAR_BELOW)
      mPrivate->mRightControlsBase->show();
#endif //NON_PROPERTY_TEST_CONDITION_BOXES_EXTEND_TO_THE_RIGHT

      mPrivate->mTypeComboBox->setOpacity( 1.0 );
      mPrivate->mDataMemberDescriptorComboBox->show();
      mPrivate->mOperatorDescriptorComboBox->show();
      mPrivate->mValueLineEdit->show();
#ifdef PROPERTY_TEST_CONDITION_WIDGETS_APPEAR_BELOW
      mPrivate->mChildConditionListBase->show();
      mPrivate->mExtensionLabel->show();
      mPrivate->mExtensionLabel->setOverlayColor( Qt::black );
#else
      mPrivate->mChildConditionListBase->hide();
      mPrivate->mExtensionLabel->hide();
#endif
    break;
    case Condition::ConditionTypeNot:
    case Condition::ConditionTypeAnd:
    case Condition::ConditionTypeOr:
#if defined(NON_PROPERTY_TEST_CONDITION_BOXES_EXTEND_TO_THE_RIGHT) || defined(PROPERTY_TEST_CONDITION_WIDGETS_APPEAR_BELOW)
      mPrivate->mRightControlsBase->hide();
#endif //NON_PROPERTY_TEST_CONDITION_BOXES_EXTEND_TO_THE_RIGHT
      mPrivate->mTypeComboBox->setOpacity( 1.0 );
      mPrivate->mExtensionLabel->setFixedHeight( ( d->mType == Condition::ConditionTypeNot ) ? ( mPrivate->mTypeComboBox->sizeHint().height() - 5 ) : -1 );
      mPrivate->mExtensionLabel->setOverlayColor( d->mColor );
      mPrivate->mDataMemberDescriptorComboBox->hide();
      mPrivate->mOperatorDescriptorComboBox->hide();
      mPrivate->mValueLineEdit->hide();

      if( mPrivate->mChildConditionSelectorList.count() == 0 )
      {
        ConditionSelector * child = new ConditionSelector( mPrivate->mChildConditionListBase, mFactory, this );
        mPrivate->mChildConditionSelectorList.append( child );
        mPrivate->mChildConditionListLayout->addWidget( child );
      }

      mPrivate->mChildConditionListBase->show();
      mPrivate->mExtensionLabel->show();
    break;
    default:
      Q_ASSERT( false ); // unhandled condition type
    break;
  }

}

void ConditionSelector::childEditorTypeChanged( ConditionSelector * child )
{
  ConditionDescriptor * d = descriptorForActiveType();

  if(
      ( d->mType != Condition::ConditionTypeAnd ) &&
      ( d->mType != Condition::ConditionTypeOr )
    )
    return; // nothing to do here

  int idx = mPrivate->mChildConditionSelectorList.indexOf( child );

  Q_ASSERT( idx >= 0 );

  if( idx == ( mPrivate->mChildConditionSelectorList.count() - 1 ) )
  {
    if( !child->isEmpty() )
    {
      // need a new one
      child = new ConditionSelector( mPrivate->mChildConditionListBase, mFactory, this );
      mPrivate->mChildConditionSelectorList.append( child );
      mPrivate->mChildConditionListLayout->addWidget( child );
    }
  } else {
    if( child->isEmpty() )
    {
      // if the following editors are empty too: kill them
      idx++;
      while( idx < mPrivate->mChildConditionSelectorList.count() ) // note that it's count() that changes here, not idx
      {
        child = mPrivate->mChildConditionSelectorList.at( idx );
        Q_ASSERT( child );
        if( child->isEmpty() )
        {
          delete child;
          mPrivate->mChildConditionSelectorList.removeOne( child );
        } else {
          break;
        }
      }
    }
  }
}

void ConditionSelector::typeComboBoxActivated( int )
{
  setupUIForActiveType();

  ConditionDescriptor * d = descriptorForActiveType();
  Q_ASSERT( d );

  switch( d->mType )
  {
    case Condition::ConditionTypeUnknown:
      // do nothing here
    break;
    case Condition::ConditionTypeFalse:
    case Condition::ConditionTypeTrue:
      // do nothing here
    break;
    case Condition::ConditionTypeAnd:
    case Condition::ConditionTypeOr:
    case Condition::ConditionTypeNot:
      // do nothing here
    break;
    case Condition::ConditionTypePropertyTest:
      qDeleteAll( mPrivate->mChildConditionSelectorList );
      mPrivate->mChildConditionSelectorList.clear();
      fillPropertyTestControls( d );
    break;
    default:
      Q_ASSERT( false ); //unhandled condition type
    break;
  }

  if( mParentConditionSelector )
    mParentConditionSelector->childEditorTypeChanged( this );
}

ConditionDescriptor * ConditionSelector::descriptorForActiveType()
{
  int idx = mPrivate->mTypeComboBox->currentIndex();
  if( idx < 0 )
    idx = 0;
  Q_ASSERT( idx < mPrivate->mConditionDescriptorList.count() );
  return mPrivate->mConditionDescriptorList.at( idx );
}


bool ConditionSelector::isEmpty()
{
  ConditionDescriptor * d = descriptorForActiveType();
  Q_ASSERT( d );
  return d->mType == Condition::ConditionTypeUnknown;
}

void ConditionSelector::reset()
{
  ConditionDescriptor * descriptor = 0;
  int idx = 0;

  foreach( ConditionDescriptor * d, mPrivate->mConditionDescriptorList )
  {
    if( d->mType == Condition::ConditionTypeUnknown )
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

void ConditionSelector::fillPropertyTestControls( ConditionDescriptor * descriptor )
{
  mPrivate->mDataMemberDescriptorComboBox->clear();

  QList< const DataMemberDescriptor * > dataMembers = mFactory->enumerateDataMembers( descriptor->mFunctionDescriptor->acceptableInputDataTypeMask() );

  foreach( const DataMemberDescriptor * dataMember, dataMembers )
  {
    mPrivate->mDataMemberDescriptorComboBox->addItem( dataMember->name() );
  }

  mPrivate->mOperatorDescriptorComboBox->clear();

  const QList< const OperatorDescriptor * > operators = mFactory->enumerateOperators( descriptor->mFunctionDescriptor->outputDataTypeMask() );
  if( operators.isEmpty() )
  {
    // doesn't need an operator
    mPrivate->mOperatorDescriptorComboBox->hide();
    mPrivate->mValueLineEdit->hide();
    return;
  }
  foreach( const OperatorDescriptor * op, operators )
  {
    mPrivate->mOperatorDescriptorComboBox->addItem( op->name() );
  }
}

void ConditionSelector::fillFromCondition( Condition::Base * condition )
{
  ConditionDescriptor * descriptor = 0;
  int idx = 0;

  switch( condition->conditionType() )
  {
    case Condition::ConditionTypeAnd:
    case Condition::ConditionTypeOr:
    case Condition::ConditionTypeNot:
    case Condition::ConditionTypeFalse:
    case Condition::ConditionTypeTrue:
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
    case Condition::ConditionTypePropertyTest:
      foreach( ConditionDescriptor * d, mPrivate->mConditionDescriptorList )
      {
        if(
            ( d->mType == Condition::ConditionTypePropertyTest ) &&
            ( d->mFunctionDescriptor == static_cast< Condition::PropertyTest * >( condition )->function() )
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
    case Condition::ConditionTypeAnd:
    case Condition::ConditionTypeOr:
    {
      const QList< Condition::Base * > * childConditions = static_cast< Condition::Multi * >( condition )->childConditions();
      Q_ASSERT( childConditions );

      int expectedEditorCount = childConditions->count() + 1;

      ConditionSelector * child;

      while( mPrivate->mChildConditionSelectorList.count() > expectedEditorCount )
      {
        child = mPrivate->mChildConditionSelectorList.last();
        Q_ASSERT( child );
        delete child;
        mPrivate->mChildConditionSelectorList.removeLast();
      }
      
      while( mPrivate->mChildConditionSelectorList.count() < expectedEditorCount )
      {
        child = new ConditionSelector( mPrivate->mChildConditionListBase, mFactory, this );
        mPrivate->mChildConditionSelectorList.append( child );
        mPrivate->mChildConditionListLayout->addWidget( child );
      }

      QList< ConditionSelector * >::Iterator editorListIterator = mPrivate->mChildConditionSelectorList.begin();
      QList< Condition::Base * >::ConstIterator conditionIterator = childConditions->begin();
      while( conditionIterator != childConditions->end() )
      {
        ( *editorListIterator )->fillFromCondition( *conditionIterator );
        ++conditionIterator;
        ++editorListIterator;
      }

      Q_ASSERT( editorListIterator != mPrivate->mChildConditionSelectorList.end() ); // there must be exactly another one
      ( *editorListIterator )->reset();

      ++editorListIterator;
      Q_ASSERT( editorListIterator == mPrivate->mChildConditionSelectorList.end() );

    }
    break;
    case Condition::ConditionTypeNot:
    {
      ConditionSelector * child;
      if( mPrivate->mChildConditionSelectorList.count() != 1 )
      {
        if( mPrivate->mChildConditionSelectorList.count() == 0 )
        {
          child = new ConditionSelector( mPrivate->mChildConditionListBase, mFactory, this );
          mPrivate->mChildConditionSelectorList.append( child );
          mPrivate->mChildConditionListLayout->addWidget( child );
        } else {
          while( mPrivate->mChildConditionSelectorList.count() != 1 )
          {
            child = mPrivate->mChildConditionSelectorList.last();
            Q_ASSERT( child );
            delete child;
            mPrivate->mChildConditionSelectorList.removeLast();
          }
        }
      }
      child = mPrivate->mChildConditionSelectorList.last();
      Q_ASSERT( child );
      child->fillFromCondition( static_cast< Condition::Not * >( condition )->childCondition() );
    }
    break;
    case Condition::ConditionTypeTrue:
    case Condition::ConditionTypeFalse:
      qDeleteAll( mPrivate->mChildConditionSelectorList );
      mPrivate->mChildConditionSelectorList.clear();
    break;
    case Condition::ConditionTypePropertyTest:
    {
      qDeleteAll( mPrivate->mChildConditionSelectorList );
      mPrivate->mChildConditionSelectorList.clear();
      fillPropertyTestControls( descriptor );
    }
    break;
    default:
      Q_ASSERT( false ); //unhandled condition type
    break;
  }

  setupUIForActiveType();

}

bool ConditionSelector::commitToCondition( Condition::Base * condition )
{
  return false;
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi
