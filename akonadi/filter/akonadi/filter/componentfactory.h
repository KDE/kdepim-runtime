/****************************************************************************** *
 *
 *  File : componentfactory.h
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

#ifndef _AKONADI_FILTER_COMPONENTFACTORY_H_
#define _AKONADI_FILTER_COMPONENTFACTORY_H_

#include "config-akonadi-filter.h"

#include <QString>
#include <QList>
#include <QHash>

#include "attribute.h"

namespace Akonadi
{
namespace Filter
{

class Program;
class Rule;
class Component;

namespace Condition
{
  class Base;
  class And;
  class Or;
  class Not;
} // namespace Condition

namespace Action
{
  class Base;
  class Stop;
  class RuleList;
} // namespace Action

class AKONADI_FILTER_EXPORT ComponentFactory
{
public:
  ComponentFactory();
  virtual ~ComponentFactory();

private:
  QList< const Attribute * > mAttributeList;
  QHash< QString, Attribute * > mAttributeHash;

public:
  void registerAttribute( Attribute * attribute );

  virtual const Attribute * findAttribute( const QString &id );

  virtual const QList< const Attribute * > & enumerateAttributes();

  virtual Program * createProgram( Component * parent );

  virtual Rule * createRule( Component * parent );

  virtual Condition::And * createAndCondition( Component * parent );
  virtual Condition::Or * createOrCondition( Component * parent );
  virtual Condition::Not * createNotCondition( Component * parent );
  virtual Condition::Base * createAttributeTestCondition( Component * parent, const Attribute * attribute );

  virtual Action::Base * createGenericAction( Component * parent, const QString &name );
  virtual Action::RuleList * createRuleList( Component * parent );
  virtual Action::Stop * createStopAction( Component * parent );
}; // class ComponentFactory

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_COMPONENTFACTORY_H_
