/******************************************************************************
 *
 *  File : filterenginerfc822.cpp
 *  Created on Sat 20 Jun 2009 14:36:26 by Szymon Tomasz Stefanek
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

#include "filterenginerfc822.h"
#include "datarfc822.h"

#include <akonadi/filter/program.h>


FilterEngineRfc822::FilterEngineRfc822(
   const QString &id,
   const QString &mimeType,
   const QString &source,
   Akonadi::Filter::Program * program
 )
  : FilterEngine( id, mimeType, source, program )
{
}

FilterEngineRfc822::~FilterEngineRfc822()
{
}

bool FilterEngineRfc822::run( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  DataRfc822 data( item, collection );

  switch( program()->execute( &data ) )
  {
    case Akonadi::Filter::Program::SuccessAndContinue:
      return true; // continue processing
    break;
    case Akonadi::Filter::Program::SuccessAndStop:
      return false; // stop processing
    break;
    case Akonadi::Filter::Program::Failure:
      // filter failed, but continue processing
      return true; // continue processing anyway
    break;
    default:
      Q_ASSERT_X( false, __FUNCTION__, "You forgot to handle a Program::execute() result, or it returned something unexpected anyway" );
    break;
  }

  return true; // continue processing
}

