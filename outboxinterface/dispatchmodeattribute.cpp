/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "dispatchmodeattribute.h"

#include <KDebug>

using namespace Akonadi;
using namespace OutboxInterface;


DispatchModeAttribute::DispatchModeAttribute( DispatchMode mode, const QDateTime &date )
  : mMode(mode)
  , mDueDate(date)
{
}

DispatchModeAttribute::~DispatchModeAttribute()
{
}

DispatchModeAttribute* DispatchModeAttribute::clone() const
{
    return new DispatchModeAttribute( mMode, mDueDate );
}

QByteArray DispatchModeAttribute::type() const
{
    static const QByteArray sType( "DispatchModeAttribute" );
    return sType;
}

QByteArray DispatchModeAttribute::serialized() const
{
  switch ( mMode )
  {
    case Immediately: return "immediately";
    case AfterDueDate: return "after" + mDueDate.toString(Qt::ISODate).toLatin1();
    case Never: return "never";
  }

  Q_ASSERT(false);
  return ""; // suppress control-reaches-end-of-non-void-function warning
}

void DispatchModeAttribute::deserialize( const QByteArray &data )
{
  mDueDate = QDateTime();
  if ( data == "immediately" )
    mMode = Immediately;
  else if ( data == "never" )
    mMode = Never;
  else if ( data.startsWith( QByteArray( "after" ) ) )
  {
    mMode = AfterDueDate;
    mDueDate = QDateTime::fromString( data.mid(5), Qt::ISODate );
    // NOTE: 5 is the strlen of "after". Not very maintenance-friendly.
  }
  else
    kWarning() << "Failed to deserialize data [" << data << "]";
}

DispatchModeAttribute::DispatchMode DispatchModeAttribute::dispatchMode() const
{
  return mMode;
}

void DispatchModeAttribute::setDispatchMode( DispatchMode mode )
{
  mMode = mode;
}
  
QDateTime DispatchModeAttribute::dueDate() const
{
  return mDueDate;
}

void DispatchModeAttribute::setDueDate( const QDateTime &date )
{
  mDueDate = date;
}

