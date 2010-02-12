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

#include <akonadi/item.h>
#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

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
      "Content-Type: text/plain\n"
      "\n"
      "Body data.";

  // deserializing
  Item item;
  item.setMimeType( "message/rfc822" );
  item.setPayloadFromData( serialized );

  QVERIFY( item.hasPayload<KMime::Message::Ptr>() );
  KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  QVERIFY( msg != 0 );

  QCOMPARE( msg->to()->asUnicodeString(), QString( "receiver@test.org" ) );
  QCOMPARE( msg->body(), QByteArray( "Body data." ) );

  // serializing
  QByteArray data = item.payloadData();
  QCOMPARE( data, serialized );
}

void MailSerializerPluginTest::testMessageIntegrity()
{
  // A message that will be slightly modified if KMime::Content::assemble() is
  // called.  We want to avoid this, because it breaks signatures.
  QByteArray serialized = 
    "from: sender@example.com\n"
    "to: receiver@example.com\n"
    "Subject: Serializer Test\n"
    "Date: Thu, 30 Jul 2009 13:46:31 +0300\n"
    "MIME-Version: 1.0\n"
    "Content-type: text/plain; charset=us-ascii\n"
    "\n"
    "Bla bla bla.";

  // Deserialize.
  Item item;
  item.setMimeType( "message/rfc822" );
  item.setPayloadFromData( serialized );

  QVERIFY( item.hasPayload<KMime::Message::Ptr>() );
  KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  QVERIFY( msg != 0 );

  kDebug() << "original data:" << serialized;
  kDebug() << "message content:" << msg->encodedContent();
  QCOMPARE( msg->encodedContent(), serialized );

  // Serialize.
  QByteArray data = item.payloadData();
  kDebug() << "original data:" << serialized;
  kDebug() << "serialized data:" << data;
  QCOMPARE( data, serialized );
}

#include "mailserializerplugintest.moc"
