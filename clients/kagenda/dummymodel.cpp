/*
    This file is part of Akonadi.

    Copyright (c) 2006 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "dummymodel.h"

#include <QPainter>

DummyModel::DummyModel()
{
}

QString DummyModel::label( int position ) const
{
  return QString::number( position );
}

QColor DummyModel::cellColor( int position ) const
{
  if ( position % 2 == 0 ) return QColor( 183, 216, 252 );
  else return QColor( "yellow" );
}

QColor DummyModel::decorationColor( int position ) const
{
  if ( position / 10 % 2 == 0 ) return QColor( "#EEC19F" );
  else return QColor( "#A1EEA6" );
}

QString DummyModel::decorationLabel( int position ) const
{
  if ( position % 10 != 0 ) return QString();

  return QString::number( int( position / 10 ) );
}

bool DummyModel::hasDecoration() const
{
  return true;
}
