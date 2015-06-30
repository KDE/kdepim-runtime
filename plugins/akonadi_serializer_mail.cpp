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

#include "akonadi_serializer_mail.h"
#include "akonadi_serializer_mail_debug.h"

#include <QtCore/qplugin.h>
#include <QtCore/QDataStream>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <AkonadiCore/item.h>
#include <Akonadi/KMime/MessageParts>
#include <akonadi/private/imapparser_p.h>

using namespace Akonadi;
using namespace KMime;

QString StringPool::sharedValue(const QString &value)
{
    QMutexLocker lock(&m_mutex);
    QSet<QString>::const_iterator it = m_pool.constFind(value);
    if (it != m_pool.constEnd()) {
        return *it;
    }
    m_pool.insert(value);
    return value;
}

template <typename T>
static void parseAddrList(const QVarLengthArray<QByteArray, 16> &addrList, T *hdr,
                          int version, StringPool &pool)
{
    hdr->clear();
    const int count = addrList.count();
    QVarLengthArray<QByteArray, 16> addr;
    for (int i = 0; i < count; ++i) {
        ImapParser::parseParenthesizedList(addrList[ i ], addr);
        if (addr.count() != 4) {
            qCWarning(AKONADI_SERIALIZER_MAIL_LOG) << "Error parsing envelope address field: " << addrList[ i ];
            continue;
        }
        KMime::Types::Mailbox addrField;
        if (version == 0) {
            addrField.setNameFrom7Bit(addr[0]);
        } else if (version == 1) {
            addrField.setName(pool.sharedValue(QString::fromUtf8(addr[0])));
        }
        KMime::Types::AddrSpec addrSpec;
        addrSpec.localPart = pool.sharedValue(QString::fromUtf8(addr[2]));
        addrSpec.domain = pool.sharedValue(QString::fromUtf8(addr[3]));
        addrField.setAddress(addrSpec);
        hdr->addAddress(addrField);
    }
}

template<typename T>
static void parseAddrList(QDataStream &stream, T *hdr,
                          int version, StringPool &pool)
{
    Q_UNUSED(version);

    hdr->clear();
    int count = 0;
    stream >> count;
    for (int i = 0; i < count; ++i) {
        QString str;
        KMime::Types::Mailbox mbox;
        KMime::Types::AddrSpec addrSpec;
        stream >> str;
        mbox.setName(pool.sharedValue(str));
        stream >> str;
        addrSpec.localPart = pool.sharedValue(str);
        stream >> str;
        addrSpec.domain = pool.sharedValue(str);
        mbox.setAddress(addrSpec);

        hdr->addAddress(mbox);
    }
}

bool SerializerPluginMail::deserialize(Item &item, const QByteArray &label, QIODevice &data, int version)
{
    if (label != MessagePart::Body && label != MessagePart::Envelope && label != MessagePart::Header) {
        return false;
    }

    KMime::Message::Ptr msg;
    if (!item.hasPayload()) {
        Message *m = new  Message();
        msg = KMime::Message::Ptr(m);
        item.setPayload(msg);
    } else {
        msg = item.payload<KMime::Message::Ptr>();
    }

    if (label == MessagePart::Body) {
        QByteArray buffer = data.readAll();
        if (buffer.isEmpty()) {
            return true;
        }
        msg->setContent(buffer);
        msg->parse();
    } else if (label == MessagePart::Header) {
        QByteArray buffer = data.readAll();
        if (buffer.isEmpty()) {
            return true;
        }
        if (msg->body().isEmpty() && msg->contents().isEmpty()) {
            msg->setHead(buffer);
            msg->parse();
        }
    } else if (label == MessagePart::Envelope) {
        if (version <= 1) {
            QByteArray buffer = data.readAll();
            if (buffer.isEmpty()) {
                return true;
            }
            QVarLengthArray<QByteArray, 16> env;
            ImapParser::parseParenthesizedList(buffer, env);
            if (env.count() < 10) {
                qCWarning(AKONADI_SERIALIZER_MAIL_LOG) << "Akonadi KMime Deserializer: Got invalid envelope: " << buffer;
                return false;
            }
            Q_ASSERT(env.count() >= 10);
            // date
            msg->date()->from7BitString(env[0]);
            // subject
            msg->subject()->from7BitString(env[1]);
            // from
            QVarLengthArray<QByteArray, 16> addrList;
            ImapParser::parseParenthesizedList(env[2], addrList);
            if (!addrList.isEmpty()) {
                parseAddrList(addrList, msg->from(), version, m_stringPool);
            }
            // sender
            ImapParser::parseParenthesizedList(env[3], addrList);
            if (!addrList.isEmpty()) {
                parseAddrList(addrList, msg->sender(), version, m_stringPool);
            }
            // reply-to
            ImapParser::parseParenthesizedList(env[4], addrList);
            if (!addrList.isEmpty()) {
                parseAddrList(addrList, msg->replyTo(), version, m_stringPool);
            }
            // to
            ImapParser::parseParenthesizedList(env[5], addrList);
            if (!addrList.isEmpty()) {
                parseAddrList(addrList, msg->to(), version, m_stringPool);
            }
            // cc
            ImapParser::parseParenthesizedList(env[6], addrList);
            if (!addrList.isEmpty()) {
                parseAddrList(addrList, msg->cc(), version, m_stringPool);
            }
            // bcc
            ImapParser::parseParenthesizedList(env[7], addrList);
            if (!addrList.isEmpty()) {
                parseAddrList(addrList, msg->bcc(), version, m_stringPool);
            }
            // in-reply-to
            msg->inReplyTo()->from7BitString(env[8]);
            // message id
            msg->messageID()->from7BitString(env[9]);
            // references
            if (env.count() > 10) {
                msg->references()->from7BitString(env[10]);
            }
        } else if (version == 2) {
            QDataStream stream(&data);
            QDateTime dt;
            QString str;

            stream >> dt;
            msg->date()->setDateTime(dt);
            stream >> str;
            msg->subject()->fromUnicodeString(str, "UTF-8");
            stream >> str;
            msg->inReplyTo()->fromUnicodeString(str, "UTF-8");
            stream >> str;
            msg->messageID()->fromUnicodeString(str, "UTF-8");
            stream >> str;
            msg->references()->fromUnicodeString(str, "UTF-8");

            parseAddrList(stream, msg->from(), version, m_stringPool);
            parseAddrList(stream, msg->sender(), version, m_stringPool);
            parseAddrList(stream, msg->replyTo(), version, m_stringPool);
            parseAddrList(stream, msg->to(), version, m_stringPool);
            parseAddrList(stream, msg->cc(), version, m_stringPool);
            parseAddrList(stream, msg->bcc(), version, m_stringPool);

            if (stream.status() == QDataStream::ReadCorruptData || stream.status() == QDataStream::ReadPastEnd) {
                qCWarning(AKONADI_SERIALIZER_MAIL_LOG) << "Akonadi KMime Deserializer: Got invalid envelope";
                return false;
            }
        }
    }

    return true;
}

template<typename T>
static void serializeAddrList(QDataStream &stream, T *hdr)
{
    KMime::Types::Mailbox::List mb = hdr->mailboxes();
    stream << mb.size();
    foreach (const KMime::Types::Mailbox &mbox, mb) {
        stream << mbox.name()
               << mbox.addrSpec().localPart
               << mbox.addrSpec().domain;
    }
}

void SerializerPluginMail::serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version)
{
    version = 1;

    boost::shared_ptr<Message> m = item.payload< boost::shared_ptr<Message> >();
    if (label == MessagePart::Body) {
        data.write(m->encodedContent());
    } else if (label == MessagePart::Envelope) {
        version = 2;
        QDataStream stream(&data);
        stream << m->date()->dateTime()
               << m->subject()->asUnicodeString()
               << m->inReplyTo()->asUnicodeString()
               << m->messageID()->asUnicodeString()
               << m->references()->asUnicodeString();
        serializeAddrList(stream, m->from());
        serializeAddrList(stream, m->sender());
        serializeAddrList(stream, m->replyTo());
        serializeAddrList(stream, m->to());
        serializeAddrList(stream, m->cc());
        serializeAddrList(stream, m->bcc());
    } else if (label == MessagePart::Header) {
        data.write(m->head());
    }
}

QSet<QByteArray> SerializerPluginMail::parts(const Item &item) const
{
    QSet<QByteArray> set;

    if (!item.hasPayload<KMime::Message::Ptr>()) {
        return set;
    }

    KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
    if (!msg) {
        return set;
    }

    // FIXME: we really want "has any header" here, but the kmime api doesn't offer that yet
    if (msg->hasContent() || msg->hasHeader("Message-ID")) {
        set << MessagePart::Envelope << MessagePart::Header;
        if (!msg->body().isEmpty() || !msg->contents().isEmpty()) {
            set << MessagePart::Body;
        }
    }
    return set;
}

QString SerializerPluginMail::extractGid(const Item &item) const
{
    if (!item.hasPayload<KMime::Message::Ptr>()) {
        return QString();
    }
    const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
    KMime::Headers::MessageID *mid = msg->messageID(false);
    if (mid) {
        return mid->asUnicodeString();
    } else if (KMime::Headers::Base *uid = msg->headerByType("X-Akonotes-UID")) {
        return uid->asUnicodeString();
    }
    return QString();
}

#include "moc_akonadi_serializer_mail.cpp"
