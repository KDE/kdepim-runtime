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
#include <attachment.h>

#include <akonadi/item.h>

#include <kmime/kmime_message.h>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>

#include <strigi/analyzerconfiguration.h>
#include <strigi/analysisresult.h>
#include <strigi/indexpluginloader.h>
#include <strigi/indexmanager.h>
#include <strigi/indexwriter.h>
#include <strigi/streamanalyzer.h>
#include <strigi/stringstream.h>

#include <boost/shared_ptr.hpp>

MessageAnalyzer::MessageAnalyzer(const Akonadi::Item& item, const QUrl& graphUri, QObject* parent) :
  QObject( parent ),
  m_email( item.url(), graphUri ),
  m_graphUri( graphUri )
{
  NepomukFeederAgentBase::setParent( m_email, item );
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();

  processHeaders( msg );
  processPart( msg.get() );

  // TODO: run OTP for decryption

  KMime::Content* content = msg->mainBodyPart( "text/plain" );

  // FIXME: simplyfy this text as in: remove all html tags. Is there a quick way to do this?
  if ( content ) {
    const QString text = content->decodedText( true, true );
    if ( !text.isEmpty() ) {
      m_email.setPlainTextMessageContents( QStringList( text ) );
    }
  }

  deleteLater();
}

void MessageAnalyzer::processHeaders(const KMime::Message::Ptr& msg)
{
  if ( msg->subject( false ) ) {
    m_email.setMessageSubject( msg->subject()->asUnicodeString() );
    m_email.setLabel( msg->subject()->asUnicodeString() );
  }

  if ( msg->date( false ) ) {
    m_email.setReceivedDate( msg->date()->dateTime().dateTime() );
  }

  if ( msg->from( false ) ) {
    m_email.setFroms( extractContactsFromMailboxes( msg->from()->mailboxes(), graphUri() ) );
  }

  if ( msg->sender( false ) )
    m_email.setSenders( extractContactsFromMailboxes( msg->sender()->mailboxes(), graphUri() ) );

  if ( msg->to( false ) ) {
    m_email.setTos( extractContactsFromMailboxes( msg->to()->mailboxes(), graphUri() ) );
  }

  if ( msg->cc( false ) ) {
    m_email.setCcs( extractContactsFromMailboxes( msg->cc()->mailboxes(), graphUri() ) );
  }

  if ( msg->bcc( false ) ) {
    m_email.setBccs( extractContactsFromMailboxes( msg->bcc()->mailboxes(), graphUri() ) );
  }

  if ( msg->messageID( false ) ) {
    m_email.setMessageIds( QStringList( msg->messageID()->asUnicodeString() ) );
  }
}

void MessageAnalyzer::processPart(KMime::Content* content)
{
  // multipart -> recurse
  if ( content->contentType()->isMultipart() ) {
    // TODO: handle multipart/alternative and multipart/encrypted separately
    foreach ( KMime::Content* child, content->contents() )
      processPart( child );
  }

  // main body part 
  // TODO handle primary body part and put the code from the ctor here

  // attachment -> delegate to strigi
  else {
    const KMime::ContentIndex index = content->index();
    KUrl attachmentUrl = m_email.uri();
    attachmentUrl.setHTMLRef( index.toString() );
    kDebug() << attachmentUrl;
    NepomukFast::Attachment attachment( attachmentUrl, graphUri() );
    attachment.addProperty( Vocabulary::NIE::isPartOf(), m_email.uri() );
    if ( !content->contentType()->name().isEmpty() )
      attachment.setLabel( content->contentType()->name() );
    else if ( content->contentDisposition( false ) && !content->contentDisposition()->filename().isEmpty() )
      attachment.setLabel( content->contentDisposition()->filename() );
    if ( content->contentDescription( false ) && !content->contentDescription()->asUnicodeString().isEmpty() )
      attachment.addProperty( Vocabulary::NIE::description(), Soprano::LiteralValue( content->contentDescription()->asUnicodeString() ) );
    m_email.addAttachment( attachment );
    processAttachmentBody( attachmentUrl, content );
  }
}

void MessageAnalyzer::processAttachmentBody(const KUrl& url, KMime::Content* content)
{
  const QByteArray decodedContent = content->decodedContent();

  Strigi::IndexManager* indexManager = Strigi::IndexPluginLoader::createIndexManager( "sopranobackend", 0 );
  Q_ASSERT( indexManager );

  Strigi::IndexWriter* writer = indexManager->indexWriter();
  Strigi::AnalyzerConfiguration ic;
  Strigi::StreamAnalyzer streamindexer( ic );
  streamindexer.setIndexWriter( *writer );
  Strigi::StringInputStream sr( decodedContent.constData(), decodedContent.size(), false );
  Strigi::AnalysisResult idx( url.url().toLatin1().constData(), QDateTime::currentDateTime().toTime_t(), *writer, streamindexer );
  idx.index( &sr );

  Strigi::IndexPluginLoader::deleteIndexManager( indexManager );
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
