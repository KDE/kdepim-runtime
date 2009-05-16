/****************************************************************************** *
 *
 *  File : program.h
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

#ifndef _AKONADI_FILTER_PROGRAM_H_
#define _AKONADI_FILTER_PROGRAM_H_

#include "config-akonadi-filter.h"

#include <QList>
#include <QString>

#include "component.h"
#include "rulelist.h"
#include "data.h"

namespace Akonadi
{
namespace Filter
{

/**
 * A complete filtering program.
 *
 * The program is made up of a list of rules that are applied in sequence.
 */
class AKONADI_FILTER_EXPORT Program : public Action::RuleList
{
public:

  /**
   * Creates an empty filtering program.
   */
  Program( Component * parent );

  /**
   * Destroys the filtering program including any included rules.
   */
  virtual ~Program();

public:

  /**
   * The user supplied description of this filtering program.
   */
  QString mDescription;

public:

  /**
   * Returns the user supplied description attacched to this program.
   */
  const QString & description() const
  {
    return mDescription;
  }

  /**
   * Sets the user supplied description of this filtering program.
   */
  void setDescription( const QString &description )
  {
    mDescription = description;
  }

  virtual bool isProgram() const;


  virtual void dump( const QString &prefix );
};

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_PROGRAM_H_
