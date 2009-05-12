/****************************************************************************** *
 *
 *  File : component.h
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

#ifndef _AKONADI_FILTER_COMPONENT_H_
#define _AKONADI_FILTER_COMPONENT_H_

#include "config-akonadi-filter.h"

#include <QString>

namespace Akonadi
{
namespace Filter
{

class AKONADI_FILTER_EXPORT Component
{
public:
  enum ProcessingStatus
  {
    SuccessAndContinue,
    SuccessAndStop,
    Failure
  };

  enum ComponentType
  {
    ComponentTypeProgram,
    ComponentTypeRule,
    ComponentTypeCondition,
    ComponentTypeAction
  };
public:
  Component( ComponentType type, Component * parent );
  virtual ~Component();

protected:
  QString mLastError;
  ComponentType mComponentType;
  Component * mParent;
public:

  Component * parent() const
  {
    return mParent;
  }

  ComponentType componentType() const
  {
    return mComponentType;
  }

  bool isProgram() const
  {
    return mComponentType == ComponentTypeProgram;
  }

  bool isRule() const
  {
    return mComponentType == ComponentTypeRule;
  }

  bool isAction() const
  {
    return mComponentType == ComponentTypeAction;
  }

  bool isCondition() const
  {
    return mComponentType == ComponentTypeCondition;
  }

  /**
   * Returns the last error occured in this component execution run.
   */
  const QString & lastError() const
  {
    return mLastError;
  }

  void setLastError( const QString &error )
  {
    mLastError = error;
  }

};

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_COMPONENT_H_
