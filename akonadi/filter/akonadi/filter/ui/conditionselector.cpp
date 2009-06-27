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

#include <akonadi/filter/ui/conditionselector.h>

#include <akonadi/filter/condition.h>
#include <akonadi/filter/componentfactory.h>
#include <akonadi/filter/functiondescriptor.h>

#include <akonadi/filter/ui/coolcombobox.h>
#include <akonadi/filter/ui/extensionlabel.h>
#include <akonadi/filter/ui/editorfactory.h>
#include <akonadi/filter/ui/valueeditor.h>

#include <KLocale>

#include <QtGui/QLayout>
#include <QtCore/QList>

static int gSpacing = -1; // 0 would be nice for several reasons, but it looks confusing in windows and cde styles... -1 means default
static int gIndent = 20;
static const qreal gSemiTransparentWidgetsOpacity = 0.32;

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
  const DataMemberDescriptor * mDataMemberDescriptor;
};

class ConditionSelectorPrivate
{
public:
  QList< ConditionDescriptor * > mConditionDescriptorList; // FIXME: This could be shared between all the editors with the same componentfactory
  Private::CoolComboBox * mTypeComboBox;
  Private::ExtensionLabel * mExtensionLabel;
  KComboBox * mOperatorDescriptorComboBox;
  Private::ValueEditor * mValueEditor;
  QWidget * mChildConditionListBase;
  QWidget * mRightControlsBase;
  QGridLayout * mRightControlsLayout;
  QList< ConditionSelector * > mChildConditionSelectorList;
  QVBoxLayout * mChildConditionListLayout;  
};


ConditionSelector::ConditionSelector(
    QWidget * parent,
    ComponentFactory * componentfactory,
    EditorFactory * editorComponentFactory,
    ConditionSelector * parentConditionSelector
  )
  : QWidget( parent ), mComponentFactory( componentfactory ), mEditorFactory( editorComponentFactory )
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
  d->mDataMemberDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = Condition::ConditionTypeNot;
  d->mText = i18n( "if the condition below is NOT met" );
  d->mColor = Qt::red;
  d->mFunctionDescriptor = 0;
  d->mDataMemberDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = Condition::ConditionTypeAnd;
  d->mText = i18n( "if ALL of the conditions below are met" );
  d->mColor = Qt::blue;
  d->mFunctionDescriptor = 0;
  d->mDataMemberDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = Condition::ConditionTypeOr;
  d->mText = i18n( "if ANY of the conditions below is met" );
  d->mColor = Qt::darkGreen;
  d->mFunctionDescriptor = 0;
  d->mDataMemberDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  const QList< const FunctionDescriptor * > * props = mComponentFactory->enumerateFunctions();
  Q_ASSERT( props );

  foreach( const FunctionDescriptor *prop, *props )
  {
    QList< const DataMemberDescriptor * > dataMembers = mComponentFactory->enumerateDataMembers( prop->acceptableInputDataTypeMask() );

    foreach( const DataMemberDescriptor * dm, dataMembers )
    {
      d = new ConditionDescriptor;
      d->mType = Condition::ConditionTypePropertyTest;
      d->mText = prop->name();
      d->mText += QChar(' ');
      d->mText += dm->name();
      d->mColor = QColor(); // no overlay
      d->mFunctionDescriptor = prop;
      d->mDataMemberDescriptor = dm;
      mPrivate->mConditionDescriptorList.append( d );
    }
  }

  d = new ConditionDescriptor;
  d->mType = Condition::ConditionTypeTrue;
  d->mText = i18n( "if true (so always)" );
  d->mColor = QColor( 0, 60, 0 );
  d->mFunctionDescriptor = 0;
  d->mDataMemberDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mType = Condition::ConditionTypeFalse;
  d->mText = i18n( "if false (so never)" );
  d->mColor = QColor( 60, 0, 0 );
  d->mFunctionDescriptor = 0;
  d->mDataMemberDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  int idx = 0;

  QColor clrText = palette().color( QPalette::Text );

  foreach( d, mPrivate->mConditionDescriptorList )
  {
    mPrivate->mTypeComboBox->addItem( d->mText, QVariant( idx ) );
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
  mPrivate->mRightControlsBase->setFixedWidth( 400 );
  g->addWidget( mPrivate->mRightControlsBase, 0, 2, 1, 1 );

  mPrivate->mRightControlsLayout = new QGridLayout( mPrivate->mRightControlsBase );
  mPrivate->mRightControlsLayout->setMargin( 0 );
  if( gSpacing != -1 )
    mPrivate->mRightControlsLayout->setSpacing( gSpacing );


  mPrivate->mOperatorDescriptorComboBox = new KComboBox( false, mPrivate->mRightControlsBase );
  mPrivate->mOperatorDescriptorComboBox->hide();
  mPrivate->mOperatorDescriptorComboBox->setFixedWidth( 200 );
  mPrivate->mRightControlsLayout->addWidget( mPrivate->mOperatorDescriptorComboBox, 0, 1 );
  connect( mPrivate->mOperatorDescriptorComboBox, SIGNAL( activated( int ) ), this, SLOT( operatorDescriptorComboBoxActivated( int ) ) );

  mPrivate->mValueEditor = Private::ValueEditor::editorForDataType( DataTypeString, mPrivate->mRightControlsBase );
  Q_ASSERT( mPrivate->mValueEditor );
  mPrivate->mValueEditor->widget()->hide();
  mPrivate->mRightControlsLayout->addWidget( mPrivate->mValueEditor->widget(), 0, 2 );

  mPrivate->mExtensionLabel = new Private::ExtensionLabel( this );
  g->addWidget( mPrivate->mExtensionLabel, 1, 0, 1, 1 );

  mPrivate->mChildConditionListBase = new QWidget( this );
  mPrivate->mChildConditionListBase->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

  mPrivate->mChildConditionListLayout = new QVBoxLayout( mPrivate->mChildConditionListBase );
  mPrivate->mChildConditionListLayout->setMargin( 0 );
  if( gSpacing != -1 )
    mPrivate->mChildConditionListLayout->setSpacing( gSpacing );

  g->addWidget( mPrivate->mChildConditionListBase, 1, 1, 1, 2 );

  mPrivate->mChildConditionListBase->hide();

  g->setColumnMinimumWidth( 0, gIndent );
  g->setColumnStretch( 1, 1 );
}

ConditionSelector::~ConditionSelector()
{
  delete mPrivate->mValueEditor; // delete this as it might not be a wieget
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
      mPrivate->mRightControlsBase->hide();
      mPrivate->mTypeComboBox->setOpacity( gSemiTransparentWidgetsOpacity );
      mPrivate->mOperatorDescriptorComboBox->hide();
      mPrivate->mValueEditor->widget()->hide();
      mPrivate->mExtensionLabel->hide();
      mPrivate->mChildConditionListBase->hide();
    break;
    case Condition::ConditionTypeTrue:
    case Condition::ConditionTypeFalse:
      mPrivate->mRightControlsBase->hide();
      mPrivate->mTypeComboBox->setOpacity( 1.0 );
      mPrivate->mOperatorDescriptorComboBox->hide();
      mPrivate->mValueEditor->widget()->hide();
      mPrivate->mExtensionLabel->hide();
      mPrivate->mChildConditionListBase->hide();
    break;
    case Condition::ConditionTypePropertyTest:
      mPrivate->mRightControlsBase->show();
      mPrivate->mTypeComboBox->setOpacity( 1.0 );
      mPrivate->mOperatorDescriptorComboBox->show();
      mPrivate->mValueEditor->widget()->show();
      mPrivate->mChildConditionListBase->hide();
      mPrivate->mExtensionLabel->hide();
    break;
    case Condition::ConditionTypeNot:
    case Condition::ConditionTypeAnd:
    case Condition::ConditionTypeOr:
      mPrivate->mRightControlsBase->hide();
      mPrivate->mTypeComboBox->setOpacity( 1.0 );
      mPrivate->mExtensionLabel->setFixedHeight( ( d->mType == Condition::ConditionTypeNot ) ? ( mPrivate->mTypeComboBox->sizeHint().height() - 5 ) : -1 );
      mPrivate->mExtensionLabel->setOverlayColor( d->mColor );
      mPrivate->mOperatorDescriptorComboBox->hide();
      mPrivate->mValueEditor->widget()->hide();

      if( mPrivate->mChildConditionSelectorList.count() == 0 )
      {
        ConditionSelector * child = new ConditionSelector( mPrivate->mChildConditionListBase, mComponentFactory, mEditorFactory, this );
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
  ConditionDescriptor * d = conditionDescriptorForActiveType();

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
      child = new ConditionSelector( mPrivate->mChildConditionListBase, mComponentFactory, mEditorFactory, this );
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

  ConditionDescriptor * d = conditionDescriptorForActiveType();
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

Condition::ConditionType ConditionSelector::currentConditionType()
{
  ConditionDescriptor * d = conditionDescriptorForActiveType();
  Q_ASSERT( d );
  return d->mType;
}


ConditionDescriptor * ConditionSelector::conditionDescriptorForActiveType()
{
  int idx = mPrivate->mTypeComboBox->currentIndex();
  if( idx < 0 )
    idx = 0;
  Q_ASSERT( idx < mPrivate->mConditionDescriptorList.count() );
  return mPrivate->mConditionDescriptorList.at( idx );
}


bool ConditionSelector::isEmpty()
{
  ConditionDescriptor * d = conditionDescriptorForActiveType();
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
  mPrivate->mOperatorDescriptorComboBox->clear();

  QList< const OperatorDescriptor * > operators = mComponentFactory->enumerateOperators( descriptor->mFunctionDescriptor->outputDataTypeMask() );
  if( operators.isEmpty() )
  {
    // doesn't need an operator
    mPrivate->mOperatorDescriptorComboBox->hide();
    mPrivate->mValueEditor->widget()->hide();
    return;
  }
  foreach( const OperatorDescriptor * op, operators )
  {
    mPrivate->mOperatorDescriptorComboBox->addItem( op->name(), QVariant( (qlonglong)op ) );
  }
  operatorDescriptorComboBoxActivated( -1 );
}

void ConditionSelector::operatorDescriptorComboBoxActivated( int )
{
  const OperatorDescriptor * op = operatorDescriptorForActiveType();

  if( !op )
  {
    mPrivate->mValueEditor->widget()->hide();
    return;
  }

  switch( op->rightOperandDataType() )
  {
    case DataTypeNone:
      mPrivate->mValueEditor->widget()->hide();
      return;
    break;
    case DataTypeString:
    case DataTypeInteger:
    case DataTypeDate:
      // ok
    break;
    default:
      Q_ASSERT_X( false, __FUNCTION__, "Unhandled operator data type" );
      mPrivate->mValueEditor->widget()->hide();
      return;
    break;
  }

  if( mPrivate->mValueEditor->dataType() != op->rightOperandDataType() )
  {
    delete mPrivate->mValueEditor;
    mPrivate->mValueEditor = Private::ValueEditor::editorForDataType( op->rightOperandDataType(), mPrivate->mRightControlsBase );
    Q_ASSERT( mPrivate->mValueEditor );
    mPrivate->mRightControlsLayout->addWidget( mPrivate->mValueEditor->widget(), 0, 2 );
  }
  mPrivate->mValueEditor->widget()->show();
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
        child = new ConditionSelector( mPrivate->mChildConditionListBase, mComponentFactory, mEditorFactory, this );
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
          child = new ConditionSelector( mPrivate->mChildConditionListBase, mComponentFactory, mEditorFactory, this );
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
      int cnt = mPrivate->mOperatorDescriptorComboBox->count();
      for( int idx = 0; idx < cnt; idx++ )
      {
        QVariant v = mPrivate->mOperatorDescriptorComboBox->itemData( idx );
        if( v.isNull() )
          continue;

        bool ok;
        qlonglong ptr = v.toLongLong( &ok );
        if( !ok )
          continue;
        const OperatorDescriptor * op = reinterpret_cast< const OperatorDescriptor * >( ptr );
        if( op == static_cast< Condition::PropertyTest * >( condition )->functionOperatorDescriptor() )
        {
          mPrivate->mOperatorDescriptorComboBox->setCurrentIndex( idx );
          operatorDescriptorComboBoxActivated( idx );
          mPrivate->mValueEditor->setValue( static_cast< Condition::PropertyTest * >( condition )->operand() );
          break;
        }
      }
    }
    break;
    default:
      Q_ASSERT( false ); //unhandled condition type
    break;
  }

  setupUIForActiveType();

}

const OperatorDescriptor * ConditionSelector::operatorDescriptorForActiveType()
{
  int idx = mPrivate->mOperatorDescriptorComboBox->currentIndex();
  if( idx < 0 )
    return 0;
  QVariant v = mPrivate->mOperatorDescriptorComboBox->itemData( idx );
  if( v.isNull() )
    return 0;

  bool ok;
  qlonglong ptr = v.toLongLong( &ok );

  if( !ok )
    return 0;

  return reinterpret_cast< const OperatorDescriptor * >( ptr );
}


Condition::Base * ConditionSelector::commitState( Component * parent )
{
  ConditionDescriptor * d = conditionDescriptorForActiveType();
  Q_ASSERT( d );

  switch( d->mType )
  {
    case Condition::ConditionTypeUnknown:
      // empty condition (always true)
      return mComponentFactory->createTrueCondition( parent );
    break;
    case Condition::ConditionTypeFalse:
      return mComponentFactory->createFalseCondition( parent );
    break;
    case Condition::ConditionTypeTrue:
      return mComponentFactory->createTrueCondition( parent );
    break;
    case Condition::ConditionTypeAnd:
    case Condition::ConditionTypeOr:
    {
      Condition::Multi * multiCondition = ( d->mType == Condition::ConditionTypeAnd ) ? \
                                             static_cast< Condition::Multi * >( mComponentFactory->createAndCondition( parent ) ) : \
                                             static_cast< Condition::Multi * >( mComponentFactory->createOrCondition( parent ) );
      Q_ASSERT( multiCondition );

      foreach( ConditionSelector * sel, mPrivate->mChildConditionSelectorList )
      {
        if( sel->currentConditionType() == Condition::ConditionTypeUnknown )
          continue;

        Condition::Base * child = sel->commitState( multiCondition );
        if( !child )
        {
          delete multiCondition;
          return 0;
        }
        multiCondition->addChildCondition( child );
      }

      return multiCondition;     
    }
    break;
    case Condition::ConditionTypeNot:
    {
      Q_ASSERT( mPrivate->mChildConditionSelectorList.count() == 1 );
      ConditionSelector * ed = mPrivate->mChildConditionSelectorList.first();
      Q_ASSERT( ed );

      if( ed->currentConditionType() == Condition::ConditionTypeUnknown )
        return mComponentFactory->createFalseCondition( parent );

      Condition::Not * notCondition = mComponentFactory->createNotCondition( parent );
      Q_ASSERT( notCondition );

      Condition::Base * child = ed->commitState( notCondition );
      if( !child )
      {
        delete notCondition;
        return 0;
      }
      notCondition->setChildCondition( child );
      return notCondition;
    }
    break;
    case Condition::ConditionTypePropertyTest:
    {
      Q_ASSERT( d->mFunctionDescriptor );
      Q_ASSERT( d->mDataMemberDescriptor );

      QVariant val = mPrivate->mValueEditor->value();
      if( !val.isValid() )
        return 0; // message already shown

      const OperatorDescriptor * op = operatorDescriptorForActiveType();
      Q_ASSERT( op );

      return mComponentFactory->createPropertyTestCondition(
          parent,
          d->mFunctionDescriptor,
          d->mDataMemberDescriptor,
          op,
          val
        );
    }
    break;
    default:
      Q_ASSERT( false ); //unhandled condition type
    break;
  }

  Q_ASSERT( false );
  return 0;
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi
