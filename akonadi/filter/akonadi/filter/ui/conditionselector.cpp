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

#include "../condition.h"
#include "../componentfactory.h"
#include "../functiondescriptor.h"

#include "coolcombobox.h"
#include "extensionlabel.h"
#include "editorfactory.h"
#include "valueeditor.h"
#include "ruleeditor.h"
#include "widgethighlighter.h"

#include <KLocale>
#include <KMessageBox>
#include <KDebug>

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
  bool mIsSeparator;
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
    RuleEditor * ruleEditor,
    ConditionSelector * parentConditionSelector,
    bool isFirst
  )
  : QWidget( parent ), mComponentFactory( componentfactory ), mEditorFactory( editorComponentFactory ), mRuleEditor( ruleEditor )
{
  mParentConditionSelector = parentConditionSelector;

  mPrivate = new ConditionSelectorPrivate;

  QGridLayout * g = new QGridLayout( this );
  g->setMargin( 0 );
  if( gSpacing != -1 )
    g->setSpacing( gSpacing );

  mPrivate->mTypeComboBox = new Private::CoolComboBox( false, this );
  mPrivate->mTypeComboBox->setOpacity( gSemiTransparentWidgetsOpacity );
  mPrivate->mTypeComboBox->setMaxVisibleItems( 20 );
  connect( mPrivate->mTypeComboBox, SIGNAL( activated( int ) ), this, SLOT( typeComboBoxActivated( int ) ) );

  g->addWidget( mPrivate->mTypeComboBox, 0, 0, 1, 2 );

  ConditionDescriptor * d;

  d = new ConditionDescriptor;
  d->mIsSeparator = false;
  d->mType = Condition::ConditionTypeUnknown;
  d->mText = i18n( "<...select to activate condition...>" );
  d->mColor = palette().color( QPalette::Button );
  d->mFunctionDescriptor = 0;
  d->mDataMemberDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mIsSeparator = true;
  d->mType = Condition::ConditionTypeUnknown;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mIsSeparator = false;
  d->mType = Condition::ConditionTypeNot;
  d->mText = i18n( "the condition below is NOT met" );
  d->mColor = Qt::red;
  d->mFunctionDescriptor = 0;
  d->mDataMemberDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mIsSeparator = false;
  d->mType = Condition::ConditionTypeAnd;
  d->mText = i18n( "ALL of the conditions below are met" );
  d->mColor = Qt::blue;
  d->mFunctionDescriptor = 0;
  d->mDataMemberDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mIsSeparator = false;
  d->mType = Condition::ConditionTypeOr;
  d->mText = i18n( "ANY of the conditions below are met" );
  d->mColor = Qt::darkGreen;
  d->mFunctionDescriptor = 0;
  d->mDataMemberDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mIsSeparator = true;
  d->mType = Condition::ConditionTypeUnknown;
  mPrivate->mConditionDescriptorList.append( d );

  QList< const DataMemberDescriptor * > dataMembers = mComponentFactory->enumerateDataMembers();
  const DataMemberDescriptor * dm;

  foreach( dm, dataMembers )
  {
    d = new ConditionDescriptor;
    d->mIsSeparator = false;
    d->mType = Condition::ConditionTypePropertyTest;
    d->mText = dm->name();
    d->mColor = QColor(); // no overlay
    d->mFunctionDescriptor = 0;
    d->mDataMemberDescriptor = dm;
    mPrivate->mConditionDescriptorList.append( d );
  }

  const QList< const FunctionDescriptor * > * props = mComponentFactory->enumerateFunctions();
  Q_ASSERT( props );

  foreach( const FunctionDescriptor *prop, *props )
  {
    d = new ConditionDescriptor;
    d->mIsSeparator = true;
    d->mType = Condition::ConditionTypeUnknown;
    mPrivate->mConditionDescriptorList.append( d );

    dataMembers = mComponentFactory->enumerateDataMembers( prop->acceptableInputDataTypeMask(), prop->requiredInputFeatureMask() );

    foreach( dm, dataMembers )
    {
      d = new ConditionDescriptor;
      d->mIsSeparator = false;
      d->mType = Condition::ConditionTypePropertyTest;
      d->mText = prop->name();
      if( !d->mText.isEmpty() )
        d->mText += QLatin1Char(' ');
      d->mText += dm->name();
      d->mColor = QColor(); // no overlay
      d->mFunctionDescriptor = prop;
      d->mDataMemberDescriptor = dm;
      mPrivate->mConditionDescriptorList.append( d );
    }
  }

  d = new ConditionDescriptor;
  d->mIsSeparator = true;
  d->mType = Condition::ConditionTypeUnknown;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mIsSeparator = false;
  d->mType = Condition::ConditionTypeTrue;
  d->mText = i18n( "true (so always)" );
  d->mColor = QColor( 0, 60, 0 );
  d->mFunctionDescriptor = 0;
  d->mDataMemberDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  d = new ConditionDescriptor;
  d->mIsSeparator = false;
  d->mType = Condition::ConditionTypeFalse;
  d->mText = i18n( "false (so never)" );
  d->mColor = QColor( 60, 0, 0 );
  d->mFunctionDescriptor = 0;
  d->mDataMemberDescriptor = 0;
  mPrivate->mConditionDescriptorList.append( d );

  int idx = 0;

  QColor clrText = palette().color( QPalette::Text );

  
  foreach( d, mPrivate->mConditionDescriptorList )
  {
    if( d->mIsSeparator )
    {
      mPrivate->mTypeComboBox->insertSeparator( mPrivate->mTypeComboBox->count() );
      idx++;
      continue;
    }

    if( mParentConditionSelector )
    {
      if( isFirst )
      {
        mPrivate->mTypeComboBox->addItem( d->mText, QVariant( idx ) );
      } else {
        ConditionDescriptor * parentCondition = mParentConditionSelector->conditionDescriptorForActiveType();
        switch( parentCondition->mType )
        {
          case Condition::ConditionTypeAnd:
            mPrivate->mTypeComboBox->addItem( i18n( "and %1", d->mText ), QVariant( idx ) );
          break;
          case Condition::ConditionTypeOr:
            mPrivate->mTypeComboBox->addItem( i18n( "or %1", d->mText ), QVariant( idx ) );
          break;
          default:
            // hum... should be NOT, right ?
            Q_ASSERT( parentCondition->mType == Condition::ConditionTypeNot );
            mPrivate->mTypeComboBox->addItem( d->mText, QVariant( idx ) );
          break;
        }
      }
    } else {
      if( d->mText.isEmpty() )
        mPrivate->mTypeComboBox->addItem( i18n( "if" ), QVariant( idx ) );
      else
        mPrivate->mTypeComboBox->addItem( i18n( "if %1", d->mText ), QVariant( idx ) );
    }

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
  mPrivate->mRightControlsLayout->addWidget( mPrivate->mOperatorDescriptorComboBox, 0, 1, Qt::AlignLeft | Qt::AlignVCenter );
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
  Q_ASSERT( !d->mIsSeparator );

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
     // Don't touch operator and value editor (managed by fillPropertyTestControls())
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
        ConditionSelector * child = new ConditionSelector( mPrivate->mChildConditionListBase, mComponentFactory, mEditorFactory, mRuleEditor, this, true );
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

QString ConditionSelector::conditionDescription()
{
  ConditionDescriptor * d = conditionDescriptorForActiveType();

  switch( d->mType )
  {
    case Condition::ConditionTypeAnd:
      return i18n( "multiple conditions apply" );
    break;
    case Condition::ConditionTypeOr:
      return i18n( "some conditions apply" );
    break;
    case Condition::ConditionTypeNot:
    {
      QString childCondition;
      if( mPrivate->mChildConditionSelectorList.count() > 0 )
      {
        ConditionSelector * sel = mPrivate->mChildConditionSelectorList.first();
        Q_ASSERT( sel );

        childCondition = sel->conditionDescription();
      }
      if( childCondition.isEmpty() )
        childCondition = QString::fromAscii( "???" );

      return i18n( "not ( %1 )", childCondition );
    }
    break;
    case Condition::ConditionTypeTrue:
      return QString(); // like an empty condition
    break;
    case Condition::ConditionTypeFalse:
      return i18n( "false" );
    break;
    case Condition::ConditionTypePropertyTest:
    {
      const OperatorDescriptor * op = operatorDescriptorForActiveType();

      if( !op )
        return d->mText;

      switch( op->rightOperandDataType() )
      {
        case DataTypeNone:
          return QString::fromAscii( "%1 %2" ).arg( d->mText ).arg( op->name() );
        break;
        case DataTypeInteger:
          return QString::fromAscii( "%1 %2 %3" ).arg( d->mText ).arg( op->name() ).arg( mPrivate->mValueEditor->value( false ).toString() );
        break;
        case DataTypeString:
        case DataTypeDate:
        {
          QString val = mPrivate->mValueEditor->value( false ).toString();
          if( val.length() > 20 )
            val = val.left( 20 ) + QString::fromAscii( "..." );
          return QString::fromAscii( "%1 %2 \"%3\"" ).arg( d->mText ).arg( op->name() ).arg( val );
        }
        break;
        default:
          Q_ASSERT_X( false, __FUNCTION__, "Unhandled operator data type" );
        break;
      }

      return d->mText;
    }
    break;
    case Condition::ConditionTypeUnknown:
      return QString(); // empty condition
    break;
    default:
    break;
  }

  return QString::fromAscii( "???" );
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
      child = new ConditionSelector( mPrivate->mChildConditionListBase, mComponentFactory, mEditorFactory, mRuleEditor, this, false );
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

  mRuleEditor->childConditionSelectorContentsChanged();
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
  ConditionDescriptor * d = mPrivate->mConditionDescriptorList.at( idx );
  Q_ASSERT( d );
  Q_ASSERT( !d->mIsSeparator );
  return d;
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
 
  QList< const OperatorDescriptor * > operators = mComponentFactory->enumerateOperators(
      descriptor->mFunctionDescriptor ?
        descriptor->mFunctionDescriptor->outputDataType() : descriptor->mDataMemberDescriptor->dataType(),
      descriptor->mFunctionDescriptor ? 
        descriptor->mFunctionDescriptor->outputFeatureMask() : descriptor->mDataMemberDescriptor->featureMask()
    );

  if( operators.isEmpty() )
  {
    // no operator for this kind of property test
    if( descriptor->mFunctionDescriptor )
    {
      Q_ASSERT( descriptor->mFunctionDescriptor->outputDataType() == DataTypeBoolean );
    } else {
      Q_ASSERT( descriptor->mDataMemberDescriptor->dataType() == DataTypeBoolean );
    }

    mPrivate->mOperatorDescriptorComboBox->hide();
    mPrivate->mValueEditor->widget()->hide();
    return;
  }

  mPrivate->mOperatorDescriptorComboBox->show();

  foreach( const OperatorDescriptor * op, operators )
    mPrivate->mOperatorDescriptorComboBox->addItem( op->name(), QVariant( (qlonglong)op ) );

  operatorDescriptorComboBoxActivated( -1 );
}

void ConditionSelector::operatorDescriptorComboBoxActivated( int )
{
  const OperatorDescriptor * op = operatorDescriptorForActiveType();

  if( !op )
  {
    mPrivate->mValueEditor->widget()->hide();
    mRuleEditor->childConditionSelectorContentsChanged();
    return;
  }

  switch( op->rightOperandDataType() )
  {
    case DataTypeNone:
      mPrivate->mValueEditor->widget()->hide();
      mRuleEditor->childConditionSelectorContentsChanged();
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

  mRuleEditor->childConditionSelectorContentsChanged();
}

void ConditionSelector::fillFromCondition( Condition::Base * condition )
{
  ConditionDescriptor * descriptor = 0;
  int idx = 0;

  Condition::ConditionType type = condition ? condition->conditionType() : Condition::ConditionTypeUnknown;

  switch( type )
  {
    case Condition::ConditionTypeAnd:
    case Condition::ConditionTypeOr:
    case Condition::ConditionTypeNot:
    case Condition::ConditionTypeFalse:
    case Condition::ConditionTypeTrue:
    case Condition::ConditionTypeUnknown:
      foreach( ConditionDescriptor * d, mPrivate->mConditionDescriptorList )
      {
        if( d->mType == type )
        {
          descriptor = d;
          break;
        }
        idx++;
      }
    break;
    case Condition::ConditionTypePropertyTest:
      Q_ASSERT( condition );

      foreach( ConditionDescriptor * d, mPrivate->mConditionDescriptorList )
      {
        if(
            ( !d->mIsSeparator ) && 
            ( d->mType == Condition::ConditionTypePropertyTest ) &&
            ( d->mFunctionDescriptor == static_cast< Condition::PropertyTest * >( condition )->function() ) &&
            ( d->mDataMemberDescriptor == static_cast< Condition::PropertyTest * >( condition )->dataMember() )
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

  Q_ASSERT( descriptor );
  Q_ASSERT( idx >= 0 ); // same as above

  mPrivate->mTypeComboBox->setCurrentIndex( idx );

  switch( type )
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
        child = new ConditionSelector(
            mPrivate->mChildConditionListBase,
            mComponentFactory,
            mEditorFactory,
            mRuleEditor,
            this,
            mPrivate->mChildConditionSelectorList.count() == 0
          );
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
          child = new ConditionSelector( mPrivate->mChildConditionListBase, mComponentFactory, mEditorFactory, mRuleEditor, this, true );
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
    case Condition::ConditionTypeUnknown:
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
        if( op == static_cast< Condition::PropertyTest * >( condition )->operatorDescriptor() )
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

      if( multiCondition->childConditions()->count() < 1 )
      {
        KMessageBox::sorry(
            this,
            i18n(
                "The '%1' condition must have at least one valid child condition",
                ( d->mType == Condition::ConditionTypeAnd ) ? QLatin1String( "all of" ) : QLatin1String( "any of" )
              ),
            i18n( "Invalid condition" )
          );
        new Private::WidgetHighlighter( this );
        return 0;
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
      {
        KMessageBox::sorry( this, i18n( "The 'not' condition must have a valid child condition" ), i18n( "Invalid condition" ) );
        new Private::WidgetHighlighter( this );
        return 0;
      }

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
      Q_ASSERT( d->mDataMemberDescriptor );

      QVariant val;

      const OperatorDescriptor * op = operatorDescriptorForActiveType();

      if( op && ( op->rightOperandDataType() != DataTypeNone ) )
      {
        val = mPrivate->mValueEditor->value();
        if( !val.isValid() )
          return 0; // message already shown
      }

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
