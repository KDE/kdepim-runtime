/****************************************************************************** *
 *
 *  File : agent.h
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

#ifndef _AKONADI_FILTER_AGENT_H_
#define _AKONADI_FILTER_AGENT_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <akonadi/collection.h>

#include <QtCore/QMetaType>
#include <QtCore/QList>
#include <QtCore/QString>

/**
 * This type is used in the agent D-BUS methods.
 * In xml it is encoded as... (what?)
 */
Q_DECLARE_METATYPE( QList< Akonadi::Collection::Id > );

namespace Akonadi
{
namespace Filter
{

/**
 * This namespace contains definitions useful in the implementation
 * of the filtering agent.
 */
namespace Agent
{

/**
 * This enumeration is encoded in the integer return values of some
 * agent methods (since D-BUS doesn't allow for definition of constants
 * we need this method of encoding).
 */
enum Status
{
  /**
   * The operation succeeded.
   */
  Success,

  /**
   * The method failed because one of the parameters specified was invalid.
   * This return value is used when no specific error is defined for the
   * parameter.
   */
  ErrorInvalidParameter,

  /**
   * The method failed because the specified filter already exists.
   */
  ErrorFilterAlreadyExists,

  /**
   * The method failed because filtering of the specified mimetype is not supported.
   */
  ErrorInvalidMimeType,

  /**
   * The method failed because the source of the filter contained a syntax error.
   * If you want to know more about the syntax error then pass your filter source
   * to an instance of Akonadi::Filter::IO::SieveDecoder and check lastError().
   */
  ErrorFilterSyntaxError,

  /**
   * The method failed because there is no such filter in the agent (a filter
   * with the specified identifier was not found).
   */
  ErrorNoSuchFilter,

  /**
   * This is returned from the several methods in the agent that process
   * lists of collections. Mainly attachFilter(), detachFilter() and changeFilter().
   * It means that some of the collections specified in the call were invalid
   * and could not be attached/detached.
   */
  ErrorNotAllCollectionsProcessed
};

/**
 * You should call this function before offering or accessing
 * the agent interfaces.
 */
void AKONADI_FILTER_EXPORT registerMetaTypes();

/**
 * Returns a descriptive string for the specified status code.
 */
QString AKONADI_FILTER_EXPORT statusDescription( Status status );

} // namespace Agent

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_AGENT_H_
