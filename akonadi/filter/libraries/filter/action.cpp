/****************************************************************************** *
 *
 *  File : action.cpp
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

#include "action.h"

namespace Akonadi
{
namespace Filter 
{
namespace Action
{

Base::Base( Component * parent )
  : Component( ComponentTypeAction, parent )
{
  Q_ASSERT( parent );
}

Base::~Base()
{
}

Base::ProcessingStatus Base::execute( Data * data )
{
  return SuccessAndContinue;
}

} // namespace Action

} // namespace Filter

} // namespace Akonadi

