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

#include <akonadi/filter/config-akonadi-filter.h>

#include <QtCore/QString>

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

public:
  Component( Component * parent );
  virtual ~Component();

protected:
  QString mLastError;
  Component * mParent;
public:

  Component * parent() const
  {
    return mParent;
  }

  virtual bool isProgram() const;
  virtual bool isRule() const;
  virtual bool isAction() const;
  virtual bool isCondition() const;
  virtual bool isRuleList() const;

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

  virtual void dump( const QString &prefix );
  void debugOutput( const QString &prefix, const char *text );
  void debugOutput( const QString &prefix, const QString &text );
};

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_COMPONENT_H_
