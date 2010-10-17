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

template <typename T> static void parseAddrList( const QVarLengthArray<QByteArray, 16> &addrList, T *hdr, int version )
{
  hdr->clear();
  const int count = addrList.count();
  QVarLengthArray<QByteArray, 16> addr;
  for ( int i = 0; i < count; ++i ) {
    ImapParser::parseParenthesizedList( addrList[ i ], addr );
    if ( addr.count() != 4 ) {
      kWarning( 5264 ) << "Error parsing envelope address field: " << addrList[ i ];
      continue;
    }
    KMime::Types::Mailbox addrField;
    if ( version == 0 )
      addrField.setNameFrom7Bit( addr[0] );
    else if ( version == 1 )
      addrField.setName( QString::fromUtf8( addr[0] ) );
    KMime::Types::AddrSpec addrSpec;
    addrSpec.localPart = QString::fromUtf8( addr[2] );
    addrSpec.domain = QString::fromUtf8( addr[3] );
    addrField.setAddress( addrSpec );
    hdr->addAddress( addrField );
  }
}


bool SerializerPluginMail::deserialize( Item& item, const QByteArray& label, QIODevice& data, int version )
{
    if ( label != MessagePart::Body && label != MessagePart::Envelope && label != MessagePart::Header )
      return false;

    KMime::Message::Ptr msg;
    if ( !item.hasPayload() ) {
        Message *m = new  Message();
        msg = KMime::Message::Ptr( m );
        item.setPayload( msg );
    } else {
        msg = item.payload<KMime::Message::Ptr>();
    }

    QByteArray buffer = data.readAll();
    if ( buffer.isEmpty() )
      return true;
    if ( label == MessagePart::Body ) {
      msg->setContent( buffer );
      msg->parse();
    } else if ( label == MessagePart::Header ) {
      if ( msg->body().isEmpty() && msg->contents().isEmpty() ) {
        msg->setHead( buffer );
        msg->parse();
      }
    } else if ( label == MessagePart::Envelope ) {
        QVarLengthArray<QByteArray, 16> env;
        ImapParser::parseParenthesizedList( buffer, env );
        if ( env.count() < 10 ) {
          kWarning( 5264 ) << "Akonadi KMime Deserializer: Got invalid envelope: " << buffer;
          return false;
        }
        Q_ASSERT( env.count() >= 10 );
        // date
        msg->date()->from7BitString( env[0] );
        // subject
        msg->subject()->from7BitString( env[1] );
        // from
        QVarLengthArray<QByteArray, 16> addrList;
        ImapParser::parseParenthesizedList( env[2], addrList );
        if ( !addrList.isEmpty() )
          parseAddrList( addrList, msg->from(), version );
        // sender
        ImapParser::parseParenthesizedList( env[2], addrList );
        if ( !addrList.isEmpty() )
          parseAddrList( addrList, msg->sender(), version );
        // reply-to
        ImapParser::parseParenthesizedList( env[4], addrList );
        if ( !addrList.isEmpty() )
          parseAddrList( addrList, msg->replyTo(), version );
        // to
        ImapParser::parseParenthesizedList( env[5], addrList );
        if ( !addrList.isEmpty() )
          parseAddrList( addrList, msg->to(), version );
        // cc
        ImapParser::parseParenthesizedList( env[6], addrList );
        if ( !addrList.isEmpty() )
          parseAddrList( addrList, msg->cc(), version );
        // bcc
        ImapParser::parseParenthesizedList( env[7], addrList );
        if ( !addrList.isEmpty() )
          parseAddrList( addrList, msg->bcc(), version );
        // in-reply-to
        msg->inReplyTo()->from7BitString( env[8] );
        // message id
        msg->messageID()->from7BitString( env[9] );
        // references
        if ( env.count() > 10 )
          msg->references()->from7BitString( env[10] );
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
  foreach ( const KMime::Types::Mailbox &mbox, mb ) {
    QList<QByteArray> addrStruct;
    addrStruct << quoteImapListEntry( mbox.name().toUtf8() );
    addrStruct << quoteImapListEntry( QByteArray() );
    addrStruct << quoteImapListEntry( mbox.addrSpec().localPart.toUtf8() );
    addrStruct << quoteImapListEntry( mbox.addrSpec().domain.toUtf8() );
    addrList << buildImapList( addrStruct );
  }
  return buildImapList( addrList );
}

void SerializerPluginMail::serialize( const Item& item, const QByteArray& label, QIODevice& data, int &version )
{
  version = 1;

  boost::shared_ptr<Message> m = item.payload< boost::shared_ptr<Message> >();
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
    env << quoteImapListEntry( m->references()->as7BitString( false ) );
    data.write( buildImapList( env ) );
  } else if ( label == MessagePart::Header ) {
    data.write( m->head() );
  }
}

QSet<QByteArray> SerializerPluginMail::parts(const Item & item) const
{
  if ( !item.hasPayload<KMime::Message::Ptr>() )
    return QSet<QByteArray>();
  KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  QSet<QByteArray> set;
  // FIXME: we actually want "has any header" here, but the kmime api doesn't offer that yet
  if ( msg->hasContent() || msg->hasHeader( "Message-ID" ) ) {
    set << MessagePart::Envelope << MessagePart::Header;
    if ( !msg->body().isEmpty() || !msg->contents().isEmpty() )
      set << MessagePart::Body;
  }
  return set;
}

Q_EXPORT_PLUGIN2( akonadi_serializer_mail, SerializerPluginMail )

#include "akonadi_serializer_mail.moc"
