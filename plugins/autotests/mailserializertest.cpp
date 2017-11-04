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

#include "akonadi_serializer_mail.cpp"

#include <Akonadi/KMime/MessageParts>

#include <qtest.h>
#include <QBuffer>

QTEST_MAIN(MailSerializerTest)

void MailSerializerTest::testEnvelopeDeserialize_data()
{
    QTest::addColumn<int>("version");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QDateTime>("date");
    QTest::addColumn<QString>("subject");
    QTest::addColumn<QString>("from");
    QTest::addColumn<QString>("sender");
    QTest::addColumn<QString>("replyTo");
    QTest::addColumn<QString>("to");
    QTest::addColumn<QString>("cc");
    QTest::addColumn<QString>("bcc");
    QTest::addColumn<QString>("inReplyTo");
    QTest::addColumn<QString>("messageId");
    QTest::addColumn<QString>("references");

    QTest::newRow("v1 - no references")
        << 1
        << QByteArray(
        "(\"Wed, 1 Feb 2006 13:37:19 UT\" \"IMPORTANT: Akonadi Test\" ((\"Tobias Koenig\" NIL \"tokoe\" \"kde.org\")) ((\"Tobias Koenig\" NIL \"tokoe\" \"kde.org\")) NIL ((\"Ingo Kloecker\" NIL \"kloecker\" \"kde.org\")) NIL NIL NIL <{7b55527e-77f4-489d-bf18-e805be96718c}@server.kde.org>)")
        << QDateTime(QDate(2006, 2, 1), QTime(13, 37, 19), Qt::UTC)
        << QStringLiteral("IMPORTANT: Akonadi Test")
        << QStringLiteral("Tobias Koenig <tokoe@kde.org>")
        << QStringLiteral("Tobias Koenig <tokoe@kde.org>")
        << QString()
        << QStringLiteral("Ingo Kloecker <kloecker@kde.org>")
        << QString() << QString() << QString()
        << QStringLiteral("<{7b55527e-77f4-489d-bf18-e805be96718c}@server.kde.org>")
        << QString();

    QTest::newRow("v1 - with references")
        << 1
        << QByteArray(
        "(\"Wed, 1 Feb 2006 13:37:19 UT\" \"IMPORTANT: Akonadi Test\" ((\"Tobias Koenig\" NIL \"tokoe\" \"kde.org\")) ((\"Tobias Koenig\" NIL \"tokoe\" \"kde.org\")) NIL ((\"Ingo Kloecker\" NIL \"kloecker\" \"kde.org\")) NIL NIL NIL <{7b55527e-77f4-489d-bf18-e805be96718c}@server.kde.org> \"<{8888827e-77f4-489d-bf18-e805be96718c}@server.kde.org> <{9999927e-77f4-489d-bf18-e805be96718c}@server.kde.org>\")")
        << QDateTime(QDate(2006, 2, 1), QTime(13, 37, 19), Qt::UTC)
        << QStringLiteral("IMPORTANT: Akonadi Test")
        << QStringLiteral("Tobias Koenig <tokoe@kde.org>")
        << QStringLiteral("Tobias Koenig <tokoe@kde.org>")
        << QString()
        << QStringLiteral("Ingo Kloecker <kloecker@kde.org>")
        << QString()
        << QString()
        << QString()
        << QStringLiteral("<{7b55527e-77f4-489d-bf18-e805be96718c}@server.kde.org>")
        << QStringLiteral("<{8888827e-77f4-489d-bf18-e805be96718c}@server.kde.org> <{9999927e-77f4-489d-bf18-e805be96718c}@server.kde.org>");

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);
    stream << QDateTime(QDate(2015, 6, 29), QTime(23, 50, 10), Qt::UTC)
           << QStringLiteral("Důležité: Testování Akonadi")
           << QStringLiteral("<1234567.icameup@withthis>")
           << QStringLiteral("<2849719.IhmDc3qecs@thor>")
           << QStringLiteral("<1234567.icameup@withthis> <7654321.icameupwith@thistoo>")
           << 1 << QStringLiteral("Daniel Vrátil") << QStringLiteral("dvratil") << QStringLiteral("kde.org")
           << 1 << QStringLiteral("Daniel Vrátil") << QStringLiteral("dvratil") << QStringLiteral("kde.org")
           << 0
           << 1 << QStringLiteral("Volker Krause") << QStringLiteral("vkrause") << QStringLiteral("kde.org")
           << 0
           << 0;

    QTest::newRow("v2")
        << 2
        << buffer.data()
        << QDateTime(QDate(2015, 6, 29), QTime(23, 50, 10), Qt::UTC)
        << QStringLiteral("Důležité: Testování Akonadi")
        << QStringLiteral("Daniel Vrátil <dvratil@kde.org>")
        << QStringLiteral("Daniel Vrátil <dvratil@kde.org>")
        << QString()     // reply to
        << QStringLiteral("Volker Krause <vkrause@kde.org>")
        << QString()     // cc
        << QString()     // bcc
        << QStringLiteral("<1234567.icameup@withthis>")
        << QStringLiteral("<2849719.IhmDc3qecs@thor>")
        << QStringLiteral("<1234567.icameup@withthis> <7654321.icameupwith@thistoo>");
}

void MailSerializerTest::testEnvelopeDeserialize()
{
    QFETCH(int, version);
    QFETCH(QByteArray, data);
    QFETCH(QDateTime, date);
    QFETCH(QString, subject);
    QFETCH(QString, from);
    QFETCH(QString, sender);
    QFETCH(QString, replyTo);
    QFETCH(QString, to);
    QFETCH(QString, cc);
    QFETCH(QString, bcc);
    QFETCH(QString, inReplyTo);
    QFETCH(QString, messageId);
    QFETCH(QString, references);

    Item i;
    i.setMimeType(QStringLiteral("message/rfc822"));

    SerializerPluginMail *serializer = new SerializerPluginMail();

    // envelope
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);
    QBENCHMARK {
        buffer.seek(0);
        serializer->deserialize(i, MessagePart::Envelope, buffer, version);
    }
    QVERIFY(i.hasPayload<KMime::Message::Ptr>());

    KMime::Message::Ptr msg = i.payload<KMime::Message::Ptr>();
    QCOMPARE(msg->date()->dateTime(), date);
    QCOMPARE(msg->subject()->asUnicodeString(), subject);
    QCOMPARE(msg->from()->asUnicodeString(), from);
    QCOMPARE(msg->sender()->asUnicodeString(), sender);
    QCOMPARE(msg->replyTo()->asUnicodeString(), replyTo);
    QCOMPARE(msg->to()->asUnicodeString(), to);
    QCOMPARE(msg->cc()->asUnicodeString(), cc);
    QCOMPARE(msg->bcc()->asUnicodeString(), bcc);
    QCOMPARE(msg->inReplyTo()->asUnicodeString(), inReplyTo);
    QCOMPARE(msg->messageID()->asUnicodeString(), messageId);
    QCOMPARE(msg->references()->asUnicodeString(), references);

    delete serializer;
}

void MailSerializerTest::testEnvelopeSerialize_data()
{
    QTest::addColumn<QDateTime>("date");
    QTest::addColumn<QString>("subject");
    QTest::addColumn<QString>("from");
    QTest::addColumn<QString>("sender");
    QTest::addColumn<QString>("replyTo");
    QTest::addColumn<QString>("to");
    QTest::addColumn<QString>("cc");
    QTest::addColumn<QString>("bcc");
    QTest::addColumn<QString>("inReplyTo");
    QTest::addColumn<QString>("messageId");
    QTest::addColumn<QString>("references");
    QTest::addColumn<QByteArray>("data");

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);
    stream << QDateTime(QDate(2006, 2, 1), QTime(13, 37, 19), Qt::UTC)
           << QStringLiteral("IMPORTANT: Akonadi Test")
           << QString()
           << QStringLiteral("<{7b55527e-77f4-489d-bf18-e805be96718c}@server.kde.org>")
           << QString()
           << 1 << QStringLiteral("Tobias Koenig") << QStringLiteral("tokoe") << QStringLiteral("kde.org")
           << 1 << QStringLiteral("Tobias Koenig") << QStringLiteral("tokoe") << QStringLiteral("kde.org")
           << 0
           << 1 << QStringLiteral("Ingo Kloecker") << QStringLiteral("kloecker") << QStringLiteral("kde.org")
           << 0
           << 0;
    QTest::newRow("")
        << QDateTime(QDate(2006, 2, 1), QTime(13, 37, 19), Qt::UTC)
        << QStringLiteral("IMPORTANT: Akonadi Test")
        << QStringLiteral("Tobias Koenig <tokoe@kde.org>")
        << QStringLiteral("Tobias Koenig <tokoe@kde.org>")
        << QString()
        << QStringLiteral("Ingo Kloecker <kloecker@kde.org>")
        << QString()
        << QString()
        << QString()
        << QStringLiteral("<{7b55527e-77f4-489d-bf18-e805be96718c}@server.kde.org>")
        << QString()
        << buffer.data();
}

void MailSerializerTest::testEnvelopeSerialize()
{
    QFETCH(QDateTime, date);
    QFETCH(QString, subject);
    QFETCH(QString, from);
    QFETCH(QString, sender);
    QFETCH(QString, replyTo);
    QFETCH(QString, to);
    QFETCH(QString, cc);
    QFETCH(QString, bcc);
    QFETCH(QString, inReplyTo);
    QFETCH(QString, messageId);
    QFETCH(QString, references);
    QFETCH(QByteArray, data);

    Item i;
    i.setMimeType(QStringLiteral("message/rfc822"));
    Message *msg = new Message();
    msg->date()->setDateTime(date);
    msg->subject()->fromUnicodeString(subject, "UTF-8");
    msg->from()->fromUnicodeString(from, "UTF-8");
    msg->sender()->fromUnicodeString(sender, "UTF-8");
    msg->replyTo()->fromUnicodeString(replyTo, "UTF-8");
    msg->to()->fromUnicodeString(to, "UTF-8");
    msg->cc()->fromUnicodeString(cc, "UTF-8");
    msg->bcc()->fromUnicodeString(bcc, "UTF-8");
    msg->inReplyTo()->fromUnicodeString(inReplyTo, "UTF-8");
    msg->messageID()->fromUnicodeString(messageId, "UTF-8");
    msg->references()->fromUnicodeString(references, "UTF-8");
    i.setPayload(KMime::Message::Ptr(msg));

    SerializerPluginMail *serializer = new SerializerPluginMail();

    // envelope
    QByteArray env;
    QBuffer buffer;
    buffer.setBuffer(&env);
    buffer.open(QIODevice::ReadWrite);
    int version = 0;
    QBENCHMARK {
        buffer.seek(0);
        serializer->serialize(i, MessagePart::Envelope, buffer, version);
    }
    QCOMPARE(env, data);

    delete serializer;
}

void MailSerializerTest::testParts()
{
    Item item;
    item.setMimeType(QStringLiteral("message/rfc822"));
    KMime::Message *m = new Message;
    KMime::Message::Ptr msg(m);
    item.setPayload(msg);

    SerializerPluginMail *serializer = new SerializerPluginMail();
    QVERIFY(serializer->parts(item).isEmpty());

    msg->setHead("foo");
    QSet<QByteArray> parts = serializer->parts(item);
    QCOMPARE(parts.count(), 2);
    QVERIFY(parts.contains(MessagePart::Envelope));
    QVERIFY(parts.contains(MessagePart::Header));

    msg->setBody("bar");
    parts = serializer->parts(item);
    QCOMPARE(parts.count(), 3);
    QVERIFY(parts.contains(MessagePart::Envelope));
    QVERIFY(parts.contains(MessagePart::Header));
    QVERIFY(parts.contains(MessagePart::Body));

    delete serializer;
}

void MailSerializerTest::testHeaderFetch()
{
    Item i;
    i.setMimeType(QStringLiteral("message/rfc822"));

    SerializerPluginMail *serializer = new SerializerPluginMail();

    QByteArray headerData("From: David Johnson <david@usermode.org>\n"
                          "To: kde-commits@kde.org\n"
                          "MIME-Version: 1.0\n"
                          "Date: Sun, 01 Feb 2009 06:25:22 +0000\n"
                          "Message-Id: <1233469522.741324.18468.nullmailer@svn.kde.org>\n"
                          "Subject: [kde-doc-english] KDE/kdeutils/kcalc\n");

    QString expectedSubject = QStringLiteral("[kde-doc-english] KDE/kdeutils/kcalc");
    QString expectedFrom = QStringLiteral("David Johnson <david@usermode.org>");
    QString expectedTo = QStringLiteral("kde-commits@kde.org");

    // envelope
    QBuffer buffer;
    buffer.setData(headerData);
    buffer.open(QIODevice::ReadOnly);
    buffer.seek(0);
    serializer->deserialize(i, MessagePart::Header, buffer, 0);
    QVERIFY(i.hasPayload<KMime::Message::Ptr>());

    KMime::Message::Ptr msg = i.payload<KMime::Message::Ptr>();
    QCOMPARE(msg->subject()->asUnicodeString(), expectedSubject);
    QCOMPARE(msg->from()->asUnicodeString(), expectedFrom);
    QCOMPARE(msg->to()->asUnicodeString(), expectedTo);

    delete serializer;
}

void MailSerializerTest::testMultiDeserialize()
{
    // The Body part includes the Header.
    // When serialization is done a second time, we should already have the header deserialized.
    // We change the header data for the second deserialization (which is an unrealistic scenario)
    // to demonstrate that it is not deserialized again.

    Item i;
    i.setMimeType(QStringLiteral("message/rfc822"));

    SerializerPluginMail *serializer = new SerializerPluginMail();

    QByteArray messageData("From: David Johnson <david@usermode.org>\n"
                           "To: kde-commits@kde.org\n"
                           "MIME-Version: 1.0\n"
                           "Date: Sun, 01 Feb 2009 06:25:22 +0000\n"
                           "Subject: [kde-doc-english] KDE/kdeutils/kcalc\n"
                           "Content-Type: text/plain\n"
                           "\n"
                           "This is content");

    QString expectedSubject = QStringLiteral("[kde-doc-english] KDE/kdeutils/kcalc");
    QString expectedFrom = QStringLiteral("David Johnson <david@usermode.org>");
    QString expectedTo = QStringLiteral("kde-commits@kde.org");
    QByteArray expectedBody("This is content");

    // envelope
    QBuffer buffer;
    buffer.setData(messageData);
    buffer.open(QIODevice::ReadOnly);
    buffer.seek(0);
    serializer->deserialize(i, MessagePart::Body, buffer, 0);
    QVERIFY(i.hasPayload<KMime::Message::Ptr>());

    KMime::Message::Ptr msg = i.payload<KMime::Message::Ptr>();
    QCOMPARE(msg->subject()->asUnicodeString(), expectedSubject);
    QCOMPARE(msg->from()->asUnicodeString(), expectedFrom);
    QCOMPARE(msg->to()->asUnicodeString(), expectedTo);
    QCOMPARE(msg->body(), expectedBody);

    buffer.close();

    messageData = QByteArray("From: DIFFERENT CONTACT <DIFFERENTCONTACT@example.org>\n"
                             "To: kde-commits@kde.org\n"
                             "MIME-Version: 1.0\n"
                             "Date: Sun, 01 Feb 2009 06:25:22 +0000\n"
                             "Message-Id: <1233469522.741324.18468.nullmailer@svn.kde.org>\n"
                             "Subject: [kde-doc-english] KDE/kdeutils/kcalc\n"
                             "Content-Type: text/plain\n"
                             "\r\n"
                             "This is content");

    buffer.setData(messageData);
    buffer.open(QIODevice::ReadOnly);
    buffer.seek(0);

    serializer->deserialize(i, MessagePart::Header, buffer, 0);
    QVERIFY(i.hasPayload<KMime::Message::Ptr>());

    msg = i.payload<KMime::Message::Ptr>();
    QCOMPARE(msg->subject()->asUnicodeString(), expectedSubject);
    QCOMPARE(msg->from()->asUnicodeString(), expectedFrom);
    QCOMPARE(msg->to()->asUnicodeString(), expectedTo);

    delete serializer;
}
