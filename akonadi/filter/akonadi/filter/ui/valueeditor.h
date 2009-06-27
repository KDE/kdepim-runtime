/****************************************************************************** * *
 *
 *  File : valueeditor.h
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

#ifndef _AKONADI_FILTER_UI_VALUEEDITOR_H_
#define _AKONADI_FILTER_UI_VALUEEDITOR_H_

#include <akonadi/filter/ui/config-akonadi-filter-ui.h>

#include <akonadi/filter/datatype.h>

#include <QtCore/QVariant>

#include <QtGui/QLineEdit>
#include <QtGui/QWidget>

#include <KDateWidget>

namespace Akonadi
{
namespace Filter
{
namespace UI
{
namespace Private
{

/**
 * The interface for all the value editors.
 */
class AKONADI_FILTER_UI_EXPORT ValueEditor
{
public:
  ValueEditor();
  virtual ~ValueEditor();
public:
  virtual DataType dataType() = 0;
  virtual QVariant value() = 0;
  virtual void setValue( const QVariant &val ) = 0;
  virtual QWidget * widget() = 0;

  static ValueEditor * editorForDataType( DataType dataType, QWidget * parent );
}; // class ValueEditor

class AKONADI_FILTER_UI_EXPORT StringValueEditor : public QLineEdit, public ValueEditor
{
  Q_OBJECT
public:
  StringValueEditor( QWidget * parent );
  virtual ~StringValueEditor();
public:
  DataType dataType();
  virtual QVariant value();
  virtual void setValue( const QVariant &val );
  virtual QWidget * widget();
};

class AKONADI_FILTER_UI_EXPORT IntegerValueEditor : public QLineEdit, public ValueEditor
{
  Q_OBJECT
public:
  IntegerValueEditor( QWidget * parent );
  virtual ~IntegerValueEditor();
public:
  DataType dataType();
  virtual QVariant value();
  virtual void setValue( const QVariant &val );
  virtual QWidget * widget();
};

class AKONADI_FILTER_UI_EXPORT DateValueEditor : public KDateWidget, public ValueEditor
{
  Q_OBJECT
public:
  DateValueEditor( QWidget * parent );
  virtual ~DateValueEditor();
public:
  DataType dataType();
  virtual QVariant value();
  virtual void setValue( const QVariant &val );
  virtual QWidget * widget();
};

} // namespace Private

} // namespace UI

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_UI_VALUEEDITOR_H_
