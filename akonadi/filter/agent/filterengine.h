/******************************************************************************
 *
 *  File : engine.h
 *  Created on Mon 8 Jun 2009 04:17:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filtering Agent
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

#ifndef _FILTERENGINE_H_
#define _FILTERENGINE_H_

#include <QtCore/QString>

#include <akonadi/item.h>

#include <akonadi/filter/errorstack.h>

namespace Akonadi
{
namespace Filter
{
  class Program;
} // namespace Filter
} // namespace Akonadi

/**
 * A single filtering engine descriptor.
 *
 * This object contains all the informations about a single filter (which can be attached
 * to multiple collections by the FilterAgent.
 *
 * Each engine has a runtime id, which must be unique inside the agent.
 * Each engine has a mimetype, which determines the ComponentFactory used
 * to encode/decode the program and the Filter::Data subclass that is used
 * for Program execution.
 * Each engine has a source code for the program, in sieve format.
 * Each engine has a filtering program: the real tree of filtering instructions.
 */
class FilterEngine
{
public:

  /**
   * Create an instance of a filtering engine.
   *
   * \param id
   *   The unique id of the engine
   * \param mimeType
   *   The mimetype associated to the filtering program
   * \param source
   *   The sieve source code of the program
   * \param program
   *   The program decoded from the sieve source: the engine takes the ownership of this pointer
   */
  FilterEngine(
      const QString &id,
      const QString &mimeType,
      const QString &source,
      Akonadi::Filter::Program * program
    );

  virtual ~FilterEngine();

protected:

  /**
   * The unique id of this filtering engine.
   */
  QString mId;

  /**
   * The mimetype this engine is associated to.
   */
  QString mMimeType;

  /**
   * The sieve source code of the filtering program this engine executes
   */
  QString mSource;

  /**
   * The program this engine executes. Owned pointer.
   */
  Akonadi::Filter::Program * mProgram;

  /**
   * The stack of errors generated in a run() call.
   */
  Akonadi::Filter::ErrorStack mErrorStack;

public:

  /**
   * Returns the unique id of this filtering engine.
   */
  const QString & id() const
  {
    return mId;
  }

  /**
   * Returns the mimetype associated to this filtering engine.
   */
  const QString & mimeType() const
  {
    return mMimeType;
  }

  /**
   * Returns the source code of the filtering program this engine executes.
   */
  const QString & source() const
  {
    return mSource;
  }

  /**
   * Returns the name of the program stored in the engine
   * or the id string if the program is null or its name is empty.
   * This is useful for displaying messages to the user.
   */
  QString nameOrId() const;

  /**
   * Returns the (read only) stack of errors generated during a run() call
   */
  Akonadi::Filter::ErrorStack & errorStack()
  {
    return mErrorStack;
  }


  /**
   * Sets the sieve source of the filtering program this engine executes.
   * You *MUST* call setProgram() with the corresponding Akonadi::Filter::Program
   * object in order to preserve coherency.
   */
  void setSource( const QString &source )
  {
    mSource = source;
  }

  /**
   * Sets the filtering program this engine will execute.
   * You *MUST* call setSource() with the corresponding sieve source code
   * in order to preserve coherency.
   */
  void setProgram( Akonadi::Filter::Program * program );

  /**
   * Returns the filtering program set by setProgram()
   */
  Akonadi::Filter::Program * program()
  {
    return mProgram;
  }

  /**
   * This must be reimplemented by subclasses to handle item processing.
   * The method should return true if processing should continue (and eventually
   * other filter engines should be applied) or false if processing should stop.
   */
  virtual bool run( const Akonadi::Item &item ) = 0;

};

#endif //!_FILTERENGINE_H_
