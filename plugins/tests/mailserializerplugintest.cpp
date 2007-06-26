/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "mailserializerplugintest.h"

#include <libakonadi/item.h>
#include <libakonadi/itemserializer.h>
#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KMime::Message> MessagePtr;

#include <qtest_kde.h>

QTEST_KDEMAIN( MailSerializerPluginTest, NoGUI )

using namespace Akonadi;
using namespace KMime;

void MailSerializerPluginTest::testMailPlugin()
{
  QByteArray serialized =
      "From: sender@test.org\n"
      "Subject: Serializer Test\n"
      "To: receiver@test.org\n"
      "Date: Fri, 22 Jun 2007 17:24:24 +0000\n"
      "MIME-Version: 1.0\n"
      "\n"
      "Body data.";

  // deserializing
  Item item;
  item.setMimeType( "message/rfc822" );
  ItemSerializer::deserialize( item, Item::PartBody, serialized );

  QVERIFY( item.hasPayload<MessagePtr>() );
  MessagePtr msg = item.payload<MessagePtr>();
  QVERIFY( msg != 0 );

  QCOMPARE( msg->to()->asUnicodeString(), QString( "receiver@test.org" ) );
  QCOMPARE( msg->body(), QByteArray( "Body data." ) );

  // serializing
  QByteArray data;
  ItemSerializer::serialize( item, Item::PartBody, data );
  QCOMPARE( data, serialized );
}

#include "mailserializerplugintest.moc"
