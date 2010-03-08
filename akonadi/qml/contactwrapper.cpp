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

#include "contactwrapper.h"

#include <QPixmap>

DeclarativeContact::DeclarativeContact(const KABC::Addressee addressee, QObject* parent)
  : QObject(parent), m_addressee(addressee)
{

}

QString DeclarativeContact::name() const
{
  return m_addressee.name();
}

QString DeclarativeContact::email() const
{
  return m_addressee.preferredEmail();
}

QString DeclarativeContact::address() const
{
  return m_addressee.address( KABC::Address::Home ).formattedAddress();
}

QPixmap DeclarativeContact::image() const
{
  return QPixmap::fromImage( m_addressee.photo().data() );
}

QString DeclarativeContact::phone() const
{
  return m_addressee.phoneNumber(KABC::PhoneNumber::Home).number();
}

#include "contactwrapper.moc"
