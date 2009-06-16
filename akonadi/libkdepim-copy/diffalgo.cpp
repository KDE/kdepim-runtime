/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <libkdepim/diffalgo.h>
#include <QList>

using namespace KPIM;

void DiffAlgo::begin()
{
  QList<DiffAlgoDisplay*>::Iterator it;
  for ( it = mDisplays.begin(); it != mDisplays.end(); ++it )
    (*it)->begin();
}

void DiffAlgo::end()
{
  QList<DiffAlgoDisplay*>::Iterator it;
  for ( it = mDisplays.begin(); it != mDisplays.end(); ++it )
    (*it)->end();
}

void DiffAlgo::setLeftSourceTitle( const QString &title )
{
  QList<DiffAlgoDisplay*>::Iterator it;
  for ( it = mDisplays.begin(); it != mDisplays.end(); ++it )
    (*it)->setLeftSourceTitle( title );
}

void DiffAlgo::setRightSourceTitle( const QString &title )
{
  QList<DiffAlgoDisplay*>::Iterator it;
  for ( it = mDisplays.begin(); it != mDisplays.end(); ++it )
    (*it)->setRightSourceTitle( title );
}

void DiffAlgo::additionalLeftField( const QString &id, const QString &value )
{
  QList<DiffAlgoDisplay*>::Iterator it;
  for ( it = mDisplays.begin(); it != mDisplays.end(); ++it )
    (*it)->additionalLeftField( id, value );
}

void DiffAlgo::additionalRightField( const QString &id, const QString &value )
{
  QList<DiffAlgoDisplay*>::Iterator it;
  for ( it = mDisplays.begin(); it != mDisplays.end(); ++it )
    (*it)->additionalRightField( id, value );
}

void DiffAlgo::conflictField( const QString &id, const QString &leftValue,
                                 const QString &rightValue )
{
  QList<DiffAlgoDisplay*>::Iterator it;
  for ( it = mDisplays.begin(); it != mDisplays.end(); ++it )
    (*it)->conflictField( id, leftValue, rightValue );
}

void DiffAlgo::addDisplay( DiffAlgoDisplay *display )
{
  if ( !mDisplays.contains( display )  )
    mDisplays.append( display );
}

void DiffAlgo::removeDisplay( DiffAlgoDisplay *display )
{
  mDisplays.removeAll( display );
}
