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

#ifndef DAVITEM_H
#define DAVITEM_H

#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QList>
#include <QtCore/QString>

class DavItem
{
  public:
    typedef QList<DavItem> List;

    DavItem();
    DavItem( const QString &url, const QString &contentType, const QByteArray &data, const QString &etag );

    void setUrl( const QString &url );
    QString url() const;

    QString contentType() const;
    QByteArray data() const;

    void setEtag( const QString &etag );
    QString etag() const;

  private:
    QString mUrl;
    QString mContentType;
    QByteArray mData;
    QString mEtag;
};

QDataStream& operator<<( QDataStream &out, const DavItem &item );
QDataStream& operator>>( QDataStream &in, DavItem &item);

#endif
