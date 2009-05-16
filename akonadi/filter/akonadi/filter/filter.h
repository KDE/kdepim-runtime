/****************************************************************************** *
 *
 *  File : filter.h
 *  Created on Sun 03 May 2009 12:10:16 by Szymon Tomasz Stefanek
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

#ifndef _AKONADI_FILTER_H_
#define _AKONADI_FILTER_H_

#include "config-akonadi-filter.h"

#include <QString>

class QTextStream;

namespace Akonadi
{

#if 0
namespace Filter
{

  bool loadSieveScript( QTextStream &stream );
  bool saveSieveScript( QTextStream &stream );

  bool loadSieveScript( const QString &fileName );
  bool saveSieveScript( const QString &fileName );

} // namespace Filter
#endif

} // namespace Akonadi



#endif //!_AKONADI_FILTER_H_
