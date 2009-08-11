/****************************************************************************** *
 *
 *  File : encoder.h
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

#ifndef _AKONADI_FILTER_IO_ENCODER_H_
#define _AKONADI_FILTER_IO_ENCODER_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <akonadi/filter/errorstack.h>

#include <QtCore/QString>

namespace Akonadi
{
namespace Filter
{

class Program;

/**
 * @namespace Akonadi::Filter::IO
 * @brief Filter storage I/O classes and routines.
 */
namespace IO
{

/**
 * The base class of all filter Encoders.
 *
 * The Encoder instances take a filtering Program and convert
 * it to some rappresentation stored inside a QByteArray.
 *
 * The run() method is pure virtual and must be overridden
 * in derived classes in order to provide the implementation of
 * the encoding process.
 */
class AKONADI_FILTER_EXPORT Encoder : public ErrorStack
{
public:

  /**
   * Create an Encoder instance
   */
  Encoder();

  /**
   * Kill an Encoder instance and all the associated resources.
   */
  virtual ~Encoder();

protected:

  /**
   * The stack of errors encountered during the encoding process.
   */
  ErrorStack mErrorStack;

public:

  /**
   * Returns the stack of errors encountered during the encoding process.
   */
  ErrorStack & errorStack()
  {
    return mErrorStack;
  }

  /**
   * This method must be overridden by subclasses in order
   * to provide an implementation of the encoding process.
   *
   * The method should serialize the Program into a QByteArray
   * which should be at least 1 byte long.
   *
   * In case of error the method should return a null QByteArray
   * and a description of the error should be avaiable via errorStack().
   */
  virtual QByteArray run( Program * program ) = 0;

}; // class Encoder

} // namespace IO

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_IO_ENCODER_H_
