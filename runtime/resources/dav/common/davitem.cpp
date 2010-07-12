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

#include "davitem.h"

DavItem::DavItem()
{
}

DavItem::DavItem( const QString &url, const QString &contentType, const QByteArray &data, const QString &etag )
  : mUrl( url ), mContentType( contentType ), mData( data ), mEtag( etag )
{
}

void DavItem::setUrl( const QString &url )
{
  mUrl = url;
}

QString DavItem::url() const
{
  return mUrl;
}

void DavItem::setContentType( const QString &contentType )
{
  mContentType = contentType;
}

QString DavItem::contentType() const
{
  return mContentType;
}

void DavItem::setData( const QByteArray &data )
{
  mData = data;
}

QByteArray DavItem::data() const
{
  return mData;
}

void DavItem::setEtag( const QString &etag )
{
  mEtag = etag;
}

QString DavItem::etag() const
{
  return mEtag;
}

QDataStream& operator<<( QDataStream &stream, const DavItem &item )
{
  stream << item.url();
  stream << item.contentType();
  stream << item.data();
  stream << item.etag();

  return stream;
}

QDataStream& operator>>( QDataStream &stream, DavItem &item )
{
  QString url, contentType, etag;
  QByteArray data;

  stream >> url;
  stream >> contentType;
  stream >> data;
  stream >> etag;

  item = DavItem( url, contentType, data, etag );

  return stream;
}
