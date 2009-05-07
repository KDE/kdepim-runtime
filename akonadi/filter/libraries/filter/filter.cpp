/****************************************************************************** *
 *
 *  File : filter.cpp
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

#include "filter.h"

#include <ksieve/parser.h>
#include <ksieve/error.h>
#include <ksieve/scriptbuilder.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QByteArray>

namespace Akonadi
{

class SieveReader : public KSieve::ScriptBuilder
{
public:
  SieveReader( Filter * filter );
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
  Filter * mFilter;
};

SieveReader::SieveReader( Filter * filter )
  : KSieve::ScriptBuilder(), mFilter( filter )
{
}

SieveReader::~SieveReader()
{
}


void SieveReader::taggedArgument( const QString & tag )
{
  qDebug( "Tagged argument '%s'", tag.toUtf8().data() );
}

void SieveReader::stringArgument( const QString & string, bool multiLine, const QString & embeddedHashComment )
{
  qDebug( "String argument '%s' (multiline=%d, comment='%s')", string.toUtf8().data(), multiLine, embeddedHashComment.toUtf8().data() );
}

void SieveReader::numberArgument( unsigned long number, char quantifier )
{
  qDebug( "Number argument %lu (quantifier=%d)", number, quantifier );
}

void SieveReader::stringListArgumentStart()
{
  qDebug( "String list argument start" );
}

void SieveReader::stringListEntry( const QString & string, bool multiLine, const QString & embeddedHashComment )
{
  qDebug( "String list entry '%s' (multiline=%d, comment='%s')", string.toUtf8().data(), multiLine, embeddedHashComment.toUtf8().data() );
}

void SieveReader::stringListArgumentEnd()
{
  qDebug( "String list argument end" );
}

void SieveReader::commandStart( const QString & identifier )
{
  qDebug( "Command start '%s'", identifier.toUtf8().data() );
}

void SieveReader::commandEnd()
{
  qDebug( "Command end" );
}

void SieveReader::testStart( const QString & identifier )
{
  qDebug( "Test start '%s'", identifier.toUtf8().data() );
}

void SieveReader::testEnd()
{
  qDebug( "Test end" );
}

void SieveReader::testListStart()
{
  qDebug( "Test list start" );
}

void SieveReader::testListEnd()
{
  qDebug( "Test list end" );
}

void SieveReader::blockStart()
{
  qDebug( "Block start" );
}

void SieveReader::blockEnd()
{
  qDebug( "Block end" );
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
}

void SieveReader::finished()
{
  qDebug( "Finished" );
}



Filter::Filter()
{
}

Filter::~Filter()
{
}

bool Filter::loadSieveScript( const QString &fileName )
{
  QFile f(fileName);

  if ( !f.open( QFile::ReadOnly ) )
    return false;

  QTextStream stream( &f );

  return loadSieveScript( stream );
}

bool Filter::loadSieveScript( QTextStream &stream )
{
  QString script = stream.readAll();

  QByteArray utf8Script = script.toUtf8();

  KSieve::Parser parser(utf8Script.data(),utf8Script.data() + utf8Script.length());

  SieveReader sieveReader( this );

  parser.setScriptBuilder( &sieveReader );

  if ( !parser.parse() )
    return false;

  return true;
}

bool Filter::saveSieveScript( const QString &fileName )
{
  QFile f(fileName);

  if ( !f.open( QFile::WriteOnly ) )
    return false;

  QTextStream stream( &f );

  return saveSieveScript( stream );
}

bool Filter::saveSieveScript( QTextStream &stream )
{
  return true;
}



} // namespace Akonadi
