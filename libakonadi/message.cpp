/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "message.h"

#include <kmime/kmime_message.h>

using namespace Akonadi;

class Akonadi::MessagePrivate
{
  public:
    KMime::Message *mime;
};


Message::Message( const DataReference & ref ) :
    Item( ref ),
    d( new MessagePrivate() )
{
}

Message::~Message( )
{
  delete d->mime;
  delete d;
}

KMime::Message * Message::mime( ) const
{
  return d->mime;
}

void Message::setMime( KMime::Message * mime )
{
  d->mime = mime;
}
