/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "oxutils.h"

#include <kdatetime.h>

using namespace OXA;

QString OXUtils::writeBoolean( bool value )
{
  return (value ? QLatin1String( "true" ) : QLatin1String( "false" ));
}

QString OXUtils::writeNumber( qlonglong value )
{
  return QString::number( value );
}

QString OXUtils::writeString( const QString &value )
{
  QString text( value );
  text.replace( '\\', "\\\\" );
  text.replace( '"', "\\\"" );
  text.replace( '\n', "\\n" );

  return text;
}

QString OXUtils::writeName( const QString &value )
{
  //TODO: assert on invalid names
  return value;
}

QString OXUtils::writeDateTime( const KDateTime &value )
{
  const uint ticks = value.toTime_t();

  return QString::number( ticks ) + QLatin1String( "000" );
}

QString OXUtils::writeDate( const KDateTime &value )
{
  return writeDateTime( value );
}

bool OXUtils::readBoolean( const QString &text )
{
  if ( text == QLatin1String( "true" ) )
    return true;
  else if ( text == QLatin1String( "false" ) )
    return false;
  else {
    Q_ASSERT( false );
    return false;
  }
}

qlonglong OXUtils::readNumber( const QString &text )
{
  return text.toLongLong();
}

QString OXUtils::readString( const QString &text )
{
  QString value( text );
  value.replace( "\\n", "\n" );
  value.replace( "\\\"", "\"" );
  value.replace( "\\\\", "\\" );

  return value;
}

QString OXUtils::readName( const QString &text )
{
  return text;
}

KDateTime OXUtils::readDateTime( const QString &text )
{
  // remove the trailing '000', they exceed the unsigned integer dimension
  const uint ticks = text.mid( 0, text.length() - 3 ).toLongLong();

  KDateTime value;
  value.setTime_t( ticks );

  return value;
}

KDateTime OXUtils::readDate( const QString &text )
{
  return readDateTime( text );
}
