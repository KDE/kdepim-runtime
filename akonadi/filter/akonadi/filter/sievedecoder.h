/****************************************************************************** *
 *
 *  File : sievedecoder.h
 *  Created on Sun 03 May 2009 12:10:16 by Szymon Tomasz Stefanek
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

#ifndef _AKONADI_FILTER_IO_SIEVEDECODER_H_
#define _AKONADI_FILTER_IO_SIEVEDECODER_H_

#include "config-akonadi-filter.h"

#include "decoder.h"

#include <QString>
#include <QList>
#include <QVariant>

namespace Akonadi
{
namespace Filter
{

class Program;
class Component;

namespace Condition
{
  class Base;
}

namespace IO
{

namespace Private
{
  class SieveReader;
} // namespace Private

class AKONADI_FILTER_EXPORT SieveDecoder : public Decoder
{
  friend class Private::SieveReader;
public:
  SieveDecoder( Factory * componentFactory );
  ~SieveDecoder();
protected:
  Program * mProgram; // the currently parsed program

  Component * mCurrentComponent; // the component we're 'in'
  QString mCurrentSimpleTestName;
  QList< QVariant > mCurrentSimpleTestArguments;
  QString mCurrentSimpleCommandName;
  QList< QVariant > mCurrentSimpleCommandArguments;
  bool mGotError;
public:

  virtual Program * run();
protected:
  void onCommandStart( const QString & identifier );
  void onCommandEnd();
  void onTestStart( const QString & identifier );
  void onTestEnd();
  void onTestListStart();
  void onTestListEnd();
  void onError( const QString &error );
  void onBlockStart();
  void onBlockEnd();
  void onTaggedArgument( const QString & tag );
  void onStringArgument( const QString & string, bool multiLine, const QString & embeddedHashComment );
  void onNumberArgument( unsigned long number, char quantifier );
  void onStringListEntry( const QString & string, bool multiLine, const QString & embeddedHashComment );

private:
  void addConditionToCurrentComponent( Condition::Base * condition, const QString &identifier );

};

} // namespace IO

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_IO_DECODER_H_
