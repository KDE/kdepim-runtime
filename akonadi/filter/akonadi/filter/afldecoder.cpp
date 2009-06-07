/****************************************************************************** *
 *
 *  File : afldecoder.cpp
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

#include "afldecoder.h"
#include "program.h"
#include "componentfactory.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QByteArray>

namespace Akonadi
{
namespace Filter
{
namespace IO
{

AFLDecoder::AFLDecoder( ComponentFactory * componentFactory )
  : Decoder( componentFactory ), mCurrentChar( 0 ), mProgram( 0 )
{
}

AFLDecoder::~AFLDecoder()
{
  if( mProgram )
    delete mProgram;
}

void AFLDecoder::nextChar()
{
  Q_ASSERT( mCurrentChar );
  Q_ASSERT( !mCurrentChar->isNull() );
  if( mCurrentChar->unicode() == '\n' )
    mCurrentLine++;
  mCurrentChar++;
}

void AFLDecoder::parseComment()
{
  // We actually just skip the comments, later we might extract them
  Q_ASSERT( mCurrentChar->unicode() == '#' );

  while( !mCurrentChar->isNull() )
  {
    if ( mCurrentChar->unicode() == '\n' )
    {
      nextChar();
      return;
    } 
    nextChar();
  }
}

bool AFLDecoder::fatal( const QString &error )
{
  qDebug( "FATAL: at line %u: %s", mCurrentLine, error.toUtf8().data() );
  return false;
}

bool AFLDecoder::fatalUnexpectedCharacter( const QString &where )
{
  Q_ASSERT( mCurrentChar );

  QString error;

  if( mCurrentChar->isNull() )
    return fatal( QObject::tr( "Unexpected end of stream %1" ).arg( where ) );

  return fatal( QObject::tr( "Unexpected character 0x%1 %2" ).arg( mCurrentChar->unicode(), 16 ).arg( where ) );
}


bool AFLDecoder::extractIdentifier( QString &buffer )
{
  Q_ASSERT( mCurrentChar );
  Q_ASSERT( mCurrentChar->isLetter() );

  const QChar * begin = mCurrentChar;

  nextChar();

  while( mCurrentChar->isLetter() || mCurrentChar->isDigit() )
    nextChar();

  buffer.setUnicode( begin, mCurrentChar - begin );

  return true;
}


bool AFLDecoder::parseDeclaration()
{
  Q_ASSERT( mCurrentChar );

  // look for the beginning of the declaration
  for( ;; )
  {
    if( mCurrentChar->isNull() )
      return true; // finished

    if( mCurrentChar->isSpace() )
    {
      // skip space
      while( mCurrentChar->isSpace() )
        nextChar();
      continue;
    }

    if( mCurrentChar->unicode() == '#' )
    {
      // skip comment
      parseComment();
      continue;
    }

    if( mCurrentChar->isLetter() )
      break; // got identifier start

    // unexpected character
    return fatalUnexpectedCharacter( QObject::tr( "while looking for the 'filter' statement" ) );
  }

  // pointing to a letter
  QString filter;

  if( !extractIdentifier( filter ) )
    return false;
  
  if( QString::compare( filter, QLatin1String( "filter" ), Qt::CaseInsensitive ) != 0 )
    return fatal( QObject::tr( "Unexpected token %1, while looking for the 'filter' statement" ).arg( filter ) );

  if( mCurrentChar->isSpace() )
  {
    // skip space
    while( mCurrentChar->isSpace() )
      nextChar();
  }

  if( mCurrentChar->unicode() != '(' )
    return fatalUnexpectedCharacter( QObject::tr( "after the 'filter' statement" ) );

  nextChar();



  return true;
}

Program * AFLDecoder::run()
{
  if( mProgram )
  {
    delete mProgram;
    mProgram = 0;
  }

  QString test = QString::fromUtf8(
      "# This is a comment\r\n" \
      "filter( # Yet another comment \r\n" \
      "  version(1.0),\r\n" \
      "  description(\"My test filter\"),\r\n" \
      "  rule(\r\n" \
      "    condition(\r\n" \
      "      ),\r\n" \
      "    action(\r\n" \
      "      )\r\n" \
      "    ),\r\n" \
      "  rule(\r\n" \
      "    condition(\r\n" \
      "      ),\r\n" \
      "    action(\r\n" \
      "      )\r\n" \
      "    )\r\n" \
      "  )\r\n"
    );

  mCurrentLine = 1;
  mCurrentChar = test.constData();
  
  if ( !parseDeclaration() )
    return 0;

  if ( !mProgram )
  {
    fatal( QObject::tr( "The script doesn't contain a valid filter specification" ) );
    return 0;
  }

  Program * program = mProgram;
  mProgram = 0;
  return program;
}


} // namespace IO

} // namespace Filter

} // namespace Akonadi

