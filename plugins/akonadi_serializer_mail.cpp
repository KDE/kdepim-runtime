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

#include <QtCore/qplugin.h>

#include <kdebug.h>
#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <akonadi/item.h>
#include <akonadi/kmime/messageparts.h>
#include <akonadi/private/imapparser_p.h>

using namespace Akonadi;
using namespace KMime;

typedef boost::shared_ptr<KMime::Message> MessagePtr;

template <typename T> static void parseAddrList( const QList<QByteArray> &addrList, T *hdr )
{
  for ( QList<QByteArray>::ConstIterator it = addrList.constBegin(); it != addrList.constEnd(); ++it ) {
    QList<QByteArray> addr;
    ImapParser::parseParenthesizedList( *it, addr );
    if ( addr.count() != 4 ) {
      kWarning( 5264 ) << "Error parsing envelope address field: " << addr;
      continue;
    }
    KMime::Types::Mailbox addrField;
    addrField.setNameFrom7Bit( addr[0] );
    addrField.setAddress( addr[2] + '@' + addr[3] );
    hdr->addAddress( addrField );
  }
}


bool SerializerPluginMail::deserialize( Item& item, const QByteArray& label, QIODevice& data )
{
    if ( label != MessagePart::Body && label != MessagePart::Envelope && label != MessagePart::Header )
      return false;

    MessagePtr msg;
    if ( !item.hasPayload() ) {
        Message *m = new  Message();
        msg = MessagePtr( m );
        item.setPayload( msg );
    } else {
        msg = item.payload<MessagePtr>();
    }

    QByteArray buffer = data.readAll();
    if ( buffer.isEmpty() )
      return true;
    if ( label == MessagePart::Body ) {
      msg->setContent( buffer );
      msg->parse();
    } else if ( label == MessagePart::Header ) {
      if ( !msg->body().isEmpty() && !msg->contents().isEmpty() ) {
        msg->setHead( buffer );
        msg->parse();
      }
    } else if ( label == MessagePart::Envelope ) {
        QList<QByteArray> env;
        ImapParser::parseParenthesizedList( buffer, env );
        if ( env.count() < 10 ) {
          kWarning( 5264 ) << "Akonadi KMime Deserializer: Got invalid envelope: " << env;
          return false;
        }
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
        ImapParser::parseParenthesizedList( env[2], addrList );
        if ( !addrList.isEmpty() )
          parseAddrList( addrList, msg->sender() );
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

    return true;
}

static QByteArray quoteImapListEntry( const QByteArray &b )
{
  if ( b.isEmpty() )
    return "NIL";
  return ImapParser::quote( b );
}

static QByteArray buildImapList( const QList<QByteArray> &list )
{
  if ( list.isEmpty() )
    return "NIL";
  return QByteArray( "(" ) + ImapParser::join( list, " " ) + QByteArray( ")" );
}

template <typename T> static QByteArray buildAddrStruct( T const *hdr )
{
  QList<QByteArray> addrList;
  KMime::Types::Mailbox::List mb = hdr->mailboxes();
  foreach ( const KMime::Types::Mailbox mbox, mb ) {
    QList<QByteArray> addrStruct;
    addrStruct << quoteImapListEntry( KMime::encodeRFC2047String( mbox.name(), "utf-8" ) );
    addrStruct << quoteImapListEntry( QByteArray() );
    addrStruct << quoteImapListEntry( mbox.addrSpec().localPart.toUtf8() );
    addrStruct << quoteImapListEntry( mbox.addrSpec().domain.toUtf8() );
    addrList << buildImapList( addrStruct );
  }
  return buildImapList( addrList );
}

void SerializerPluginMail::serialize( const Item& item, const QByteArray& label, QIODevice& data )
{
  boost::shared_ptr<Message> m = item.payload< boost::shared_ptr<Message> >();
  m->assemble();
  if ( label == MessagePart::Body ) {
    data.write( m->encodedContent() );
  } else if ( label == MessagePart::Envelope ) {
    QList<QByteArray> env;
    env << quoteImapListEntry( m->date()->as7BitString( false ) );
    env << quoteImapListEntry( m->subject()->as7BitString( false ) );
    env << buildAddrStruct( m->from() );
    env << buildAddrStruct( m->sender() );
    env << buildAddrStruct( m->replyTo() );
    env << buildAddrStruct( m->to() );
    env << buildAddrStruct( m->cc() );
    env << buildAddrStruct( m->bcc() );
    env << quoteImapListEntry( m->inReplyTo()->as7BitString( false ) );
    env << quoteImapListEntry( m->messageID()->as7BitString( false ) );
    data.write( buildImapList( env ) );
  } else if ( label == MessagePart::Header ) {
    data.write( m->head() );
  }
}

QSet<QByteArray> SerializerPluginMail::parts(const Item & item) const
{
  if ( !item.hasPayload<MessagePtr>() )
    return QSet<QByteArray>();
  MessagePtr msg = item.payload<MessagePtr>();
  QSet<QByteArray> set;
  // FIXME: we actually want "has any header" here, but the kmime api doesn't offer that yet
  if ( msg->hasContent() || msg->hasHeader( "Message-ID" ) ) {
    set << MessagePart::Envelope << MessagePart::Header;
    if ( !msg->body().isEmpty() || !msg->contents().isEmpty() )
      set << MessagePart::Body;
  }
  return set;
}

Q_EXPORT_PLUGIN2( akonadi_serializer_mail, SerializerPluginMail );

#include "akonadi_serializer_mail.moc"
