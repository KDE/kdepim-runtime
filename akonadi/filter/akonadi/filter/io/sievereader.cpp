/****************************************************************************** *
 *
 *  File : sievereader.cpp
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

#include "sievereader.h"
#include "sievedecoder.h"

namespace Akonadi
{
namespace Filter
{
namespace IO
{
namespace Private
{

SieveReader::SieveReader( SieveDecoder * decoder )
  : KSieve::ScriptBuilder(), mDecoder( decoder )
{
  Q_ASSERT( mDecoder );
}

SieveReader::~SieveReader()
{
}


void SieveReader::taggedArgument( const QString & tag )
{
  mDecoder->onTaggedArgument( tag );
}

void SieveReader::stringArgument( const QString & string, bool multiLine, const QString & embeddedHashComment )
{
  mDecoder->onStringArgument( string, multiLine, embeddedHashComment );
}

void SieveReader::numberArgument( unsigned long number, char quantifier )
{
  mDecoder->onNumberArgument( number, quantifier );
}

void SieveReader::stringListArgumentStart()
{
  mDecoder->onStringListArgumentStart();
}

void SieveReader::stringListEntry( const QString & string, bool multiLine, const QString & embeddedHashComment )
{
  mDecoder->onStringListEntry( string, multiLine, embeddedHashComment );
}

void SieveReader::stringListArgumentEnd()
{
  mDecoder->onStringListArgumentEnd();
}

void SieveReader::commandStart( const QString & identifier )
{
  mDecoder->onCommandDescriptorStart( identifier );
}

void SieveReader::commandEnd()
{
  mDecoder->onCommandDescriptorEnd();
}

void SieveReader::testStart( const QString & identifier )
{
  mDecoder->onTestStart( identifier );  
}

void SieveReader::testEnd()
{
  mDecoder->onTestEnd();
}

void SieveReader::testListStart()
{
  mDecoder->onTestListStart();
}

void SieveReader::testListEnd()
{
  mDecoder->onTestListEnd();
}

void SieveReader::blockStart()
{
  mDecoder->onBlockStart();
}

void SieveReader::blockEnd()
{
  mDecoder->onBlockEnd();
}

void SieveReader::hashComment( const QString & comment )
{
  mDecoder->onComment( comment );
}

void SieveReader::bracketComment( const QString & comment )
{
  mDecoder->onComment( comment );
}

void SieveReader::lineFeed()
{
}

void SieveReader::error( const KSieve::Error & error )
{
  mDecoder->onError( error.asString() );
}

void SieveReader::finished()
{
}

} // namespace Private

} // namespace IO

} // namespace Filter

} // namespace Akonadi
