/*
    Copyright (C) 2009 Omat Holding B.V. <info@omat.nl>

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

#ifndef __STATUSITEM_H__
#define __STATUSITEM_H__

#include <QSharedDataPointer>
#include <QByteArray>
#include <QHash>
#include <QDateTime>

class StatusItem
{
public:
    StatusItem();
    StatusItem( const QByteArray& );
    StatusItem( const StatusItem& );
    ~StatusItem();
    StatusItem operator=( const StatusItem& );
    void setData( const QByteArray& );
    qlonglong id() const;
    QString value( const QString& ) const;
    QStringList keys() const;
    QString text() const;
    QDateTime date() const;
    QByteArray data() const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};

#endif
