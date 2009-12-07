/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "incidenceutils.h"

#include "davutils.h"
#include "oxutils.h"

#include <kcal/event.h>
#include <kcal/todo.h>

#include <QtXml/QDomElement>

using namespace OXA;

void OXA::IncidenceUtils::parseEvent( const QDomElement &propElement, Object &object )
{
  KCal::Event *event = new KCal::Event;

  QDomElement element = propElement.firstChildElement();
  while ( !element.isNull() ) {
    element = element.nextSiblingElement();
  }

  object.setEvent( KCal::Incidence::Ptr( event ) );
}

void OXA::IncidenceUtils::parseTask( const QDomElement &propElement, Object &object )
{
  KCal::Todo *todo = new KCal::Todo;

  QDomElement element = propElement.firstChildElement();
  while ( !element.isNull() ) {

    element = element.nextSiblingElement();
  }

  object.setTask( KCal::Incidence::Ptr( todo ) );
}

void OXA::IncidenceUtils::addEventElements( QDomDocument &document, QDomElement &propElement, const Object &object )
{
}

void OXA::IncidenceUtils::addTaskElements( QDomDocument &document, QDomElement &propElement, const Object &object )
{
}
