/******************************************************************************
 *
 *  File : engine.h
 *  Created on Mon 8 Jun 2009 04:17:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Mail Filtering Agent
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

#ifndef _AKONADI_FILTER_ENGINE_H_
#define _AKONADI_FILTER_ENGINE_H_

#include <QString>

namespace Akonadi
{
namespace Filter
{
  class ComponentFactory;
  class Program;
} // namespace Filter
} // namespace Akonadi

/**
 * A single mail filtering engine.
 *
 * This one executes a single filtering program (so has a single
 * configuration file). It can be attacched to multiple collections though.
 */
class FilterEngine
{
public:

  /**
   * Create an instance of a filtering engine with the specified
   * unique engine id.
   */
  FilterEngine(
      const QString &id,
      Akonadi::Filter::ComponentFactory * componentFactory
    );

  ~FilterEngine();

protected:

  /**
   * The unique id of this filtering engine.
   */
  QString mId;

  /**
   * The filtering component factory we use to create our program objects.
   */
  Akonadi::Filter::ComponentFactory * mComponentFactory;

  /**
   * The program this engine executes.
   */
  Akonadi::Filter::Program * mProgram;

public:

  /**
   * Returns the unique id of this filtering engine.
   */
  const QString & id() const
  {
    return mId;
  }

  bool loadConfiguration( const QString &filterProgramFile );
};

#endif //!_AKONADI_FILTER_ENGINE_H_
