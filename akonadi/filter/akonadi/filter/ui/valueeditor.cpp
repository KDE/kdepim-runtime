/****************************************************************************** * *
 *
 *  File : valueeditor.cpp
 *  Created on Thu 25 Jun 2009 12:30:16 by Szymon Tomasz Stefanek
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

#include <akonadi/filter/ui/valueeditor.h>

#include <KMessageBox>
#include <KLocale>

#include <QtCore/QDate>

namespace Akonadi
{
namespace Filter
{
namespace UI
{
namespace Private
{

ValueEditor::ValueEditor()
{
}

ValueEditor::~ValueEditor()
{
}

ValueEditor * ValueEditor::editorForDataType( DataType dataType, QWidget * parent )
{
  switch( dataType )
  {
    case DataTypeString:
      return new StringValueEditor( parent );
    break;
    case DataTypeInteger:
      return new IntegerValueEditor( parent );
    break;
    case DataTypeDate:
      return new DateValueEditor( parent );
    break;
    default: // make gcc happy
    break;
  }
  Q_ASSERT_X( false, __FUNCTION__, "Unsupported data type for value editor creation" );
  return 0;
}


StringValueEditor::StringValueEditor( QWidget * parent )
  : QLineEdit( parent ), ValueEditor()
{
}

StringValueEditor::~StringValueEditor()
{
}

DataType StringValueEditor::dataType()
{
  return DataTypeString;
}

QVariant StringValueEditor::value()
{
  return text();
}

void StringValueEditor::setValue( const QVariant &val )
{
  setText( val.toString() );
}

QWidget * StringValueEditor::widget()
{
  return this;
}




IntegerValueEditor::IntegerValueEditor( QWidget * parent )
  : QLineEdit( parent ), ValueEditor()
{
}

IntegerValueEditor::~IntegerValueEditor()
{
}

DataType IntegerValueEditor::dataType()
{
  return DataTypeInteger;
}

QVariant IntegerValueEditor::value()
{
  QString val = text();
  bool ok;
  Integer i = val.toLongLong( &ok );
  if( !ok )
  {
    KMessageBox::error( this, i18n( "The operand must be numeric!" ), i18n( "Invalid value" ) );
    setFocus();
    return QVariant();
  }
  return QVariant( i );
}

void IntegerValueEditor::setValue( const QVariant &val )
{
  bool ok;
  Integer i = val.toLongLong( &ok );
  if( ok )
    setText( val.toString() );
  else
    setText( "0" );
}

QWidget * IntegerValueEditor::widget()
{
  return this;
}




DateValueEditor::DateValueEditor( QWidget * parent )
  : KDateWidget( parent ), ValueEditor()
{
}

DateValueEditor::~DateValueEditor()
{
}

DataType DateValueEditor::dataType()
{
  return DataTypeDate;
}

QVariant DateValueEditor::value()
{
  return QVariant( date() );
}

void DateValueEditor::setValue( const QVariant &val )
{
  QDate d = val.toDate();
  if( d.isValid() )
    setDate( d );
  else
    setDate( QDate::currentDate() );
}

QWidget * DateValueEditor::widget()
{
  return this;
}


} // namespace Private

} // namespace UI

} // namespace Filter

} // namespace Akonadi

