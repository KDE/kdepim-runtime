/*
    This file is part of KAddressBook.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include "phoneeditwidget.h"

#include <QtCore/QSignalMapper>
#include <QtCore/QString>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QScrollArea>
#include <QtGui/QScrollBar>
#include <QtGui/QVBoxLayout>

#include <kabc/phonenumber.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <klineedit.h>
#include <klocale.h>

PhoneTypeCombo::PhoneTypeCombo( QWidget *parent )
  : KComboBox( parent ),
    mType( KABC::PhoneNumber::Home ),
    mLastSelected( 0 )
{
  for ( int i = 0; i < KABC::PhoneNumber::typeList().count(); ++i )
    mTypeList.append( KABC::PhoneNumber::typeList().at( i ) );

  mTypeList.append( -1 ); // Others...

  update();

  connect( this, SIGNAL( activated( int ) ),
           this, SLOT( selected( int ) ) );
}

PhoneTypeCombo::~PhoneTypeCombo()
{
}

void PhoneTypeCombo::setType( KABC::PhoneNumber::Type type )
{
  if ( !mTypeList.contains( type ) )
    mTypeList.insert( mTypeList.at( mTypeList.count() - 1 ), type );

  mType = type;
  update();
}

KABC::PhoneNumber::Type PhoneTypeCombo::type() const
{
  return mType;
}

void PhoneTypeCombo::update()
{
  clear();

  for ( int i = 0; i < mTypeList.count(); ++i ) {
    if ( mTypeList.at( i ) == -1 ) // "Other..." entry
      addItem( i18nc( "@item:inlistbox Category of contact info field", "Other..." ) );
    else
      addItem( KABC::PhoneNumber::typeLabel( KABC::PhoneNumber::Type( mTypeList.at( i ) ) ) );
  }

  setCurrentIndex( mLastSelected = mTypeList.indexOf( mType ) );
}

void PhoneTypeCombo::selected( int pos )
{
  if ( mTypeList.at( pos ) == -1 )
    otherSelected();
  else {
    mType = KABC::PhoneNumber::Type( mTypeList.at( pos ) );
    mLastSelected = pos;
  }
}

void PhoneTypeCombo::otherSelected()
{
  PhoneTypeDialog dlg( mType, this );
  if ( dlg.exec() ) {
    mType = dlg.type();
    if ( !mTypeList.contains( mType ) )
      mTypeList.insert( mTypeList.at( mTypeList.count() - 1 ), mType );
  } else {
    setType( KABC::PhoneNumber::Type( mTypeList.at( mLastSelected ) ) );
  }

  update();
}

PhoneNumberWidget::PhoneNumberWidget( QWidget *parent )
  : QWidget( parent )
{
  QHBoxLayout *layout = new QHBoxLayout( this );
  layout->setSpacing( 11 );
  layout->setMargin( 0 );

  mTypeCombo = new PhoneTypeCombo( this );
  mNumberEdit = new KLineEdit( this );

  layout->addWidget( mTypeCombo );
  layout->addWidget( mNumberEdit );

  connect( mNumberEdit, SIGNAL( textChanged( const QString& ) ), SIGNAL( modified() ) );
}

void PhoneNumberWidget::setNumber( const KABC::PhoneNumber &number )
{
  mNumber = number;

  mTypeCombo->setType( number.type() );
  mNumberEdit->setText( number.number() );
}

KABC::PhoneNumber PhoneNumberWidget::number() const
{
  KABC::PhoneNumber number( mNumber );

  number.setType( mTypeCombo->type() );
  number.setNumber( mNumberEdit->text() );

  return number;
}

void PhoneNumberWidget::setReadOnly( bool readOnly )
{
  mTypeCombo->setEnabled( !readOnly );
  mNumberEdit->setReadOnly( readOnly );
}

PhoneNumberListWidget::PhoneNumberListWidget( QWidget *parent )
  : QWidget( parent ), mReadOnly( false )
{
  mWidgetLayout = new QVBoxLayout( this );

  mMapper = new QSignalMapper( this );
  connect( mMapper, SIGNAL( mapped( int ) ), SLOT( changed( int ) ) );

  setPhoneNumbers( KABC::PhoneNumber::List() );
}

PhoneNumberListWidget::~PhoneNumberListWidget()
{
}

void PhoneNumberListWidget::setReadOnly( bool readOnly )
{
  mReadOnly = readOnly;

  foreach ( PhoneNumberWidget *const widget, mWidgets )
    widget->setReadOnly( readOnly );
}

int PhoneNumberListWidget::phoneNumberCount() const
{
  return mPhoneNumberList.count();
}

void PhoneNumberListWidget::setPhoneNumbers( const KABC::PhoneNumber::List &list )
{
  mPhoneNumberList = list;

  KABC::PhoneNumber::TypeList types;
  types << KABC::PhoneNumber::Home;
  types << KABC::PhoneNumber::Work;
  types << KABC::PhoneNumber::Cell;

  // add an empty entry per default
  if ( mPhoneNumberList.count() < 3 )
    for ( int i = mPhoneNumberList.count(); i < 3; ++i )
      mPhoneNumberList.append( KABC::PhoneNumber( QString(), types[ i ] ) );

  recreateNumberWidgets();
}

KABC::PhoneNumber::List PhoneNumberListWidget::phoneNumbers() const
{
  KABC::PhoneNumber::List list;

  KABC::PhoneNumber::List::ConstIterator it;
  for ( it = mPhoneNumberList.constBegin(); it != mPhoneNumberList.constEnd(); ++it )
    if ( !(*it).number().isEmpty() )
      list.append( *it );

  return list;
}

void PhoneNumberListWidget::add()
{
  mPhoneNumberList.append( KABC::PhoneNumber() );

  recreateNumberWidgets();
}

void PhoneNumberListWidget::remove()
{
  mPhoneNumberList.removeLast();

  recreateNumberWidgets();
}

void PhoneNumberListWidget::recreateNumberWidgets()
{
  foreach ( QWidget *const widget, mWidgets ) {
    mWidgetLayout->removeWidget( widget );
    delete widget;
  }
  mWidgets.clear();

  KABC::PhoneNumber::List::ConstIterator it;
  int counter = 0;
  for ( it = mPhoneNumberList.constBegin(); it != mPhoneNumberList.constEnd(); ++it ) {
    PhoneNumberWidget *wdg = new PhoneNumberWidget( this );
    wdg->setNumber( *it );

    mMapper->setMapping( wdg, counter );
    connect( wdg, SIGNAL( modified() ), mMapper, SLOT( map() ) );

    mWidgetLayout->addWidget( wdg );
    mWidgets.append( wdg );
    wdg->show();

    ++counter;
  }

  setReadOnly( mReadOnly );
}

void PhoneNumberListWidget::changed( int pos )
{
  mPhoneNumberList[ pos ] = mWidgets.at( pos )->number();
}

PhoneEditWidget::PhoneEditWidget( QWidget *parent )
  : QWidget( parent ), mReadOnly( false )
{
  QGridLayout *layout = new QGridLayout( this );
  layout->setSpacing( KDialog::spacingHint() );

  mListScrollArea = new QScrollArea( this );
  mPhoneNumberListWidget = new PhoneNumberListWidget;
  mListScrollArea->setWidget( mPhoneNumberListWidget );
  mListScrollArea->setWidgetResizable( true );

  // ugly but size policies seem to be messed up dialog (parent) wide
  const int scrollAreaMinHeight = mPhoneNumberListWidget->sizeHint().height() +
      mListScrollArea->horizontalScrollBar()->sizeHint().height();
  mListScrollArea->setMinimumHeight( scrollAreaMinHeight );
  layout->addWidget( mListScrollArea, 0, 0, 1, 2 );

  mAddButton = new QPushButton( i18n( "Add" ), this );
  mAddButton->setMaximumSize( mAddButton->sizeHint() );
  layout->addWidget( mAddButton, 1, 0, Qt::AlignRight );

  mRemoveButton = new QPushButton( i18n( "Remove" ), this );
  mRemoveButton->setMaximumSize( mRemoveButton->sizeHint() );
  layout->addWidget( mRemoveButton, 1, 1 );

  connect( mAddButton, SIGNAL( clicked() ), mPhoneNumberListWidget, SLOT( add() ) );
  connect( mRemoveButton, SIGNAL( clicked() ), mPhoneNumberListWidget, SLOT( remove() ) );
  connect( mAddButton, SIGNAL( clicked() ), SLOT( changed() ) );
  connect( mRemoveButton, SIGNAL( clicked() ), SLOT( changed() ) );
}

PhoneEditWidget::~PhoneEditWidget()
{
}

void PhoneEditWidget::setReadOnly( bool readOnly )
{
  mReadOnly = readOnly;
  mAddButton->setEnabled( !readOnly );
  mRemoveButton->setEnabled( !readOnly && mPhoneNumberListWidget->phoneNumberCount() > 3 );

  mPhoneNumberListWidget->setReadOnly( readOnly );
}

void PhoneEditWidget::changed()
{
  mRemoveButton->setEnabled( !mReadOnly && mPhoneNumberListWidget->phoneNumberCount() > 3 );
}

void PhoneEditWidget::loadContact( const KABC::Addressee &contact )
{
  mPhoneNumberListWidget->setPhoneNumbers( contact.phoneNumbers() );
  changed();
}

void PhoneEditWidget::storeContact( KABC::Addressee &contact ) const
{
  const KABC::PhoneNumber::List oldNumbers = contact.phoneNumbers();
  for ( int i = 0; i < oldNumbers.count(); ++i )
    contact.removePhoneNumber( oldNumbers.at( i ) );

  const KABC::PhoneNumber::List newNumbers = mPhoneNumberListWidget->phoneNumbers();
  for ( int i = 0; i < newNumbers.count(); ++i )
    contact.insertPhoneNumber( newNumbers.at( i ) );
}

///////////////////////////////////////////
// PhoneTypeDialog
PhoneTypeDialog::PhoneTypeDialog( KABC::PhoneNumber::Type type, QWidget *parent )
  : KDialog( parent),
    mType( type )
{
  setCaption( i18n( "Edit Phone Number" ) );
  setButtons( Ok | Cancel );
  setDefaultButton( Ok );
  showButtonSeparator( true );

  QWidget *page = new QWidget( this );
  setMainWidget( page );

  QVBoxLayout *layout = new QVBoxLayout( page );
  layout->setSpacing( spacingHint() );
  layout->setMargin( 0 );

  mPreferredBox = new QCheckBox( i18n( "This is the preferred phone number" ), page );
  layout->addWidget( mPreferredBox );

  QGroupBox *box  = new QGroupBox( i18n( "Types" ), page );
  layout->addWidget( box );

  QGridLayout *buttonLayout = new QGridLayout( box );

  // fill widgets
  mTypeList = KABC::PhoneNumber::typeList();
  mTypeList.removeAll( KABC::PhoneNumber::Pref );

  KABC::PhoneNumber::TypeList::ConstIterator it;
  mGroup = new QButtonGroup( box );
  mGroup->setExclusive( false );
  int row, column, counter;
  row = column = counter = 0;
  for ( it = mTypeList.constBegin(); it != mTypeList.constEnd(); ++it, ++counter ) {
    QCheckBox *cb = new QCheckBox( KABC::PhoneNumber::typeLabel( *it ), box );
    cb->setChecked( type & mTypeList[ counter ] );
    buttonLayout->addWidget( cb, row, column );
    mGroup->addButton( cb );

    column++;
    if ( column == 5 ) {
      column = 0;
      ++row;
    }
  }

  mPreferredBox->setChecked( mType & KABC::PhoneNumber::Pref );
}

KABC::PhoneNumber::Type PhoneTypeDialog::type() const
{
  KABC::PhoneNumber::Type type = 0;

  for ( int i = 0; i < mGroup->buttons().count(); ++i ) {
    QCheckBox *box = dynamic_cast<QCheckBox*>( mGroup->buttons().at( i ) ) ;
    if ( box && box->isChecked() )
      type |= mTypeList[ i ];
  }

  if ( mPreferredBox->isChecked() )
    type = type | KABC::PhoneNumber::Pref;
  else
    type = type & ~KABC::PhoneNumber::Pref;

  return type;
}

#include "phoneeditwidget.moc"
