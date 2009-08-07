/****************************************************************************** *
 *
 *  File : errorstack.cpp
 *  Created on Thu 06 Aug 2009 23:37:16 by Szymon Tomasz Stefanek
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

#include "errorstack.h"

#include <KLocale>

#include <QtCore/QDebug>

namespace Akonadi
{
namespace Filter
{

ErrorStack::ErrorStack()
{
}

ErrorStack::~ErrorStack()
{
}

void ErrorStack::clearErrors()
{
  mErrorList.clear();
}

void ErrorStack::pushError( const QString &location, const QString &description )
{
  mErrorList.append( qMakePair( location, description ) );
}

void ErrorStack::pushError( const char *location, const QString &description )
{
  mErrorList.append( qMakePair( QString::fromLatin1( location ), description ) );
}

void ErrorStack::pushErrorStack( const ErrorStack &stack )
{
  const QList< QPair< QString, QString > > & errorList = stack.errors();
  QPair< QString, QString > error;
  foreach( error, errorList )
    pushError( error.first, error.second );
}

QString ErrorStack::errorMessage( const QString &topError )
{
  QString ret;

  if( !topError.isEmpty() )
    ret = QString::fromAscii( "%1\n\n" ).arg( topError );

  QString stack;

  QPair< QString, QString > error;
  foreach( error, mErrorList )
  {
    if( error.first.isEmpty() )
      stack += QString::fromAscii( "  ??: %1\n" ).arg( error.second );
    else
      stack += QString::fromAscii( "  %1: %2\n" ).arg( error.first ).arg( error.second );
  }

  if( !stack.isEmpty() )
  {
    ret += i18n( "Error Stack:" );
    ret += QString::fromAscii( "\n%1" ).arg( stack );
  }

  return ret;
}

QString ErrorStack::htmlErrorMessage( const QString &topError )
{
  QString ret;

  if( !topError.isEmpty() )
    ret = QString::fromAscii( "<p><b>%1</b></p><br><br>" ).arg( topError );

  QString stack;

  QPair< QString, QString > error;
  foreach( error, mErrorList )
  {
    if( error.first.isEmpty() )
      stack += QString::fromAscii( "<li>??: %1</li>" ).arg( error.second );
    else
      stack += QString::fromAscii( "<li>%1: %2</li>" ).arg( error.first ).arg( error.second );
  }

  if( !stack.isEmpty() )
  {
    ret += QString::fromAscii( "<p>%1</p>" ).arg( i18n( "Error Stack:" ) );
    ret += QString::fromAscii( "<p><ul>%1</ul></p>" ).arg( stack );
  }

  return ret;
}

void ErrorStack::dumpErrorMessage( const QString &topError )
{
  qDebug() << errorMessage( topError );
}

} // namespace Filter

} // namespace Akonadi

