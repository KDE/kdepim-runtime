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

#include "sievereader.h" // private header

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QByteArray>


namespace Akonadi
{

#if 0

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

#endif

} // namespace Akonadi
