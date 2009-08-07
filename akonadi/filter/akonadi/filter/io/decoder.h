/****************************************************************************** *
 *
 *  File : decoder.h
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

#ifndef _AKONADI_FILTER_IO_DECODER_H_
#define _AKONADI_FILTER_IO_DECODER_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <akonadi/filter/errorstack.h>

#include <QtCore/QString>

namespace Akonadi
{
namespace Filter
{

class ComponentFactory;
class Program;

namespace IO
{

/**
 * The base class of all filter Decoders.
 *
 * The Decoders parse a QByteArray (which may contain either
 * text or binary data) and return a complete filtering Program.
 *
 * The run() method is pure virtual and must be overridden
 * in derived classes in order to provide the implementation of
 * the decoding process.
 *
 * The Decoder objects need a ComponentFactory which will create
 * the filter components for them. The componentFactory pointer
 * must not be null.
 */
class AKONADI_FILTER_EXPORT Decoder
{
public:

  /**
   * Create a Decoder object attached to the specified ComponentFactory
   * implementation. The ComponentFactory pointer ownership is NOT
   * transferred and the caller must make sure that it's valid within
   * the lifetime of the Decoder.
   */
  Decoder( ComponentFactory * componentFactory );

  /**
   * Destroys the Decoder object.
   */
  virtual ~Decoder();

protected:

  /**
   * The ComponentFactory that this Decoder uses to create
   * the instances of the filtering program parts.
   */
  ComponentFactory * mComponentFactory;

  /**
   * The stack of errors encountered during the decoding process.
   */
  ErrorStack mErrorStack;

public:

  /**
   * Returns the ComponentFactory that this Decoder is using.
   */
  ComponentFactory * componentFactory() const
  {
    return mComponentFactory;
  }

  /**
   * Returns the stack of errors encountered during the decoding process.
   */
  ErrorStack & errorStack()
  {
    return mErrorStack;
  }

  /**
   * This method must be overridden by subclasses in order
   * to provide an implementation of the decoding process.
   *
   * The method should parse the encodedFilter and upon
   * succesfull execution should return a complete and valid
   * filtering Program object (created by the ComponentFactory).
   *
   * In case of error the method should return 0 and a description
   * of the error should be pushed on the error stack (which
   * is a base class of the Decoder) and thus be available
   * to the user via errorStack().
   */
  virtual Program * run( const QByteArray &encodedFilter ) = 0;

}; // class Decoder

} // namespace IO

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_IO_DECODER_H_
