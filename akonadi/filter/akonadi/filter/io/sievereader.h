/****************************************************************************** *
 *
 *  File : sievereader.h
 *  Created on Fri 08 May 2009 14:30:22 by Szymon Tomasz Stefanek
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

#ifndef _AKONADI_FILTER_IO_PRIVATE_SIEVEREADER_H_
#define _AKONADI_FILTER_IO_PRIVATE_SIEVEREADER_H_

///
/// This file is internal and shouldn't be installed as public API
/// as it depends on the ksieve headers.
///

#include <akonadi/filter/config-akonadi-filter.h>

// These headers should be probably installed in some other place
#include <akonadi/filter/ksieve/scriptbuilder.h>
#include <akonadi/filter/ksieve/error.h>
#include <akonadi/filter/ksieve/parser.h>

namespace Akonadi
{
namespace Filter
{
namespace IO
{

class SieveDecoder;

namespace Private
{

class SieveReader : public KSieve::ScriptBuilder
{
public:
  SieveReader( SieveDecoder * decoder );
  ~SieveReader();

protected:
  virtual void taggedArgument( const QString & tag );
  virtual void stringArgument( const QString & string, bool multiLine, const QString & embeddedHashComment );
  virtual void numberArgument( unsigned long number, char quantifier );

  virtual void stringListArgumentStart();
  virtual void stringListEntry( const QString & string, bool multiLine, const QString & embeddedHashComment );
  virtual void stringListArgumentEnd();

  virtual void commandStart( const QString & identifier );
  virtual void commandEnd();

  virtual void testStart( const QString & identifier );
  virtual void testEnd();

  virtual void testListStart();
  virtual void testListEnd();

  virtual void blockStart();
  virtual void blockEnd();

  virtual void hashComment( const QString & comment );
  virtual void bracketComment( const QString & comment );

  virtual void lineFeed();

  virtual void error( const KSieve::Error & error );

  virtual void finished();

protected:
  SieveDecoder * mDecoder;

}; // class SieveReader

} // namespace Private

} // namespace IO

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_IO_PRIVATE_SIEVEREADER_H_
