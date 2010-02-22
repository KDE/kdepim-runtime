/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef DAVPROTOCOLATTRIBUTE_H
#define DAVPROTOCOLATTRIBUTE_H

#include <akonadi/attribute.h>

#include <QtCore/QString>

class DavProtocolAttribute : public Akonadi::Attribute
{
  public:
    DavProtocolAttribute( int protocol = 0 );

    void setDavProtocol( int protocol );
    int davProtocol() const;

    virtual Akonadi::Attribute* clone() const;
    virtual QByteArray type() const;
    virtual QByteArray serialized() const;
    virtual void deserialize( const QByteArray &data );

  private:
    int mDavProtocol;
};

#endif
