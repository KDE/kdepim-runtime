/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

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


#ifndef __AKONADI_SERIALIZER_MAIL_H__
#define __AKONADI_SERIALIZER_MAIL_H__

#include <QtCore/QObject>
#include <QtCore/QMutex>

#include <akonadi/itemserializerplugin.h>

namespace Akonadi {

/**
 * Levare QString implicit sharing to decrease memory consumption.
 *
 * This class is thread safe. Apparenlty required for usage in
 * legacy KRes compat bridges.
 */
class StringPool
{
public:
    /**
     * Lookup @p value in the pool and return the known value
     * to reuse it and leverage the implicit sharing. Otherwise
     * add the value to the pool and return it again.
     */
    QString sharedValue(const QString& value);
private:
    QMutex m_mutex;
    QSet<QString> m_pool;
};

class SerializerPluginMail : public QObject, public ItemSerializerPlugin
{
    Q_OBJECT
    Q_INTERFACES( Akonadi::ItemSerializerPlugin )

public:
    bool deserialize( Item& item, const QByteArray& label, QIODevice& data, int version );
    void serialize( const Item& item, const QByteArray& label, QIODevice& data, int &version );
    QSet<QByteArray> parts( const Item &item ) const;
private:
    StringPool m_stringPool;
};


}

#endif
