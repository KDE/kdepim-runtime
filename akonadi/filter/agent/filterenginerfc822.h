/******************************************************************************
 *
 *  File : filterenginerfc822.h
 *  Created on Sat 20 Jun 2009 14:36:26 by Szymon Tomasz Stefanek
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

#ifndef _FILTERENGINERFC822_H_
#define _FILTERENGINERFC822_H_

#include "filterengine.h"

class FilterEngineRfc822 : public FilterEngine
{
public:

  /**
   * Create an instance of a message/rfc822 filtering engine.
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
  FilterEngineRfc822(
      const QString &id,
      const QString &mimeType,
      const QString &source,
      Akonadi::Filter::Program * program
    );

  virtual ~FilterEngineRfc822();

protected:

  /**
   * Reimplemented from FilterEngine
   */
  virtual bool run( const Akonadi::Item &item, const Akonadi::Collection &collection );
};

#endif //!_FILTERENGINERFC822_H_
