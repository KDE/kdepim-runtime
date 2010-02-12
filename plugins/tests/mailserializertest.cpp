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
  serializer->deserialize( i, MessagePart::Envelope, buffer, 0 );
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
  int version = 0;
  serializer->serialize( i, MessagePart::Envelope, buffer, version );
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

void MailSerializerTest::testHeaderFetch()
{
  Item i;
  i.setMimeType( "message/rfc822" );

  SerializerPluginMail *serializer = new SerializerPluginMail();


  QByteArray headerData( "From: David Johnson <david@usermode.org>\n"
                         "To: kde-commits@kde.org\n"
                         "MIME-Version: 1.0\n"
                         "Date: Sun, 01 Feb 2009 06:25:22 +0000\n"
                         "Message-Id: <1233469522.741324.18468.nullmailer@svn.kde.org>\n"
                         "Subject: [kde-doc-english] KDE/kdeutils/kcalc\n" );

  QString expectedSubject = QString::fromUtf8( "[kde-doc-english] KDE/kdeutils/kcalc" );
  QString expectedFrom = QString::fromUtf8( "David Johnson <david@usermode.org>" );
  QString expectedTo = QString::fromUtf8( "kde-commits@kde.org" );

  // envelope
  QBuffer buffer;
  buffer.setData( headerData );
  buffer.open( QIODevice::ReadOnly );
  buffer.seek( 0 );
  serializer->deserialize( i, MessagePart::Header, buffer, 0 );
  QVERIFY( i.hasPayload<MessagePtr>() );

  MessagePtr msg = i.payload<MessagePtr>();
  QCOMPARE( msg->subject()->asUnicodeString(), expectedSubject );
  QCOMPARE( msg->from()->asUnicodeString(), expectedFrom );
  QCOMPARE( msg->to()->asUnicodeString(), expectedTo );

  delete serializer;
}

void MailSerializerTest::testMultiDeserialize()
{
  // The Body part includes the Header.
  // When serialization is done a second time, we should already have the header deserialized.
  // We change the header data for the second deserialization (which is an unrealistic scenario)
  // to demonstrate that it is not deserialized again.

  Item i;
  i.setMimeType( "message/rfc822" );

  SerializerPluginMail *serializer = new SerializerPluginMail();


  QByteArray messageData( "From: David Johnson <david@usermode.org>\n"
                          "To: kde-commits@kde.org\n"
                          "MIME-Version: 1.0\n"
                          "Date: Sun, 01 Feb 2009 06:25:22 +0000\n"
                          "Subject: [kde-doc-english] KDE/kdeutils/kcalc\n"
                          "Content-Type: text/plain\n"
                          "\n"
                          "This is content" );

  QString expectedSubject = QString::fromUtf8( "[kde-doc-english] KDE/kdeutils/kcalc" );
  QString expectedFrom = QString::fromUtf8( "David Johnson <david@usermode.org>" );
  QString expectedTo = QString::fromUtf8( "kde-commits@kde.org" );
  QByteArray expectedBody( "This is content" );

  // envelope
  QBuffer buffer;
  buffer.setData( messageData );
  buffer.open( QIODevice::ReadOnly );
  buffer.seek( 0 );
  serializer->deserialize( i, MessagePart::Body, buffer, 0 );
  QVERIFY( i.hasPayload<MessagePtr>() );

  MessagePtr msg = i.payload<MessagePtr>();
  QCOMPARE( msg->subject()->asUnicodeString(), expectedSubject );
  QCOMPARE( msg->from()->asUnicodeString(), expectedFrom );
  QCOMPARE( msg->to()->asUnicodeString(), expectedTo );
  QCOMPARE( msg->body(), expectedBody );

  buffer.close();

  messageData = QByteArray ( "From: DIFFERENT CONTACT <DIFFERENTCONTACT@example.org>\n"
                             "To: kde-commits@kde.org\n"
                             "MIME-Version: 1.0\n"
                             "Date: Sun, 01 Feb 2009 06:25:22 +0000\n"
                             "Message-Id: <1233469522.741324.18468.nullmailer@svn.kde.org>\n"
                             "Subject: [kde-doc-english] KDE/kdeutils/kcalc\n"
                             "Content-Type: text/plain\n"
                             "\r\n"
                             "This is content" );

  buffer.setData( messageData );
  buffer.open( QIODevice::ReadOnly );
  buffer.seek( 0 );

  serializer->deserialize( i, MessagePart::Header, buffer, 0 );
  QVERIFY( i.hasPayload<MessagePtr>() );

  msg = i.payload<MessagePtr>();
  QCOMPARE( msg->subject()->asUnicodeString(), expectedSubject );
  QCOMPARE( msg->from()->asUnicodeString(), expectedFrom );
  QCOMPARE( msg->to()->asUnicodeString(), expectedTo );

  delete serializer;
}


