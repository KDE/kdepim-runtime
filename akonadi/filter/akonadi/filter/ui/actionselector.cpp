/****************************************************************************** * *
 *
 *  File : actionselector.cpp
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

#include "actionselector.h"

#include "../action.h"
#include "../componentfactory.h"
#include "../commanddescriptor.h"

#include "coolcombobox.h"
#include "editorfactory.h"
#include "extensionlabel.h"
#include "actioneditor.h"
#include "commandeditor.h"
#include "rulelisteditor.h"
#include "ruleeditor.h"

#include <QtGui/QLayout>
#include <QtCore/QString>
#include <QtGui/QColor>

#include <KLocale>
#include <KMessageBox>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

static int gSpacing = -1; // 0 would be nice for several reasons, but it looks confusing in windows and cde styles... -1 means default
static int gIndent = 20;
static const qreal gSemiTransparentWidgetsOpacity = 0.5;

class ActionDescriptor
{
public:
  Action::ActionType mType;
  QString mText;
  QColor mColor;
  const CommandDescriptor * mCommandDescriptor;
};

class ActionSelectorPrivate
{
public:
  Private::CoolComboBox * mTypeComboBox;
  Private::ExtensionLabel * mExtensionLabel;
  QList< ActionDescriptor * > mActionDescriptorList; // FIXME: This could be shared between all the editors with the same componentfactory
  ActionEditor * mActionEditor;
  QGridLayout * mLayout;
};

ActionSelector::ActionSelector(
    QWidget * parent,
    ComponentFactory * componentfactory,
    EditorFactory * editorComponentFactory,
    RuleEditor * ruleEditor,
    bool isFirst
  )
  : QWidget( parent ), mComponentFactory( componentfactory ), mEditorFactory( editorComponentFactory ), mRuleEditor( ruleEditor )
{
  mPrivate = new ActionSelectorPrivate;

  mPrivate->mActionEditor = 0;
  mPrivate->mExtensionLabel = 0;

  mPrivate->mLayout = new QGridLayout( this );
  mPrivate->mLayout->setMargin( 1 );

  mPrivate->mTypeComboBox = new Private::CoolComboBox( false, this );
  mPrivate->mTypeComboBox->setOverlayColor( Qt::yellow );
  mPrivate->mTypeComboBox->setOverlayOpacity( 0.2 );
  connect( mPrivate->mTypeComboBox, SIGNAL( activated( int ) ), this, SLOT( typeComboBoxActivated( int ) ) );

  mPrivate->mLayout->addWidget( mPrivate->mTypeComboBox, 0, 0, 1, 2 );


  ActionDescriptor * d;

  d = new ActionDescriptor;
  d->mType = Action::ActionTypeUnknown;
  d->mText = i18n( "continue processing" );
  d->mColor = QColor( 127, 127, 0 );
  d->mCommandDescriptor = 0;
  mPrivate->mActionDescriptorList.append( d );

  d = new ActionDescriptor;
  d->mType = Action::ActionTypeStop;
  QString defaultActionDescription = mEditorFactory->defaultActionDescription();
  if( defaultActionDescription.isEmpty() )
    d->mText = i18n( "stop processing here" );
  else
    d->mText = i18n( "%1 and stop processing here", defaultActionDescription );
  d->mColor = QColor( 150, 100, 0 );
  d->mCommandDescriptor = 0;
  mPrivate->mActionDescriptorList.append( d );

  d = new ActionDescriptor;
  d->mType = Action::ActionTypeRuleList;
  d->mText = i18n( "execute the following sub-filter" );
  d->mColor = QColor( 150, 130, 0 );
  d->mCommandDescriptor = 0;
  mPrivate->mActionDescriptorList.append( d );

  const QList< const CommandDescriptor * > * props = mComponentFactory->enumerateCommands();
  Q_ASSERT( props );

  foreach( const CommandDescriptor *prop, *props )
  {
    d = new ActionDescriptor;
    d->mType = Action::ActionTypeCommand;
    d->mText = prop->name();
    d->mColor = QColor( 150, 150, 0 );
    d->mCommandDescriptor = prop;
    mPrivate->mActionDescriptorList.append( d );
  }

  int idx = 0;

  QColor clrText = palette().color( QPalette::Text );

  foreach( d, mPrivate->mActionDescriptorList )
  {
    if( isFirst )
      mPrivate->mTypeComboBox->addItem( i18n( "then %1", d->mText ), idx );
    else
      mPrivate->mTypeComboBox->addItem( i18n( "and %1", d->mText ), idx );

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

  mPrivate->mLayout->setColumnMinimumWidth( 0, gIndent );
  mPrivate->mLayout->setColumnStretch( 1, 1 );

  setupUIForActiveType();
}

ActionSelector::~ActionSelector()
{
  delete mPrivate;
}

void ActionSelector::fillFromAction( Action::Base * action )
{
  ActionDescriptor * descriptor = 0;
  int idx = 0;

  switch( action->actionType() )
  {
    case Action::ActionTypeRuleList:
    case Action::ActionTypeStop:
    case Action::ActionTypeUnknown:
      foreach( ActionDescriptor * d, mPrivate->mActionDescriptorList )
      {
        if( d->mType == action->actionType() )
        {
          descriptor = d;
          break;
        }
        idx++;
      }
    break;
    case Action::ActionTypeCommand:
      foreach( ActionDescriptor * d, mPrivate->mActionDescriptorList )
      {
        if(
            ( d->mType == Action::ActionTypeCommand ) &&
            ( d->mCommandDescriptor == static_cast< Action::Command * >( action )->commandDescriptor() )
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

  setupUIForActiveType();

  if( mPrivate->mActionEditor )
    mPrivate->mActionEditor->fillFromAction( action );
}

Action::Base * ActionSelector::commitState( Component * parent )
{
  ActionDescriptor * d = descriptorForActiveType();
  Q_ASSERT( d );

  switch( d->mType )
  {
    case Action::ActionTypeUnknown:
      KMessageBox::sorry( this, i18n( "You must select a valid action type" ), i18n( "Invalid condition selection" ) );
      mPrivate->mTypeComboBox->setOverlayColor( Qt::red );
      return 0;
    break;
    case Action::ActionTypeRuleList:
      Q_ASSERT( mPrivate->mActionEditor );
      Q_ASSERT( mPrivate->mActionEditor->inherits("Akonadi::Filter::UI::RuleListEditor") );
      return mPrivate->mActionEditor->commitState( parent );
    break;
    case Action::ActionTypeStop:
      return mComponentFactory->createStopAction( parent );
    break;
    case Action::ActionTypeCommand:
      if( mPrivate->mActionEditor )
        return mPrivate->mActionEditor->commitState( parent );
      return mComponentFactory->createCommand( parent, d->mCommandDescriptor, QList< QVariant >() );
    break;
    default:
      Q_ASSERT( false ); // unhandled action type
    break;
  } 

  return 0;
}

bool ActionSelector::currentActionIsTerminal()
{
  ActionDescriptor * d = descriptorForActiveType();
  Q_ASSERT( d );
  if( d->mCommandDescriptor )
    return d->mCommandDescriptor->isTerminal();
  return ( d->mType == Action::ActionTypeStop ) || ( d->mType == Action::ActionTypeUnknown );
}

Action::ActionType ActionSelector::currentActionType()
{
  ActionDescriptor * d = descriptorForActiveType();
  Q_ASSERT( d );
  return d->mType; 
}

bool ActionSelector::isEmpty()
{
  ActionDescriptor * d = descriptorForActiveType();
  Q_ASSERT( d );
  return d->mType == Action::ActionTypeUnknown;
}

QString ActionSelector::actionDescription()
{
  if( isEmpty() )
    return i18n( "do nothing" );

  ActionDescriptor * d = descriptorForActiveType();
  Q_ASSERT( d );

  switch( d->mType )
  {
    case Action::ActionTypeUnknown:
      return i18n( "do nothing" );
    break;
    case Action::ActionTypeRuleList:
      return i18n( "execute a subprogram" );
    break;
    case Action::ActionTypeStop:
      return i18n( "stop processing" );
    break;
    case Action::ActionTypeCommand:
      return d->mText;
    break;
    default:
      Q_ASSERT( false ); // unhandled action type
    break;
  }

  return QString::fromAscii( "???" );
}


ActionDescriptor * ActionSelector::descriptorForActiveType()
{
  int idx = mPrivate->mTypeComboBox->currentIndex();
  if( idx < 0 )
    idx = 0;
  Q_ASSERT( idx < mPrivate->mActionDescriptorList.count() );
  return mPrivate->mActionDescriptorList.at( idx );
}

void ActionSelector::setupUIForActiveType()
{
  ActionDescriptor * d = descriptorForActiveType();
  Q_ASSERT( d );

  mPrivate->mTypeComboBox->setOverlayColor( d->mColor );
  mPrivate->mTypeComboBox->setOpacity( d->mType == Action::ActionTypeUnknown ? gSemiTransparentWidgetsOpacity : 1.0 );

  if( mPrivate->mActionEditor )
  {
    delete mPrivate->mActionEditor;
    mPrivate->mActionEditor = 0;
    Q_ASSERT( mPrivate->mExtensionLabel );
    delete mPrivate->mExtensionLabel;
    mPrivate->mExtensionLabel = 0;
  }

  switch( d->mType )
  {
    case Action::ActionTypeUnknown:
    break;
    case Action::ActionTypeRuleList:
      mPrivate->mActionEditor = mEditorFactory->createRuleListEditor( this, mComponentFactory );
    break;
    case Action::ActionTypeStop:
    break;
    case Action::ActionTypeCommand:
      mPrivate->mActionEditor = mEditorFactory->createCommandEditor( this, d->mCommandDescriptor, mComponentFactory );
    break;
    default:
      Q_ASSERT( false ); // unhandled action type
    break;
  }

  if( mPrivate->mActionEditor )
  {
    if ( !mPrivate->mExtensionLabel )
    {
      mPrivate->mExtensionLabel = new Private::ExtensionLabel( this );
      mPrivate->mLayout->addWidget( mPrivate->mExtensionLabel, 1, 0 );
      mPrivate->mExtensionLabel->show();
    }

    mPrivate->mExtensionLabel->setOverlayColor( d->mColor );
    //mPrivate->mExtensionLabel->setFixedHeight( mPrivate->mActionEditor->sizeHint().height() - 5 );

    mPrivate->mLayout->addWidget( mPrivate->mActionEditor, 1, 1 );
    mPrivate->mActionEditor->show();
  }
}

void ActionSelector::typeComboBoxActivated( int )
{
  setupUIForActiveType();

  if( mRuleEditor )
    mRuleEditor->childActionSelectorTypeChanged( this );
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi
