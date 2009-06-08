/******************************************************************************
 *
 *  File : engine.cpp
 *  Created on Mon 8 Jun 2009 04:17:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi  Filtering Agent
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

#include "engine.h"

#include <akonadi/filter/program.h>
#include <akonadi/filter/sievedecoder.h>
#include <akonadi/filter/componentfactory.h>

FilterEngine::FilterEngine(
   const QString &id,
   Akonadi::Filter::ComponentFactory * componentFactory
 )
  : mId( id ), mComponentFactory( componentFactory ), mProgram( 0 )
{
  Q_ASSERT( mComponentFactory );
}

FilterEngine::~FilterEngine()
{
  if( mProgram )
    delete mProgram;
}

bool FilterEngine::loadConfiguration( const QString &filterProgramFile )
{
  //Akonadi::Filter::IO::SieveDecoder d( f );
  //Akonadi::Filter::Program * p = d.run();

  return false;
}


