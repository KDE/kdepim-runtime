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

#include <QtDBus/QDBusMetaType>
#include <QtCore/QDebug>


QDBusArgument & operator << ( QDBusArgument &arg, const Akonadi::Filter::ErrorStack &stack )
{
  arg.beginStructure();
  QStringList locations;
  QStringList errors;
  QList< Akonadi::Filter::ErrorStack::Descriptor > descriptors = stack.errors();
  foreach( Akonadi::Filter::ErrorStack::Descriptor err, descriptors )
  {
    locations << err.first;
    errors << err.second;
  }
  arg << locations;
  arg << errors;
  arg.endStructure();
  return arg;
}

const QDBusArgument & operator >> ( const QDBusArgument &arg, Akonadi::Filter::ErrorStack &stack )
{
  arg.beginStructure();

  QStringList locations;
  QStringList errors;

  QList< Akonadi::Filter::ErrorStack::Descriptor > descriptors;


  arg >> locations;
  arg >> errors;

  QStringList::Iterator it1 = locations.begin();
  QStringList::Iterator it2 = errors.begin();

  while( ( it1 != locations.end() ) && ( it2 != errors.end() ) )
  {
    descriptors.append( qMakePair( *it1, *it2 ) );
    ++it1;
    ++it2;
  }

  stack.setErrors( descriptors );

  arg.endStructure();

  return arg;
}


namespace Akonadi
{
namespace Filter
{

ErrorStack::ErrorStack()
{
}

ErrorStack::ErrorStack( const ErrorStack &src )
  : mErrorList( src.mErrorList )
{
}


ErrorStack::~ErrorStack()
{
}

ErrorStack & ErrorStack::operator = (const ErrorStack & src )
{
  mErrorList = src.mErrorList;
  return *this;
}

void ErrorStack::registerMetaType()
{
  qRegisterMetaType< ErrorStack >();
  qDBusRegisterMetaType< ErrorStack >();
}

void ErrorStack::clearErrors()
{
  mErrorList.clear();
}

void ErrorStack::pushError( const QString &description, const QString &location )
{
  mErrorList.append( qMakePair( location, description ) );
}

void ErrorStack::pushErrorStack( const ErrorStack &stack )
{
  const QList< Descriptor > & errorList = stack.errors();
  Descriptor error;
  foreach( error, errorList )
    pushError( error.second, error.first );
}

QString ErrorStack::errorMessage( const QString &topError ) const
{
  QString ret;

  if( !topError.isEmpty() )
    ret = QString::fromAscii( "%1\n\n" ).arg( topError );

  QString stack;

  Descriptor error;

  int idx = 0;

  foreach( error, mErrorList )
  {
    if( error.first.isEmpty() )
      stack += QString::fromAscii( "  [%1] %2\n" ).arg( idx ).arg( error.second );
    else
      stack += QString::fromAscii( "  [%1] %2: %3\n" ).arg( idx ).arg( error.first ).arg( error.second );
    idx++;
  }

  if( !stack.isEmpty() )
  {
    ret += i18n( "Error Stack:" );
    ret += QString::fromAscii( "\n%1" ).arg( stack );
  }

  return ret;
}

QString ErrorStack::htmlErrorMessage( const QString &topError ) const
{
  QString ret;

  if( !topError.isEmpty() )
    ret = QString::fromAscii( "<p><b>%1</b></p><br><br>" ).arg( topError );

  QString stack;

  int idx = 0;

  Descriptor error;
  foreach( error, mErrorList )
  {
    if( error.first.isEmpty() )
      stack += QString::fromAscii( "<li>[%1] %2</li>" ).arg( idx ).arg( error.second );
    else
      stack += QString::fromAscii( "<li>[%1] %2: %3</li>" ).arg( idx ).arg( error.first ).arg( error.second );
    idx++;
  }

  if( !stack.isEmpty() )
  {
    ret += QString::fromAscii( "<p>%1</p>" ).arg( i18n( "Error Stack:" ) );
    ret += QString::fromAscii( "<p><ul>%1</ul></p>" ).arg( stack );
  }

  return ret;
}

void ErrorStack::dumpErrorMessage( const QString &topError ) const
{
  qDebug() << errorMessage( topError );
}

} // namespace Filter

} // namespace Akonadi

