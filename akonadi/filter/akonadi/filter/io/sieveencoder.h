/****************************************************************************** *
 *
 *  File : sieveencoder.h
 *  Created on Sun 15 Jun 2009 00:14:16 by Szymon Tomasz Stefanek
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
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#ifndef _AKONADI_FILTER_IO_SIEVEENCODER_H_
#define _AKONADI_FILTER_IO_SIEVEENCODER_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <akonadi/filter/io/encoder.h>
#include <akonadi/filter/datatype.h>

#include <QtCore/QString>
#include <QtCore/QVariant>

namespace Akonadi
{
namespace Filter
{
  namespace Action
  {
    class Base;
    class RuleList;
  } // namespace Action;

  namespace Condition
  {
    class Base;
  } // namespace Condition;
  class Program;
  class Rule;
  class RuleList;

namespace IO
{

class AKONADI_FILTER_EXPORT SieveEncoder : public Encoder
{
public:
  SieveEncoder();
  virtual ~SieveEncoder();

protected:
  QString mBuffer;
  QString mIndentText;
  QString mLineSeparator;
  QString mCurrentIndentPrefix;
  int mIndentLevel;
public:
  const QString & lineSeparator() const
  {
    return mLineSeparator;
  }

  void setLineSeparator( const QString &lineSeparator )
  {
    mLineSeparator = lineSeparator;
  }


  const QString & indentText() const
  {
    return mIndentText;
  }

  void setIndentText( const QString &indentText )
  {
    mIndentText = indentText;
  }
 
  virtual QString run( Program * program );

protected:
  void beginLine();
  void addLineData( const QString &data );
  void endLine();
  void addLine( const QString &line );
  void pushIndent();
  void popIndent();
  bool encodeRuleList( Action::RuleList * ruleList );
  bool encodeRule( Rule * rule, bool isFirst, bool isLast );
  bool encodeCondition( Condition::Base * condition );
  bool encodeAction( Action::Base * action );
  bool encodeData( const QVariant &data, DataType dataType );

}; // class SieveEncoder

} // namespace IO

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_IO_SIEVEENCODER_H_
