/****************************************************************************** *
 *
 *  File : config-akonadi-filter-ui.h
 *  Created on Fri 15 May 2009 04:53:16 by Szymon Tomasz Stefanek
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

#ifndef _CONFIG_AKONADI_FILTER_UI_H_
#define _CONFIG_AKONADI_FILTER_UI_H_

#include <kdemacros.h>

#ifndef AKONADI_FILTER_UI_EXPORT
  #ifdef BUILD_AKONADI_FILTER_UI_EXPORT_LIBRARY
    // We are building this library
    #define AKONADI_FILTER_UI_EXPORT KDE_EXPORT
  #else //!BUILD_AKONADI_FILTER_UI_EXPORT_LIBRARY
    // We are using this library
    #define AKONADI_FILTER_UI_EXPORT KDE_IMPORT
  #endif //!BUILD_AKONADI_FILTER_UI_EXPORT_LIBRARY
#endif //!AKONADI_FILTER_UI_EXPORT

#endif //!_CONFIG_AKONADI_FILTER_UI_H_
