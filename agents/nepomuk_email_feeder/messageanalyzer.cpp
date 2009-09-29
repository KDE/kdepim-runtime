/*
    Copyright (c) 2006, 2009 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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

#include "messageanalyzer.h"

#include <email.h>
#include <personcontact.h>
#include <nepomukfeederagentbase.h>

#include <akonadi/item.h>

#include <kmime/kmime_message.h>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>

#include <kurl.h>

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>

#include <boost/shared_ptr.hpp>

MessageAnalyzer::MessageAnalyzer(const Akonadi::Item& item, const QUrl& graphUri, QObject* parent) :
  QObject( parent )
{
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();

  // FIXME: make a distinction between email and news
  NepomukFast::Email r( item.url(), graphUri );
  NepomukFeederAgentBase::setParent( r, item );

  if ( msg->subject( false ) ) {
    r.setMessageSubject( msg->subject()->asUnicodeString() );
    r.setLabel( msg->subject()->asUnicodeString() );
  }

  if ( msg->date( false ) ) {
    r.setReceivedDate( msg->date()->dateTime().dateTime() );
  }

  if ( msg->from( false ) ) {
    r.setFroms( extractContactsFromMailboxes( msg->from()->mailboxes(), graphUri ) );
  }

  if ( msg->sender( false ) )
    r.setSenders( extractContactsFromMailboxes( msg->sender()->mailboxes(), graphUri ) );

  if ( msg->to( false ) ) {
    r.setTos( extractContactsFromMailboxes( msg->to()->mailboxes(), graphUri ) );
  }

  if ( msg->cc( false ) ) {
    r.setCcs( extractContactsFromMailboxes( msg->cc()->mailboxes(), graphUri ) );
  }

  if ( msg->bcc( false ) ) {
    r.setBccs( extractContactsFromMailboxes( msg->bcc()->mailboxes(), graphUri ) );
  }

  KMime::Content* content = msg->mainBodyPart( "text/plain" );

  // FIXME: simplyfy this text as in: remove all html tags. Is there a quick way to do this?
  if ( content ) {
    const QString text = content->decodedText( true, true );
    if ( !text.isEmpty() ) {
      r.setPlainTextMessageContents( QStringList( text ) );
    }
  }

  if ( msg->messageID( false ) ) {
    r.setMessageIds( QStringList( msg->messageID()->asUnicodeString() ) );
  }

  // IDEA: use Strigi to index the attachments


  deleteLater();
}

QList< NepomukFast::Contact > MessageAnalyzer::extractContactsFromMailboxes(const KMime::Types::Mailbox::List& mbs, const QUrl&graphUri )
{
  QList<NepomukFast::Contact> contacts;

  foreach( const KMime::Types::Mailbox& mbox, mbs ) {
    if ( mbox.hasAddress() ) {
      const NepomukFast::Contact c = NepomukFeederAgentBase::findOrCreateContact( QString::fromLatin1( mbox.address() ), mbox.name(), graphUri );
      contacts << c;
    }
  }

  return contacts;
}

#include "messageanalyzer.moc"
