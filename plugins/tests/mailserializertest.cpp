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
  serializer->deserialize( i, "ENVELOPE", env );
  QVERIFY( i.hasPayload<MessagePtr>() );

  MessagePtr msg = i.payload<MessagePtr>();
  QCOMPARE( msg->subject()->asUnicodeString(), QString::fromUtf8( "IMPORTANT: Akonadi Test" ) );
  QCOMPARE( msg->from()->asUnicodeString(), QString::fromUtf8( "Tobias Koenig <tokoe@kde.org>" ) );
  QCOMPARE( msg->to()->asUnicodeString(), QString::fromUtf8( "Ingo Kloecker <kloecker@kde.org>" ) );

  delete serializer;
}
