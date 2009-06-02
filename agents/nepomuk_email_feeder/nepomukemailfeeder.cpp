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
#include "emailaddress.h"
#include "personcontact.h"

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

#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/XMLSchema>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>


using namespace Akonadi;
typedef boost::shared_ptr<KMime::Message> MessagePtr;

Akonadi::NepomukEMailFeeder::NepomukEMailFeeder( const QString &id ) :
  AgentBase( id )
{
  changeRecorder()->itemFetchScope().fetchPayloadPart( MessagePart::Envelope );
  changeRecorder()->setMimeTypeMonitored( "message/rfc822" );
  changeRecorder()->setMimeTypeMonitored( "message/news" );
  changeRecorder()->setChangeRecordingEnabled( false );

  // initialize Nepomuk
  Nepomuk::ResourceManager::instance()->init();
}

void NepomukEMailFeeder::itemAdded(const Akonadi::Item & item, const Akonadi::Collection & collection)
{
  Q_UNUSED( collection );
  itemChanged( item, QSet<QByteArray>() );
}

void NepomukEMailFeeder::itemChanged(const Akonadi::Item & item, const QSet<QByteArray> & partIdentifiers)
{
  Q_UNUSED( partIdentifiers );
  if ( !item.hasPayload<MessagePtr>() )
    return;
  MessagePtr msg = item.payload<MessagePtr>();

  // FIXME: make a distinction between email and news
  Nepomuk::Email r( item.url() );

  if ( msg->subject( false ) ) {
    r.setMessageSubject( msg->subject()->asUnicodeString() );
  }

  if ( msg->date( false ) ) {
    r.setReceivedDate( msg->date()->dateTime().dateTime() );
  }

  if ( msg->from( false ) ) {
    r.setSenders( extractContactsFromMailboxes( msg->from()->mailboxes() ) );
  }

  if ( msg->to( false ) ) {
    r.setTos( extractContactsFromMailboxes( msg->to()->mailboxes() ) );
  }

  if ( msg->cc( false ) ) {
    r.setCcs( extractContactsFromMailboxes( msg->cc()->mailboxes() ) );
  }

  if ( msg->bcc( false ) ) {
    r.setBccs( extractContactsFromMailboxes( msg->bcc()->mailboxes() ) );
  }

  KMime::Content* content = msg->mainBodyPart( "text/plain" );

  // FIXME: simplyfy this text as in: remove all html tags. Is there a quick way to do this?
  if ( content ) {
    QString text = content->decodedText( true, true );
    if ( !text.isEmpty() ) {
      r.setProperty( Soprano::Vocabulary::Xesam::asText(), text );
    }
  }

  // FIXME: is xesam:id the best idea here?
  if ( msg->messageID( false ) ) {
    r.setProperty( Soprano::Vocabulary::Xesam::id(), Nepomuk::Variant(msg->messageID()->asUnicodeString()) );
  }

  // IDEA: use Strigi to index the attachments
}

void NepomukEMailFeeder::itemRemoved(const Akonadi::Item & item)
{
  Nepomuk::Resource r( item.url() );
  r.remove();
}

QList<Nepomuk::Contact> NepomukEMailFeeder::extractContactsFromMailboxes( const KMime::Types::Mailbox::List& mbs )
{
  QList<Nepomuk::Contact> contacts;

  foreach( const KMime::Types::Mailbox& mbox, mbs ) {
    if ( mbox.hasAddress() ) {
      Nepomuk::PersonContact c = findContact( mbox.address() );
      if ( c.fullnames().isEmpty() && mbox.hasName() )
        c.addFullname( mbox.name() );
      contacts << c;
    }
  }

  return contacts;
}

Nepomuk::Contact NepomukEMailFeeder::findContact( const QByteArray& address )
{
  //
  // Querying with the exact address string is not perfect since email addresses
  // are case insensitive. But for the moment we stick to it and hope Nepomuk
  // alignment fixes any duplicates
  //
  Soprano::QueryResultIterator it =
    Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( QString( "select distinct ?r where { ?r <%1> ?a . ?a <%2> \"%3\"^^<%4> . }" )
                                                                     .arg( Nepomuk::Role::emailAddressUri().toString() )
                                                                     .arg( Nepomuk::EmailAddress::emailAddressUri().toString() )
                                                                     .arg( QString::fromAscii( address ) )
                                                                     .arg( Soprano::Vocabulary::XMLSchema::string().toString() ),
                                                                     Soprano::Query::QueryLanguageSparql );
  if ( it.next() ) {
    return Nepomuk::Contact( it.binding( 0 ).uri() );
  }
  else {
    // create a new contact
    Nepomuk::PersonContact contact;
    Nepomuk::EmailAddress email( QUrl( "mailto:" + address ) );
    email.setEmailAddress( QString::fromAscii( address ) );
    contact.addEmailAddress( email );
    return contact;
  }
}

AKONADI_AGENT_MAIN( NepomukEMailFeeder )

#include "nepomukemailfeeder.moc"
