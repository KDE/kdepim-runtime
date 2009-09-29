/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>
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

#include "nepomukemailfeeder.h"
#include "email.h"
#include "personcontact.h"
#include "selectsqarqlbuilder.h"

#include <akonadi/changerecorder.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/kmime/messageparts.h>

#include <kmime/kmime_message.h>
#include <kmime/kmime_content.h>
#include <boost/shared_ptr.hpp>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>
#include <kurl.h>

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/XMLSchema>

using namespace Akonadi;

Akonadi::NepomukEMailFeeder::NepomukEMailFeeder( const QString &id ) :
  NepomukFeederAgent<NepomukFast::Mailbox>( id )
{
  addSupportedMimeType( "message/rfc822" );
  addSupportedMimeType( "message/news" );

  changeRecorder()->itemFetchScope().fetchFullPayload();
}

NepomukEMailFeeder::~NepomukEMailFeeder()
{
}

void NepomukEMailFeeder::updateItem(const Akonadi::Item & item, const QUrl &graphUri)
{
  if ( !item.hasPayload<KMime::Message::Ptr>() )
    return;
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();

  // FIXME: make a distinction between email and news
  NepomukFast::Email r( item.url(), graphUri );
  setParent( r, item );

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
}

QList<NepomukFast::Contact> NepomukEMailFeeder::extractContactsFromMailboxes( const KMime::Types::Mailbox::List& mbs,
                                                                              const QUrl &graphUri )
{
  QList<NepomukFast::Contact> contacts;

  foreach( const KMime::Types::Mailbox& mbox, mbs ) {
    if ( mbox.hasAddress() ) {
      const NepomukFast::Contact c = findOrCreateContact( QString::fromLatin1( mbox.address() ), mbox.name(), graphUri );
      contacts << c;
    }
  }

  return contacts;
}

AKONADI_AGENT_MAIN( NepomukEMailFeeder )

#include "nepomukemailfeeder.moc"
