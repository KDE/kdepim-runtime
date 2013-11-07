/*
    Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include <akonadi/item.h>
#include <qtest_kde.h>
#include <QtCore/QObject>
#include <akonadi_serializer_addressee.h>

using namespace Akonadi;

class AddresseeSerializerTest : public QObject
{
    Q_OBJECT
    private slots:
        void testGid()
        {
            const QString uid(QLatin1String("uid"));
            KABC::Addressee addressee;
            addressee.setUid(uid);
            Akonadi::Item item;
            item.setMimeType(addressee.mimeType());
            item.setPayload(addressee);
            SerializerPluginAddressee plugin;
            const QString gid = plugin.extractGid(item);
            QCOMPARE(gid, uid);
        }
};

QTEST_KDEMAIN( AddresseeSerializerTest, NoGUI )

#include "addresseeserializertest.moc"
