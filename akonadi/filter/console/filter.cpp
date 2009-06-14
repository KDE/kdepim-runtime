/******************************************************************************
 *
 *  File : filter.cpp
 *  Created on Sat 13 Jun 2009 06:08:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filter Console Application
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

#include <akonadi/filter/program.h>

Filter::Filter()
{
  mProgram = 0;
  mEditorFactory = 0;
  mComponentFactory = 0;
}

Filter::~Filter()
{
  if( mProgram )
    delete mProgram;
  qDeleteAll( mCollections );
}

void Filter::setProgram( Akonadi::Filter::Program * prog )
{
  Q_ASSERT( prog );
  if( mProgram )
    delete mProgram;
  mProgram = prog;
}

Akonadi::Collection * Filter::findCollection( Akonadi::Collection::Id id )
{
  foreach( Akonadi::Collection * c, mCollections )
  {
    if( c->id() == id )
      return c;
  }

  return 0;
}


bool Filter::hasCollection( Akonadi::Collection::Id id )
{
  foreach( Akonadi::Collection * c, mCollections )
  {
    if( c->id() == id )
      return true;
  }

  return false;
}

void Filter::addCollection( Akonadi::Collection * collection )
{
  Q_ASSERT( !hasCollection( collection->id() ) );
  mCollections.append( collection );
}

void Filter::removeCollection( Akonadi::Collection::Id id )
{
  Akonadi::Collection * c = findCollection( id );
  Q_ASSERT( c );
  mCollections.removeOne( c );
  delete c;
}

