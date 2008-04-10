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

#include "mailserializertest.h"
#include "mailserializertest.moc"

#include "akonadi_serializer_mail.cpp"

#include <akonadi/kmime/messageparts.h>

#include <qtest_kde.h>

QTEST_KDEMAIN( MailSerializerTest, NoGUI )

typedef boost::shared_ptr<KMime::Message> MessagePtr;

void MailSerializerTest::testEnvelopeDeserialize()
{
  Item i;
  i.setMimeType( "message/rfc822" );

  SerializerPluginMail *serializer = new SerializerPluginMail();

  // envelope
  QByteArray env( "(\"Wed, 1 Feb 2006 13:37:19 UT\" \"IMPORTANT: Akonadi Test\" ((\"Tobias Koenig\" NIL \"tokoe\" \"kde.org\")) ((\"Tobias Koenig\" NIL \"tokoe\" \"kde.org\")) NIL ((\"Ingo Kloecker\" NIL \"kloecker\" \"kde.org\")) NIL NIL NIL <{7b55527e-77f4-489d-bf18-e805be96718c}@server.kde.org>)" );
  QBuffer buffer;
  buffer.setData( env );
  buffer.open( QIODevice::ReadOnly );
  buffer.seek( 0 );
  serializer->deserialize( i, MessagePart::Envelope, buffer );
  QVERIFY( i.hasPayload<MessagePtr>() );

  MessagePtr msg = i.payload<MessagePtr>();
  QCOMPARE( msg->subject()->asUnicodeString(), QString::fromUtf8( "IMPORTANT: Akonadi Test" ) );
  QCOMPARE( msg->from()->asUnicodeString(), QString::fromUtf8( "Tobias Koenig <tokoe@kde.org>" ) );
  QCOMPARE( msg->to()->asUnicodeString(), QString::fromUtf8( "Ingo Kloecker <kloecker@kde.org>" ) );

  delete serializer;
}

void MailSerializerTest::testEnvelopeSerialize()
{
  Item i;
  i.setMimeType( "message/rfc822" );
  Message* msg = new Message();
  msg->date()->from7BitString( "Wed, 1 Feb 2006 13:37:19 UT" );
  msg->subject()->from7BitString( "IMPORTANT: Akonadi Test" );
  msg->from()->from7BitString( "Tobias Koenig <tokoe@kde.org>" );
  msg->sender()->from7BitString( "Tobias Koenig <tokoe@kde.org>" );
  msg->to()->from7BitString( "Ingo Kloecker <kloecker@kde.org>" );
  msg->messageID()->from7BitString( "<{7b55527e-77f4-489d-bf18-e805be96718c}@server.kde.org>" );
  i.setPayload( MessagePtr( msg ) );

  SerializerPluginMail *serializer = new SerializerPluginMail();

  // envelope
  QByteArray expEnv( "(\"Wed, 01 Feb 2006 13:37:19 +0000\" \"IMPORTANT: Akonadi Test\" ((\"Tobias Koenig\" NIL \"tokoe\" \"kde.org\")) ((\"Tobias Koenig\" NIL \"tokoe\" \"kde.org\")) NIL ((\"Ingo Kloecker\" NIL \"kloecker\" \"kde.org\")) NIL NIL NIL \"<{7b55527e-77f4-489d-bf18-e805be96718c}@server.kde.org>\")" );
  QByteArray env;
  QBuffer buffer;
  buffer.setBuffer( &env );
  buffer.open( QIODevice::ReadWrite );
  buffer.seek( 0 );
  serializer->serialize( i, MessagePart::Envelope, buffer );
  QCOMPARE( env, expEnv );

  delete serializer;

}

void MailSerializerTest::testParts()
{
  Item item;
  item.setMimeType( "message/rfc822" );
  KMime::Message *m = new Message;
  MessagePtr msg( m );
  item.setPayload( msg );

  SerializerPluginMail *serializer = new SerializerPluginMail();
  QVERIFY( serializer->parts( item ).isEmpty() );

  msg->setHead( "foo" );
  QSet<QByteArray> parts = serializer->parts( item );
  QCOMPARE( parts.count(), 2 );
  QVERIFY( parts.contains( MessagePart::Envelope ) );
  QVERIFY( parts.contains( MessagePart::Header ) );

  msg->setBody( "bar" );
  parts = serializer->parts( item );
  QCOMPARE( parts.count(), 3 );
  QVERIFY( parts.contains( MessagePart::Envelope ) );
  QVERIFY( parts.contains( MessagePart::Header ) );
  QVERIFY( parts.contains( MessagePart::Body ) );

  delete serializer;
}


