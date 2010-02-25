/*
* This file is part of Akonadi
*
* Copyright 2010 Stephen Kelly <steveire@gmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#include "eventwrapper.h"

#include <QPixmap>

#include <kdebug.h>

EventWrapper::EventWrapper(KCal::Incidence::Ptr incidence, QObject* parent)
  : QObject(parent), m_incidence(incidence)
{

}

KCal::Incidence::Ptr EventWrapper::incidence() const
{
  return m_incidence;
}

QString EventWrapper::summary() const
{
  return m_incidence->summary();
}

QString EventWrapper::location() const
{
  return m_incidence->location();
}

QPixmap EventWrapper::image() const
{
  return QPixmap();
}

QString EventWrapper::description() const
{
  return m_incidence->description();
}

