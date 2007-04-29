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

#include <QDebug>
#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include "../libakonadi/item.h"
#include "../libakonadi/imapparser.h"

using namespace Akonadi;
using namespace KMime;

typedef boost::shared_ptr<KMime::Message> MessagePtr;

template <typename T> static void parseAddrList( const QList<QByteArray> &addrList, T *hdr )
{
  for ( QList<QByteArray>::ConstIterator it = addrList.constBegin(); it != addrList.constEnd(); ++it ) {
    QList<QByteArray> addr;
    ImapParser::parseParenthesizedList( *it, addr );
    if ( addr.count() != 4 ) {
      qWarning() << "Error parsing envelope address field: " << addr;
      continue;
    }
    KMime::Types::Mailbox addrField;
    addrField.setNameFrom7Bit( addr[0] );
    addrField.setAddress( addr[2] + '@' + addr[3] );
    hdr->addAddress( addrField );
  }
}


void SerializerPluginMail::deserialize( Item& item, const QString& label, const QByteArray& data ) const
{
    if ( label != "RFC822" && label != "ENVELOPE" ) {
      item.addPart( label, data );
      return;
    }
    if ( item.mimeType() != QString::fromLatin1("message/rfc822") ) {
        //throw ItemSerializerException();
        return;
    }

    MessagePtr msg;
    if ( !item.hasPayload() ) {
        Message *m = new  Message();
        msg = MessagePtr( m );
        item.setPayload( msg );
    } else {
        msg = item.payload<MessagePtr>();
    }

    if ( label == "RFC822" ) {
        msg->setContent( data );
        msg->parse();
    } else if ( label == "ENVELOPE" ) {
        QList<QByteArray> env;
        ImapParser::parseParenthesizedList( data, env );
        Q_ASSERT( env.count() >= 10 );
        // date
        msg->date()->from7BitString( env[0] );
        // subject
        msg->subject()->from7BitString( env[1] );
        // from
        QList<QByteArray> addrList;
        ImapParser::parseParenthesizedList( env[2], addrList );
        if ( !addrList.isEmpty() )
          parseAddrList( addrList, msg->from() );
        // sender
        // not supported by kmime, skip it
        // reply-to
        ImapParser::parseParenthesizedList( env[4], addrList );
        if ( !addrList.isEmpty() )
          parseAddrList( addrList, msg->replyTo() );
        // to
        ImapParser::parseParenthesizedList( env[5], addrList );
        if ( !addrList.isEmpty() )
          parseAddrList( addrList, msg->to() );
        // cc
        ImapParser::parseParenthesizedList( env[6], addrList );
        if ( !addrList.isEmpty() )
          parseAddrList( addrList, msg->cc() );
        // bcc
        ImapParser::parseParenthesizedList( env[7], addrList );
        if ( !addrList.isEmpty() )
          parseAddrList( addrList, msg->bcc() );
        // in-reply-to
        msg->inReplyTo()->from7BitString( env[8] );
        // message id
        msg->messageID()->from7BitString( env[9] );
    }
}


void SerializerPluginMail::deserialize( Item& item, const QString& label, const QIODevice& data ) const
{
  Q_UNUSED( item );
  Q_UNUSED( label );
  Q_UNUSED( data );
}


void SerializerPluginMail::serialize( const Item& item, const QString& label, QByteArray& data ) const
{
    if ( label != "RFC822" )
      return;

    boost::shared_ptr<Message> m = item.payload< boost::shared_ptr<Message> >();
    m->assemble();
    data = m->encodedContent();
}


void SerializerPluginMail::serialize( const Item& item, const QString& label, QIODevice& data ) const
{
  Q_UNUSED( item );
  Q_UNUSED( label );
  Q_UNUSED( data );
}

extern "C"
KDE_EXPORT Akonadi::ItemSerializerPlugin *
libakonadi_serializer_mail_create_item_serializer_plugin() {
  return new SerializerPluginMail();
}


