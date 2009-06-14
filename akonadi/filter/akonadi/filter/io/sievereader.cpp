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

#include <akonadi/filter/io/sievereader.h>
#include <akonadi/filter/io/sievedecoder.h>

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
  qDebug( "Tagged argument '%s'", tag.toUtf8().data() );
  mDecoder->onTaggedArgument( tag );
}

void SieveReader::stringArgument( const QString & string, bool multiLine, const QString & embeddedHashComment )
{
  qDebug( "String argument '%s' (multiline=%d, comment='%s')", string.toUtf8().data(), multiLine, embeddedHashComment.toUtf8().data() );
  mDecoder->onStringArgument( string, multiLine, embeddedHashComment );
}

void SieveReader::numberArgument( unsigned long number, char quantifier )
{
  qDebug( "Number argument %lu (quantifier=%d)", number, quantifier );
  mDecoder->onNumberArgument( number, quantifier );
}

void SieveReader::stringListArgumentStart()
{
  qDebug( "String list argument start" );
  mDecoder->onStringListArgumentStart();
}

void SieveReader::stringListEntry( const QString & string, bool multiLine, const QString & embeddedHashComment )
{
  qDebug( "String list entry '%s' (multiline=%d, comment='%s')", string.toUtf8().data(), multiLine, embeddedHashComment.toUtf8().data() );
  mDecoder->onStringListEntry( string, multiLine, embeddedHashComment );
}

void SieveReader::stringListArgumentEnd()
{
  qDebug( "String list argument end" );
  mDecoder->onStringListArgumentEnd();
}

void SieveReader::commandStart( const QString & identifier )
{
  qDebug( "CommandDescriptor start '%s'", identifier.toUtf8().data() );
  mDecoder->onCommandDescriptorStart( identifier );
}

void SieveReader::commandEnd()
{
  qDebug( "CommandDescriptor end" );
  mDecoder->onCommandDescriptorEnd();
}

void SieveReader::testStart( const QString & identifier )
{
  qDebug( "Test start '%s'", identifier.toUtf8().data() );
  mDecoder->onTestStart( identifier );  
}

void SieveReader::testEnd()
{
  qDebug( "Test end" );
  mDecoder->onTestEnd();
}

void SieveReader::testListStart()
{
  qDebug( "Test list start" );
  mDecoder->onTestListStart();
}

void SieveReader::testListEnd()
{
  qDebug( "Test list end" );
  mDecoder->onTestListEnd();
}

void SieveReader::blockStart()
{
  qDebug( "Block start" );
  mDecoder->onBlockStart();
}

void SieveReader::blockEnd()
{
  qDebug( "Block end" );
  mDecoder->onBlockEnd();
}

void SieveReader::hashComment( const QString & comment )
{
  qDebug( "Hash comment ('%s')", comment.toUtf8().data() );
}

void SieveReader::bracketComment( const QString & comment )
{
  qDebug( "Bracket comment ('%s')", comment.toUtf8().data() );
}

void SieveReader::lineFeed()
{
  qDebug( "Line feed" );
}

void SieveReader::error( const KSieve::Error & error )
{
  qDebug( "Error: %s", error.asString().toUtf8().data() );
  mDecoder->onError( error.asString() );
}

void SieveReader::finished()
{
  qDebug( "Finished" );
}

} // namespace Private

} // namespace IO

} // namespace Filter

} // namespace Akonadi
