/****************************************************************************** *
 *
 *  File : agent.cpp
 *  Created on Sat 20 Jun 2009 03:30:16 by Szymon Tomasz Stefanek
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

#include <akonadi/filter/agent.h>

#include <QtDBus/QtDBus>

#include <KLocale>

namespace Akonadi
{
namespace Filter
{
namespace Agent
{

void registerMetaTypes()
{
  qDBusRegisterMetaType< QList< Collection::Id > >();
}

QString statusDescription( Status status )
{
  switch( status )
  {
    case Success:
      return i18n( "No error" );
    break;
    case ErrorInvalidParameter:
      return i18n( "One of the specified parameters was invalid" );
    break;
    case ErrorFilterAlreadyExists:
      return i18n( "The specified filter already exists" );
    break;
    case ErrorInvalidMimeType:
      return i18n( "Filtering of the specified mimetype is not supported" );
    break;
    case ErrorFilterSyntaxError:
      return i18n( "Syntax error in filter" );
    break;
    case ErrorNoSuchFilter:
      return i18n( "The specified filter doesn't exist" );
    break;
    case ErrorNotAllCollectionsProcessed:
      return i18n( "Not all the specified collections could be attached or detached" );
    break;
  }

  Q_ASSERT_X( false, __FUNCTION__, "You forgot to handle some Status code here" );

  return i18n( "Unknown status code" );
}

} // namespace Agent

} // namespace Filter

} // namespace Akonadi

