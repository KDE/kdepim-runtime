/****************************************************************************** *
 *
 *  File : program.cpp
 *  Created on Thu 07 May 2009 13:30:16 by Szymon Tomasz Stefanek
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

#include "program.h"

#include <QObject>

#include <KLocale>
#include <KDebug>

#include "componentfactory.h"

#include "io/sieveencoder.h"
#include "io/sievedecoder.h"

namespace Akonadi
{
namespace Filter 
{

Program::Program( ComponentFactory * factory )
  : Action::RuleList( 0 ), PropertyBag(), mComponentFactory( factory )
{
  Q_ASSERT( mComponentFactory );
}

Program::~Program()
{
}

bool Program::isProgram() const
{
  return true;
}

QString Program::name() const
{
  return property( QString::fromAscii( "name" ) ).toString();
}

void Program::setName( const QString &name )
{
  setProperty( QString::fromAscii( "name" ), QVariant( name ) );
}

void Program::dump( const QString &prefix )
{
  debugOutput( prefix, "Program" );
  dumpRuleList( prefix );
}

Program * Program::clone()
{
  errorStack().clearErrors();

  IO::SieveEncoder encoder;
  QByteArray serialized = encoder.run( this );
  if( serialized.isEmpty() )
  {
    errorStack().pushErrorStack( encoder.errorStack() );
    errorStack().pushError( i18n( "Encoding of the program into Sieve source failed" ) );
    return 0;
  }

  IO::SieveDecoder decoder( mComponentFactory );
  Program * prog = decoder.run( serialized );
  if( !prog )
  {
    errorStack().pushErrorStack( decoder.errorStack() );
    errorStack().pushError( i18n( "Decoding of the program from Sieve source failed" ) );
    return 0;
  }

  return prog;
}

} // namespace Filter

} // namespace Akonadi

