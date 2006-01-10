/*
    This file is part of libakonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
                  2006 Marc Mutz <mutz@kde.org>

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

#include "datarequest.h"

using namespace PIM;

DataRequest::DataRequest( const DataReference::List &references, const QString &part,
                          const QStringList &acceptableMimeTypes )
{
}

DataRequest::~DataRequest()
{
}

int DataRequest::count() const
{
}

QByteArray DataRequest::data( int index ) const
{
}

QByteArray DataRequest::mimeType( int index ) const
{
}

void DataRequest::doStart()
{
}

#include "datarequest.moc"
