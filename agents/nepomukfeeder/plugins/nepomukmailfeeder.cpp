/*
    Copyright (c) 2006, 2009 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>
    Copyright (c) 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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


#include "nepomukmailfeeder.h"

#include <Akonadi/Item>

#include <KDebug>
#include <kurl.h>
#include <KLocale>

#include <KMime/Message>

// ontology includes
#include <nepomuk/nfo.h>
#include <nepomuk/nmo.h>
#include <nmo/email.h>
#include <nmo/messageheader.h>
#include <nao/tag.h>
#include <ncal/attachment.h>
#include <Nepomuk/Vocabulary/NFO>
#include <Soprano/Vocabulary/NAO>
#include <Nepomuk/Vocabulary/NIE>

#include <qplugin.h>
#include <kexportplugin.h>
#include <kpluginfactory.h>
#include <akonadi/kmime/messagestatus.h>

#include <nepomukfeederutils.h>


using namespace Nepomuk;

namespace Akonadi {

NepomukMailFeeder::NepomukMailFeeder(QObject *parent, const QVariantList &)
: NepomukFeederPlugin(parent)
{
}

void NepomukMailFeeder::updateItem(const Akonadi::Item& item, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph)
{
  //kDebug() << item.id();
  Q_ASSERT(item.hasPayload());

  if ( !item.hasPayload<KMime::Message::Ptr>() ) {
    kWarning() << "Got item without known payload. Mimetype:" << item.mimeType()
              << "Id:" << item.id() << item.payloadData();
    return;
  }
  Akonadi::MessageStatus status;
  status.setStatusFromFlags( item.flags() );
  if ( status.isSpam() )
    return; // don't bother with indexing spam

  res.addType( Vocabulary::NMO::Email() );
  NepomukFeederUtils::setIcon( "internet-mail", res, graph );
  res.setProperty( Vocabulary::NIE::byteSize(), item.size() );

  processFlags( item.flags(), res, graph );
  KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  processHeaders( msg, res, graph );

  if ( !msg->body().isEmpty() || !msg->contents().isEmpty() ) {
    processContent( msg, item, res, graph );
  }

}


void NepomukMailFeeder::processContent(const KMime::Message::Ptr& msg, const Akonadi::Item &item, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph )
{
  // before we walk the part node tree, let's see if there is a main plain text body, so we don't interpret that as an attachment later on
  m_mainBodyPart = msg->mainBodyPart( "text/plain" );
  if ( m_mainBodyPart ) {
    const QString text = m_mainBodyPart->decodedText( true, true );
    if ( !text.isEmpty() ) {
      //kDebug() << "indexingText";
      Nepomuk::NMO::Message message(&res); //The email wrapper doesn't contain the functions from Message
      message.setPlainTextMessageContents( QStringList( text ) );
    }
  }
  processPart( msg.get(), item, res, graph );
}

void NepomukMailFeeder::processPart( KMime::Content* content, const Akonadi::Item &item, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph )
{
  // multipart -> recurse
  if ( content->contentType()->isMultipart() ) {
    if ( content->contentType()->isSubtype( "encrypted" )/* && !m_indexEncryptedContent */)
      return;
    // TODO what about multipart/alternative?
    foreach ( KMime::Content* child, content->contents() ) {
      processPart( child, item, res, graph );
    }
  }

  // plain text main body part, we already dealt with that
  else if ( content == m_mainBodyPart ) {
    return;
  }

  // ignore useless stuff such as signatures, certificates, encrypted parts, etc
  else if ( content->contentType()->mimeType() == "application/pgp-signature" ||
            content->contentType()->mimeType() == "application/pkcs7-signature" ||
            content->contentType()->mimeType() == "application/x-pkcs7-signature" ||
            content->contentType()->mimeType() == "application/pgp-encrypted" ||
            content->contentType()->mimeType() == "application/pkcs7-mime" ) 
  {
    return;
  }

  // non plain text main body part, let strigi figure out what to do about that
  else if ( !m_mainBodyPart ) {
    m_mainBodyPart = content;
    //TODO defer to post-processing
    //indexData( res.uri(), content->decodedContent(), item.modificationTime() ); //FIXME index the attachment with strigi but don't force the uri
  }

  // attachment -> delegate to strigi
  else {
    //const KMime::ContentIndex index = content->index();
    //KUrl attachmentUrl = res.uri();
    //attachmentUrl.setHTMLRef( index.toString() );
    //kDebug() << attachmentUrl;
    SimpleResource attachmentRes;
    Nepomuk::NCAL::Attachment attachment( &attachmentRes );
    attachmentRes.addType( Vocabulary::NFO::Attachment() );
    attachmentRes.addType( Vocabulary::NIE::InformationElement() ); //it needs to be an informationElement in order to set nie:description
    attachmentRes.addProperty( Vocabulary::NIE::isPartOf(), res.uri() );
    NepomukFeederUtils::setIcon( "mail-attachment", attachmentRes, graph );
    if ( !content->contentType()->name().isEmpty() )
      attachmentRes.setProperty( Soprano::Vocabulary::NAO::prefLabel(), content->contentType()->name() );
    else if ( content->contentDisposition( false ) && !content->contentDisposition()->filename().isEmpty() )
      attachmentRes.setProperty( Soprano::Vocabulary::NAO::prefLabel(), content->contentDisposition()->filename() );
    if ( content->contentDescription( false ) && !content->contentDescription()->asUnicodeString().isEmpty() )
      attachmentRes.setProperty( Vocabulary::NIE::description(), content->contentDescription()->asUnicodeString() );

    Nepomuk::NMO::Email email(&res);
    email.addHasAttachment( attachmentRes.uri() );
    graph << attachmentRes;
    //TODO defer to post-processing
    //another option would be to let a strigifeeder automatically find unprocessed attachments so we don't have to do anything further here
    //indexData( attachmentUrl, content->decodedContent(), item.modificationTime() ); //FIXME index the attachment with strigi but don't force the uri
  }

}

void NepomukMailFeeder::processFlags(const Akonadi::Item::Flags& flags, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph)
{
  Akonadi::MessageStatus status;
  status.setStatusFromFlags( flags );

  Nepomuk::NMO::Email mail(&res);
  mail.setIsRead( status.isRead() );

  if ( status.isImportant() )
    addTranslatedTag( "important", i18n("Important"), "mail-mark-important" , res, graph );
  if ( status.isToAct() )
    addTranslatedTag( "todo", i18n("To Do"), "mail-mark-task", res, graph );
  if ( status.isWatched() )
    addTranslatedTag( "watched", i18n("Watched"), QString(), res, graph );
}


void NepomukMailFeeder::addTranslatedTag(const char* tagName, const QString& tagLabel, const QString &icon , Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph)
{
  Nepomuk::SimpleResource tagResource = NepomukFeederUtils::addTag( res, graph, QString::fromLatin1( tagName ), tagLabel );
  Nepomuk::NAO::Tag tag( &tagResource );
  if ( !icon.isEmpty() ) {
    NepomukFeederUtils::setIcon( icon, tagResource, graph );
  }
  graph << tagResource;
}

void NepomukMailFeeder::processHeaders(const KMime::Message::Ptr& msg, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph)
{
  Nepomuk::NMO::Email mail(&res);
  if ( msg->subject( false ) ) {
    mail.setMessageSubject( msg->subject()->asUnicodeString() );
    res.setProperty( Soprano::Vocabulary::NAO::prefLabel(), msg->subject()->asUnicodeString() );
  }

  if ( msg->date( false ) ) {
    const QDateTime dateTime = msg->date()->dateTime().dateTime();
    if ( dateTime.isValid() )
      mail.setSentDate( dateTime );
  }

  if ( msg->from( false ) ) {
    mail.setFroms( extractContactsFromMailboxes( msg->from()->mailboxes(), graph ) );
  }

  if ( msg->sender( false ) )
    mail.setSenders( extractContactsFromMailboxes( msg->sender()->mailboxes(), graph ) );

  if ( msg->to( false ) ) {
    mail.setTos( extractContactsFromMailboxes( msg->to()->mailboxes(), graph ) );
  }

  if ( msg->cc( false ) ) {
    mail.setCcs( extractContactsFromMailboxes( msg->cc()->mailboxes(), graph ) );
  }

  if ( msg->bcc( false ) ) {
    mail.setBccs( extractContactsFromMailboxes( msg->bcc()->mailboxes(), graph ) );
  }

  if ( msg->messageID( false ) ) {
    mail.setMessageIds( QStringList( msg->messageID()->asUnicodeString() ) );
  }

  addSpecificHeader( msg, "List-Id",mail, graph );
  addSpecificHeader( msg, "X-Loop",mail, graph );
  addSpecificHeader( msg, "X-Mailing-List", mail, graph );
  addSpecificHeader( msg, "X-Spam-Flag", mail, graph );
  addSpecificHeader( msg, "Organization", mail, graph );
}

void NepomukMailFeeder::addSpecificHeader( const KMime::Message::Ptr& msg, const QByteArray& headerName, Nepomuk::NMO::Email& mail, Nepomuk::SimpleResourceGraph& graph )
{
  if ( msg->headerByType( headerName ) ) {
    Nepomuk::SimpleResource headerRes;
    Nepomuk::NMO::MessageHeader header(&headerRes);
    header.setHeaderName(headerName);
    header.setHeaderValue( msg->headerByType( headerName )->asUnicodeString());
    graph << headerRes;
    mail.addMessageHeader(headerRes.uri());
  }
}

QList<QUrl> NepomukMailFeeder::extractContactsFromMailboxes(const KMime::Types::Mailbox::List& mbs, Nepomuk::SimpleResourceGraph &graph )
{
  QList<QUrl> contacts;
  foreach( const KMime::Types::Mailbox& mbox, mbs ) {
    if ( mbox.hasAddress() ) {
      contacts << NepomukFeederUtils::addContact( QString::fromLatin1( mbox.address() ), mbox.name(), graph ).uri();
    }
  }
  return contacts;
}

 K_PLUGIN_FACTORY(factory, registerPlugin<NepomukMailFeeder>();)
 K_EXPORT_PLUGIN(factory("akonadi_nepomuk_email_feeder"))

}
