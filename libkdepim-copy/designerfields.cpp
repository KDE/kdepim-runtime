/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <qcheckbox.h>
#include <qcombobox.h>
#include <q3datetimeedit.h>
#include <qlayout.h>
#include <qobject.h>
#include <qspinbox.h>
#include <qregexp.h>
#include <q3textedit.h>
#include <QFile>
#include <QtDesigner/QFormBuilder>
//Added by qt3to4:
#include <QVBoxLayout>

#include <kdatepicker.h>
#include <kdatetimewidget.h>
#include <kdialog.h>
#include <klineedit.h>
#include <kstandarddirs.h>
#include <kdebug.h>

#include "designerfields.h"

using namespace KPIM;

DesignerFields::DesignerFields( const QString &uiFile, QWidget *parent,
  const char *name )
  : QWidget( parent, name )
{
  initGUI( uiFile );
}

void DesignerFields::initGUI( const QString &uiFile )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  QFile f( uiFile );
  QFormBuilder builder;
  QWidget *wdg = builder.load( &f, this );
  if ( !wdg ) {
    kdError() << "No ui file found" << endl;
    return;
  }

  mTitle = wdg->caption();
  mIdentifier = wdg->name();

  layout->addWidget( wdg );

  QObjectList list = wdg->queryList( "QWidget" );
  QObject *it;

  QStringList allowedTypes;
  allowedTypes << "QLineEdit"
               << "QTextEdit"
               << "QSpinBox"
               << "QCheckBox"
               << "QComboBox"
               << "QDateTimeEdit"
               << "KLineEdit"
               << "KDateTimeWidget"
               << "KDatePicker";

  Q_FOREACH( it, list ) {
    if ( allowedTypes.contains( it->className() ) ) {
      QString name = it->name();
      if ( name.startsWith( "X_" ) ) {
        name = name.mid( 2 );

        QWidget *widget = static_cast<QWidget*>( it );
        if ( !name.isEmpty() )
          mWidgets.insert( name, widget );

        if ( it->inherits( "QLineEdit" ) )
          connect( it, SIGNAL( textChanged( const QString& ) ),
                   SIGNAL( modified() ) );
        else if ( it->inherits( "QSpinBox" ) )
          connect( it, SIGNAL( valueChanged( int ) ),
                   SIGNAL( modified() ) );
        else if ( it->inherits( "QCheckBox" ) )
          connect( it, SIGNAL( toggled( bool ) ),
                   SIGNAL( modified() ) );
        else if ( it->inherits( "QComboBox" ) )
          connect( it, SIGNAL( activated( const QString& ) ),
                   SIGNAL( modified() ) );
        else if ( it->inherits( "QDateTimeEdit" ) )
          connect( it, SIGNAL( valueChanged( const QDateTime& ) ),
                   SIGNAL( modified() ) );
        else if ( it->inherits( "KDateTimeWidget" ) )
          connect( it, SIGNAL( valueChanged( const QDateTime& ) ),
                   SIGNAL( modified() ) );
        else if ( it->inherits( "KDatePicker" ) )
          connect( it, SIGNAL( dateChanged( QDate ) ),
                   SIGNAL( modified() ) );
        else if ( it->inherits( "QTextEdit" ) )
          connect( it, SIGNAL( textChanged() ),
                   SIGNAL( modified() ) );

        if ( !widget->isEnabled() )
          mDisabledWidgets.append( widget );
      }
    }
  }
}

QString DesignerFields::identifier() const
{
  return mIdentifier;
}

QString DesignerFields::title() const
{
  return mTitle;
}

void DesignerFields::load( DesignerFields::Storage *storage )
{
  QStringList keys = storage->keys();
    
  // clear all custom page widgets 
  // we can't do this in the following loop, as it works on the 
  // custom fields of the vcard, which may not be set.
  QMap<QString, QWidget *>::ConstIterator widIt;
  for ( widIt = mWidgets.begin(); widIt != mWidgets.end(); ++widIt ) {
    QString value;
    if ( widIt.data()->inherits( "QLineEdit" ) ) {
      QLineEdit *wdg = static_cast<QLineEdit*>( widIt.data() );
      wdg->setText( QString() );
    } else if ( widIt.data()->inherits( "QSpinBox" ) ) {
      QSpinBox *wdg = static_cast<QSpinBox*>( widIt.data() );
      wdg->setValue( wdg->minValue() );
    } else if ( widIt.data()->inherits( "QCheckBox" ) ) {
      QCheckBox *wdg = static_cast<QCheckBox*>( widIt.data() );
      wdg->setChecked( false );
    } else if ( widIt.data()->inherits( "QDateTimeEdit" ) ) {
      Q3DateTimeEdit *wdg = static_cast<Q3DateTimeEdit*>( widIt.data() );
      wdg->setDateTime( QDateTime::currentDateTime() );
    } else if ( widIt.data()->inherits( "KDateTimeWidget" ) ) {
      KDateTimeWidget *wdg = static_cast<KDateTimeWidget*>( widIt.data() );
      wdg->setDateTime( QDateTime::currentDateTime() );
    } else if ( widIt.data()->inherits( "KDatePicker" ) ) {
      KDatePicker *wdg = static_cast<KDatePicker*>( widIt.data() );
      wdg->setDate( QDate::currentDate() );
    } else if ( widIt.data()->inherits( "QComboBox" ) ) {
      QComboBox *wdg = static_cast<QComboBox*>( widIt.data() );
      wdg->setCurrentItem( 0 );
    } else if ( widIt.data()->inherits( "QTextEdit" ) ) {
      Q3TextEdit *wdg = static_cast<Q3TextEdit*>( widIt.data() );
      wdg->setText( QString() );
    }
  }

  QStringList::ConstIterator it2;
  for ( it2 = keys.begin(); it2 != keys.end(); ++it2 ) {
    QString value = storage->read( *it2 );

    QMap<QString, QWidget *>::ConstIterator it = mWidgets.find( *it2 );
    if ( it != mWidgets.end() ) {
      if ( it.data()->inherits( "QLineEdit" ) ) {
        QLineEdit *wdg = static_cast<QLineEdit*>( it.data() );
        wdg->setText( value );
      } else if ( it.data()->inherits( "QSpinBox" ) ) {
        QSpinBox *wdg = static_cast<QSpinBox*>( it.data() );
        wdg->setValue( value.toInt() );
      } else if ( it.data()->inherits( "QCheckBox" ) ) {
        QCheckBox *wdg = static_cast<QCheckBox*>( it.data() );
        wdg->setChecked( value == "true" || value == "1" );
      } else if ( it.data()->inherits( "QDateTimeEdit" ) ) {
        Q3DateTimeEdit *wdg = static_cast<Q3DateTimeEdit*>( it.data() );
        wdg->setDateTime( QDateTime::fromString( value, Qt::ISODate ) );
      } else if ( it.data()->inherits( "KDateTimeWidget" ) ) {
        KDateTimeWidget *wdg = static_cast<KDateTimeWidget*>( it.data() );
        wdg->setDateTime( QDateTime::fromString( value, Qt::ISODate ) );
      } else if ( it.data()->inherits( "KDatePicker" ) ) {
        KDatePicker *wdg = static_cast<KDatePicker*>( it.data() );
        wdg->setDate( QDate::fromString( value, Qt::ISODate ) );
      } else if ( it.data()->inherits( "QComboBox" ) ) {
        QComboBox *wdg = static_cast<QComboBox*>( it.data() );
        wdg->setCurrentText( value );
      } else if ( it.data()->inherits( "QTextEdit" ) ) {
        Q3TextEdit *wdg = static_cast<Q3TextEdit*>( it.data() );
        wdg->setText( value );
      }
    }
  }
}

void DesignerFields::save( DesignerFields::Storage *storage )
{
  QMap<QString, QWidget*>::Iterator it;
  for ( it = mWidgets.begin(); it != mWidgets.end(); ++it ) {
    QString value;
    if ( it.data()->inherits( "QLineEdit" ) ) {
      QLineEdit *wdg = static_cast<QLineEdit*>( it.data() );
      value = wdg->text();
    } else if ( it.data()->inherits( "QSpinBox" ) ) {
      QSpinBox *wdg = static_cast<QSpinBox*>( it.data() );
      value = QString::number( wdg->value() );
    } else if ( it.data()->inherits( "QCheckBox" ) ) {
      QCheckBox *wdg = static_cast<QCheckBox*>( it.data() );
      value = ( wdg->isChecked() ? "true" : "false" );
    } else if ( it.data()->inherits( "QDateTimeEdit" ) ) {
      Q3DateTimeEdit *wdg = static_cast<Q3DateTimeEdit*>( it.data() );
      value = wdg->dateTime().toString( Qt::ISODate );
    } else if ( it.data()->inherits( "KDateTimeWidget" ) ) {
      KDateTimeWidget *wdg = static_cast<KDateTimeWidget*>( it.data() );
      value = wdg->dateTime().toString( Qt::ISODate );
    } else if ( it.data()->inherits( "KDatePicker" ) ) {
      KDatePicker *wdg = static_cast<KDatePicker*>( it.data() );
      value = wdg->date().toString( Qt::ISODate );
    } else if ( it.data()->inherits( "QComboBox" ) ) {
      QComboBox *wdg = static_cast<QComboBox*>( it.data() );
      value = wdg->currentText();
    } else if ( it.data()->inherits( "QTextEdit" ) ) {
      Q3TextEdit *wdg = static_cast<Q3TextEdit*>( it.data() );
      value = wdg->text();
   }

   storage->write( it.key(), value );
  }
}

void DesignerFields::setReadOnly( bool readOnly )
{
  QMap<QString, QWidget*>::Iterator it;
  for ( it = mWidgets.begin(); it != mWidgets.end(); ++it )
    if ( mDisabledWidgets.find( it.data() ) == mDisabledWidgets.end() )
      it.data()->setEnabled( !readOnly );
}

#include "designerfields.moc"
