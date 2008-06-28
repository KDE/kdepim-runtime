/*
    Copyright (c) 2007 Brad Hards <bradh@frogmouth.net>

    Significant amounts of this code adapted from the openchange client utility,
    which is Copyright (C) Julien Kerihuel 2007 <j.kerihuel@openchange.org>.

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "ocresource.h"

#include <QtCore/QDebug>
#include <QtDBus/QDBusConnection>

#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <KWindowSystem>

#include <boost/shared_ptr.hpp>
#include <libmapi++/libmapi++.h>

#include <kmime/kmime_message.h>
#include <kabc/vcardconverter.h>

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/session.h>

#include "profiledialog.h"


// - Boost.Exception will look for this function if BOOST_NO_EXCEPTIONS is defined.
// - Because we compile with -fnoexception the function doesn't throw the exception.
#ifdef BOOST_NO_EXCEPTIONS
namespace boost 
{
void throw_exception(std::exception const & e)
{
}
}
#endif

typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;
using namespace libmapipp;

OCResource::OCResource( const QString &id ) 
  : ResourceBase( id )
{
  // setName( "OC Resource" );
  // Initialize C mapi library so Profile functions work correctly.
  MAPIInitialize(session::get_default_profile_path().c_str());
}

OCResource::~ OCResource()
{
  if (m_session) delete m_session;
}

bool OCResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED(parts);
  qDebug() << "currently ignoring requestItemDelivery";
  return true;
}


void OCResource::aboutToQuit()
{
  qDebug() << "currently ignoring aboutToQuit()";
}

void OCResource::configure( WId windowId )
{
  ProfileDialog configDialog( this );
  if ( windowId )
    KWindowSystem::setMainWindow( &configDialog, windowId );
  configDialog.exec();
}

void OCResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& )
{
  qDebug() << "currently ignoring itemAdded()";
}

void OCResource::itemChanged( const Akonadi::Item&, const QSet<QByteArray>& )
{
  qDebug() << "currently ignoring itemChanged()";
}

void OCResource::itemRemoved(const Akonadi::Item & item)
{
  qDebug() << "currently ignoring itemRemoved()";
}

void OCResource::getChildFolders( folder& mapi_folder,
                                  const Akonadi::Collection &parentCollection,
                                  Akonadi::Collection::List &collections)
{
  folder::hierarchy_container_type hierarchy = mapi_folder.fetch_hierarchy();

  for (unsigned int i = 0; i < hierarchy.size(); ++i) {
    property_container folderProperty = hierarchy[i]->get_property_container();
    folderProperty << PR_DISPLAY_NAME << PR_FOLDER_CHILD_COUNT << PR_CONTAINER_CLASS;
    folderProperty.fetch();

    Collection thisFolder;
    thisFolder.setParent(parentCollection);
    thisFolder.setRemoteId(QString::number( hierarchy[i]->get_id() ));
    thisFolder.setName( (const char* ) folderProperty[PR_DISPLAY_NAME] );
    QStringList folderMimeType;
    if (*((uint32_t*) folderProperty[PR_FOLDER_CHILD_COUNT]) > 0) {
      // if t has children, needs this mimetype:
      folderMimeType << "inode/directory";
      getChildFolders(*hierarchy[i], thisFolder, collections);
    } else {
      if (folderProperty[PR_CONTAINER_CLASS] == NULL) {
        qDebug() << "failed query for container class for " << thisFolder.name() << ", assuming rfc822 mailbox" << endl;
        folderMimeType << "message/rfc822";
      } else {
        folderMimeType << mimeTypeForFolderType( (const char*) folderProperty[PR_CONTAINER_CLASS]);
        qDebug() << "Folder " << thisFolder.name() << ", as mimetype " << folderMimeType << endl;
      }
    }
    thisFolder.setContentMimeTypes(folderMimeType);
    collections.append(thisFolder);
  }
}

QString OCResource::mimeTypeForFolderType( const char* folderTypeValue ) const
{
  QString folderType( folderTypeValue );
  if ( folderType == IPF_APPOINTMENT ) {
    return QString( "text/calendar" );
  } else if ( folderType == IPF_CONTACT ) {
    return QString( "text/vcard" );
  } else if ( folderType == IPF_JOURNAL ) {
    // TODO: find a better mime type for this
    return QString( "message/directory" );
  } else if ( folderType == IPF_NOTE ) {
    return QString( "message/rfc822" );
  } else if ( folderType == IPF_STICKYNOTE ) {
    // TODO: find a better mime type for this
    return QString( "message/directory" );
  } else if ( folderType == IPF_TASK ) {
    // TODO: find a better mime type for this
    return QString( "message/directory" );
  }

  qDebug() << "Unexpected result for container class: " << folderTypeValue;
  return QString( "message/x-unknown" );
}

void OCResource::login()
{
  const char *profName;
  enum MAPISTATUS retval;
  retval = GetDefaultProfile(&profName);
  if (retval != MAPI_E_SUCCESS) {
    mapi_errstr("GetDefaultProfile", GetLastError());
    exit (1);
  }
  MAPIUninitialize();

  m_profileName = profName;
  try {
  // TODO: handle password (third argument)
  m_session = new session(profName);
  }
  catch(mapi_exception e)
  {
    qDebug() << "MAPI EXception: " << e.what();
    emit error(e.what());
  }
}

void OCResource::retrieveCollections()
{
  qDebug() << "retrieving collections";

  login();

  Collection::List collections;

  /* Retrieve the mailbox folder name */
  property_container storeProperties = m_session->get_message_store().get_property_container();
  storeProperties << PR_DISPLAY_NAME;
  storeProperties.fetch();

  if ( *(storeProperties.begin()) == NULL ) {
    qDebug() << "null mailbox name";
    exit( 1 );
  } else {
    // qDebug() << "mailbox name: " << QString( (const char*) *(storeProperties.begin()) );
  }

  Collection account;
  account.setParent( Collection::root() );
  account.setRemoteId( m_profileName );
  account.setName( QString( (const char*) *(storeProperties.begin()) ) + QString( " (OpenChange)" ) );
  QStringList mimeTypes;
  mimeTypes << "inode/directory";
  account.setContentMimeTypes( mimeTypes );
  collections.append( account );

  /* Prepare the directory listing */
  message_store& store = m_session->get_message_store();
  mapi_id_t topFolderId = store.get_default_folder(olFolderTopInformationStore);
  folder top_folder(store, topFolderId);

  getChildFolders( top_folder, account, collections );

  collectionsRetrieved( collections );
}

static KDateTime convertSysTime(const FILETIME& filetime)
{
  NTTIME nt_time = filetime.dwHighDateTime;
  nt_time = nt_time << 32;
  nt_time |= filetime.dwLowDateTime;
  KDateTime kdeTime;
  kdeTime.setTime_t( nt_time_to_unix( nt_time ) );
  // qDebug() << "time: " << kdeTime.toString();
  return kdeTime;
}

void OCResource::appendMessageToCollection( libmapipp::message& mapi_message, const Akonadi::Collection & collection)
{
  property_container message_properties = mapi_message.get_property_container();

  message_properties.fetch_all();

  MessagePtr msg_ptr ( new KMime::Message );
  for ( property_container::iterator Iter = message_properties.begin(); Iter != message_properties.end(); ++Iter ) {
    switch( Iter.get_tag() ) {
    case PR_ALTERNATE_RECIPIENT_ALLOWED:
      // PT_BOOLEAN
      break;
    case PR_IMPORTANCE:
      // PT_LONG
      break;
    case PR_MESSAGE_CLASS:
      // PT_STRING8
      break;
    case PR_ORIGINATOR_DELIVERY_REPORT_REQUESTED:
      // PT_BOOLEAN
      break;
    case PR_PRIORITY:
      // PT_LONG
      break;
    case PR_READ_RECEIPT_REQUESTED:
      // PT_BOOLEAN
      break;
    case PR_SENSITIVITY:
      // PT_LONG
      break;
    case PR_CLIENT_SUBMIT_TIME:
      // PT_SYSTIME
      msg_ptr->date()->setDateTime( convertSysTime( *((const FILETIME*) *Iter) ) );
      break;
    case PR_SENT_REPRESENTING_SEARCH_KEY:
      // PT_BINARY
      break;
    case PR_RECEIVED_BY_ENTRYID:
      // PT_BINARY
      break;
    case PR_RECEIVED_BY_NAME:
      // PT_STRING8
      qDebug() << "Received by:" << (const char*) *Iter;
      break;
    case PR_SENT_REPRESENTING_ENTRYID:
      // PT_BINARY
      break;
    case PR_SENT_REPRESENTING_NAME:
      // PT_STRING8
      msg_ptr->from()->from7BitString( (const char*) *Iter );
      break;
    case PR_RCVD_REPRESENTING_ENTRYID:
      // PT_BINARY
      break;
    case PR_RCVD_REPRESENTING_NAME:
      // PT_STRING8
      qDebug() << "Representing name:" << (const char*) *Iter;
      break;
    case PR_MESSAGE_SUBMISSION_ID:
      // PR_BINARY
      break;
    case PR_RECEIVED_BY_SEARCH_KEY:
      // PT_BINARY
      break;
    case PR_RCVD_REPRESENTING_SEARCH_KEY:
      // PT_BINARY
      break;
    case PR_MESSAGE_TO_ME:
      // PT_BOOLEAN
      break;
    case PR_MESSAGE_CC_ME:
      // PT_BOOLEAN
      break;
    case PR_MESSAGE_RECIP_ME:
      // PT_BOOLEAN
      break;
    case PR_SENT_REPRESENTING_ADDRTYPE:
      // PT_STRING8
      break;
    case PR_SENT_REPRESENTING_EMAIL_ADDRESS:
      // PT_STRING8
      qDebug() << "Sent representing email addr:" << (const char*) *Iter;
      break;
    case PR_CONVERSATION_INDEX:
      // PT_BINARY
      break;
    case PR_RECEIVED_BY_ADDRTYPE:
      // PT_STRING8
      break;
    case PR_RECEIVED_BY_EMAIL_ADDRESS:
      // PT_STRING8
      break;
    case PR_RCVD_REPRESENTING_ADDRTYPE:
      // PT_STRING8
      break;
    case PR_RCVD_REPRESENTING_EMAIL_ADDRESS:
      // PT_STRING8
      break;
    case PR_TRANSPORT_MESSAGE_HEADERS:
      // PT_STRING8
      break;
    case PR_SENDER_ENTRYID:
      // PT_BINARY
      break;
    case PR_SENDER_NAME:
      // PT_STRING8
      msg_ptr->from()->from7BitString( (const char*) *Iter );
      break;
    case PR_SENDER_SEARCH_KEY:
      // PT_BINARY
      break;
    case PR_SENDER_ADDRTYPE:
      // PT_STRING8
      break;
    case PR_SENDER_EMAIL_ADDRESS:
      // PT_STRING8
      qDebug() << "Sender email addr:" << (const char*) *Iter;
      break;
    case PR_DELETE_AFTER_SUBMIT:
      // PT_BOOLEAN
      break;
    case PR_DISPLAY_BCC:
      // PT_STRING8
      msg_ptr->bcc()->from7BitString( (const char*) *Iter );
      break;
    case PR_DISPLAY_CC:
      // PT_STRING8
      msg_ptr->cc()->from7BitString( (const char*) *Iter );
      break;
    case PR_DISPLAY_TO:
      // PT_STRING8
      // This (and BCC+CC) need to be split on semicolons, and combined with
      // the email address
      msg_ptr->to()->from7BitString( (const char*) *Iter );
      break;
    case PR_MESSAGE_DELIVERY_TIME:
      // PT_SYSTIME
      break;
    case PR_MESSAGE_FLAGS:
      // PT_LONG
      break;
    case PR_MESSAGE_SIZE:
      // PT_LONG
      qDebug() << "Message size";
      break;
    case PR_HASATTACH:
      // PT_BOOLEAN
      break;
    case PR_RTF_IN_SYNC:
      // PT_BOOLEAN
      break;
    case PR_INTERNET_ARTICLE_NUMBER:
      // PT_LONG
      break;
    case PR_NT_SECURITY_DESCRIPTOR:
      // PT_BINARY
      break;
    case PR_URL_COMP_NAME_POSTFIX:
      // PT_LONG
      break;
    case PR_URL_COMP_NAME_SET:
      // PT_BOOLEAN
      break;
    case PR_TRUST_SENDER:
      // PT_LONG
      break;
    case PR_ACCESS:
      // PT_LONG
      break;
    case PR_ACCESS_LEVEL:
      // PT_LONG
      break;
    case PR_BODY:
      // PT_STRING8
      // This probably needs to construct some kind of multipart MIME body
      if ( msg_ptr->body().isEmpty() ) {
	msg_ptr->setBody( (const char*) *Iter );
      }
      break;
    case PR_BODY_UNICODE:
      // PT_UNICODE
      break;
    case PR_BODY_ERROR:
      // PT_ERROR
      break;
    case PR_RTF_COMPRESSED_ERROR:
      // PT_ERROR
      break;
    case PR_HTML:
      // PT_BINARY
      // This probably needs to construct some kind of multipart MIME body
      if ( msg_ptr->body().isEmpty() ) {
	struct SBinary_short *html = (struct SBinary_short *) *Iter;
	QByteArray body = QByteArray( ( char* ) html->lpb, html->cb );
	body.append( "\n" );
	msg_ptr->setBody( body );
      }
      break;
    case PR_INTERNET_MESSAGE_ID:
      // PT_STRING8
      break;
    case PR_DISABLE_FULL_FIDELITY:
      // PT_BOOLEAN
      break;
    case PR_URL_COMP_NAME:
      // PT_STRING8
      break;
    case PR_ATTR_HIDDEN:
      // PT_BOOLEAN
      break;
    case PR_ATTR_SYSTEM:
      // PT_BOOLEAN
      break;
    case PR_ATTR_READONLY:
      // PT_BOOLEAN
      break;
    case PR_CREATION_TIME:
      // PT_SYSTIME
      if ( msg_ptr->date()->isEmpty() ) {
	// we want to use PR_CLIENT_SUBMIT_TIME, but this will do instead
	msg_ptr->date()->setDateTime( convertSysTime( *((const FILETIME*) *Iter) ) );
      }
      break;
    case PR_LAST_MODIFICATION_TIME:
      // PT_SYSTIME
      break;
    case PR_SEARCH_KEY:
      // PT_BINARY
      break;
    case PR_INTERNET_CPID:
      // PT_LONG
      break;
    case PR_MESSAGE_LOCALE_ID:
      // PT_LONG
      break;
    case PR_CREATOR_NAME:
      // PT_STRING8
      break;
    case PR_CREATOR_ENTRYID:
      // PT_BINARY
      break;
    case PR_LAST_MODIFIER_NAME:
      // PT_STRING8
      break;
    case PR_LAST_MODIFIER_ENTRYID:
      // PT_BINARY
      break;
    case PR_MESSAGE_CODEPAGE:
      // PT_LONG
      break;
    case  PR_SENDER_FLAGS:
      // PT_LONG
      break;
    case PR_SENT_REPRESENTING_FLAGS:
      // PT_LONG
      break;
    case PR_RCVD_BY_FLAGS:
      // PT_LONG
      break;
    case PR_RCVD_REPRESENTING_FLAGS:
      // PT_LONG
      break;
    case PR_CREATOR_FLAGS_ERROR:
      // PT_ERROR
      break;
    case PR_MODIFIER_FLAGS_ERROR:
      // PT_ERROR
      break;
    case PR_INET_MAIL_OVERRIDE_FORMAT:
      // PT_LONG
      break;
    case PR_MSG_EDITOR_FORMAT:
      // PT_LONG
      break;
    case PR_DOTSTUFF_STATE:
      // PT_LONG
      break;
    case PR_SOURCE_KEY:
      // PT_BINARY
      break;
    case PR_CHANGE_KEY:
      // PT_BINARY
      break;
    case PR_PREDECESSOR_CHANGE_LIST:
      // PT_BINARY
      break;
    case PR_HAS_NAMED_PROPERTIES:
      // PT_BOOLEAN
      break;
    case PR_ICS_CHANGE_KEY:
      // PT_BINARY
      break;
    case PR_INTERNET_CONTENT_ERROR:
      // PT_ERROR
      break;
    case PR_LOCALE_ID:
      // PT_LONG
      break;
    case PR_INTERNET_PARSE_STATE:
      // PT_BINARY
      break;
    case PR_INTERNET_MESSAGE_INFO:
      // PT_BINARY
      break;
    case PR_URL_NAME:
      // PR_STRING8
      break;
    case PR_LOCAL_COMMIT_TIME:
      // PT_SYSTIME
      break;
    case PR_INTERNET_FREE_DOC_INFO:
      // PT_BINARY
      break;
    case PR_CONVERSATION_TOPIC:
      msg_ptr->subject()->from7BitString( (const char*) *Iter );
      break;
    case 0x82f1001e:
      qDebug() << "82f1001e:" << (const char*) *Iter;
      break;
    case 0x82f2001e:
      qDebug() << "82f2001e:" << (const char*) *Iter;
      break;
    case 0xe28001e:
      qDebug() << "0xe28001e:" << (const char*) *Iter;
      break;
    case 0xe29001e:
      qDebug() << "0xe29001e:" << (const char*) *Iter;
      break;
    case 0x8001001e:
      qDebug() << "0x8001001e:" << (const char*) *Iter;
      break;
    default:
      qDebug() << "Unhandled: " << QString::number( Iter.get_tag(), 16 );
    }
  }

  uint8_t *has_attach = (uint8_t *) message_properties[PR_HASATTACH];

  /* If we have attachments, retrieve them */
  if (has_attach && *has_attach) {
    // TODO
  }

  Item item;
  item.setRemoteId( "itemnumber" );
  item.setMimeType( "message/rfc822" );
  msg_ptr->from()->from7BitString( "from@someone.local" );
  item.setPayload<MessagePtr>( msg_ptr );
  ItemCreateJob *append = new ItemCreateJob( item, collection );
  if ( !append->exec() ) {
    emit percent( 0 );
    emit status( Broken, QString( "Appending new message failed: %1" ).arg( append->errorString() ) );
  }
}

void OCResource::appendContactToCollection( libmapipp::message& mapi_message, const Akonadi::Collection & collection)
{
  property_container message_properties = mapi_message.get_property_container();
  message_properties.fetch_all();

  KABC::Addressee *contact = new KABC::Addressee;

  Item item;
  item.setRemoteId( "itemnumber" );
  item.setMimeType( "text/vcard" );

  KABC::Address workAddress;
  workAddress.setType( KABC::Address::Work );
  KABC::Address homeAddress;
  homeAddress.setType( KABC::Address::Home );
  for (property_container::iterator Iter = message_properties.begin(); Iter != message_properties.end(); ++Iter ) {
    switch( Iter.get_tag() ) {
    case PR_ALTERNATE_RECIPIENT_ALLOWED:
      // PT_BOOLEAN
      if ( *((const uint8_t*) *Iter) ) {
	contact->setSecrecy( KABC::Secrecy( KABC::Secrecy::Public) );
      } else {
	contact->setSecrecy( KABC::Secrecy( KABC::Secrecy::Private) );
      }
      break;
    case PR_IMPORTANCE:
      // PT_LONG
      qDebug() << "PR_IMPORTANCE:" << *((const uint32_t*) *Iter) << QString("(0x%1)").arg(*((const uint32_t*)*Iter), (int)4, (int)16, QChar('0') );
      break;
    case PR_MESSAGE_CLASS:
      // PT_STRING8
      qDebug() << "PR_MESSAGE_CLASS:" << (const char*) *Iter;
      break;
    case PR_ORIGINATOR_DELIVERY_REPORT_REQUESTED:
      // PT_BOOLEAN
      qDebug() << "PR_ORIGINATOR_DELIVERY_REPORT_REQUESTED:" << ( *((const uint8_t*) *Iter) ? "true":"false" );
      break;
    case PR_READ_RECEIPT_REQUESTED:
      // PT_BOOLEAN
      qDebug() << "PR_READ_RECEIPT_REQUESTED:" << ( *((const uint8_t*) *Iter) ? "true":"false" );
      break;
    case PR_SENT_REPRESENTING_NAME:
      // PT_STRING8
      qDebug() << "PR_SENT_REPRESENTING_NAME:" << (const char*) *Iter;
      break;
    case PR_SENT_REPRESENTING_ADDRTYPE:
      // PT_STRING8
      qDebug() << "PR_SENT_REPRESENTING_ADDRTYPE:" << (const char*) *Iter;
      break;
    case PR_SENT_REPRESENTING_EMAIL_ADDRESS:
      // PT_STRING8
      qDebug() << "PR_SENT_REPRESENTING_EMAIL_ADDRESS:" << (const char*) *Iter;
      break;
    case PR_CONVERSATION_TOPIC:
      // PT_STRING8
      qDebug() << "PR_CONVERSATION_TOPIC:" << (const char*) *Iter;
      break;
    case PR_SENDER_NAME:
      // PT_STRING8
      qDebug() << "PR_SENDER_NAME:" << (const char*) *Iter;
      break;
    case PR_SENDER_ADDRTYPE:
      // PT_STRING8
      qDebug() << "PR_SENDER_ADDRTYPE:" << (const char*) *Iter;
      break;
    case PR_SENDER_EMAIL_ADDRESS:
      // PT_STRING8
      qDebug() << "PR_SENDER_EMAIL_ADDRESS:" << (const char*) *Iter;
      break;
    case PR_DELETE_AFTER_SUBMIT:
      // PT_BOOLEAN
      qDebug() << "PR_DELETE_AFTER_SUBMIT:" << ( *((const uint8_t*) *Iter) ? "true":"false" );
      break;
    case PR_DISPLAY_BCC:
      // PT_STRING8
      qDebug() << "PR_DISPLAY_BCC:" << (const char*) *Iter;
      break;
    case PR_DISPLAY_CC:
      // PT_STRING8
      qDebug() << "PR_DISPLAY_CC:" << (const char*) *Iter;
      break;
    case PR_DISPLAY_TO:
      // PT_STRING8
      qDebug() << "PR_DISPLAY_TO:" << (const char*) *Iter;
      break;
    case PR_HASATTACH:
    {
      // PT_BOOLEAN
      qDebug() << "PR_HASATTACH:" << ( *((const uint8_t*) *Iter) ? "true":"false" );

      message::attachment_container_type attachments = mapi_message.fetch_attachments();

      qDebug() << "Num attachments:" << attachments.size();

      if (!attachments.size()) {
        continue;
      }

      // TODO: We always use the first attachment. Do we need to check for other attachments?
      property_container attachment_properties = attachments[0]->get_property_container();
      attachment_properties.fetch_all();

      QString attachFilename = QString( (const char*) attachment_properties[PR_ATTACH_FILENAME] );

      if ( attachFilename.isEmpty() ) {
        attachFilename = QString( (const char*) attachment_properties[PR_ATTACH_LONG_FILENAME] );
      }

      qDebug() << "attachment Filename:" << attachFilename;

      const uint32_t *attachSize = (const uint32_t *) attachment_properties[PR_ATTACH_SIZE];
      qDebug() << "file size: " << *attachSize;

      if ( attachFilename == "ContactPicture.jpg" ) {
        struct SBinary *attachData = (struct SBinary*) attachment_properties[PR_ATTACH_DATA_BIN];
        QImage photoData = QImage::fromData( (const uchar*)attachData->lpb, attachData->cb );
          if ( !photoData.isNull() ) {
            qDebug() << "Photo: " << photoData.size();
            contact->setPhoto( KABC::Picture( photoData) );
          }
      }

      break;
    }
    case PR_RTF_IN_SYNC:
      // PT_BOOLEAN
      qDebug() << "PR_RTF_IN_SYNC:" << ( *((const uint8_t*) *Iter) ? "true":"false" );
      break;
    case PR_URL_COMP_NAME_SET:
      // PT_BOOLEAN
      qDebug() << "PR_URL_COMP_NAME_SET:" << ( *((const uint8_t*) *Iter) ? "true":"false" );
      break;
    case PR_RTF_COMPRESSED:
      // PT_BINARY
      qDebug() << "PR_RTF_COMPRESSED size:" << ((const SBinary*) *Iter)->cb;
      qDebug() << "PR_RTF_COMPRESSED data:" << QByteArray( (char*)((const SBinary*) *Iter)->lpb, ((const SBinary*) *Iter)->cb ).toHex();
      break;
    case PR_URL_COMP_NAME:
      // PT_STRING8
      qDebug() << "PR_URL_COMP_NAME:" << (const char*) *Iter;
      break;
    case PR_ATTR_HIDDEN:
      // PT_BOOLEAN
      qDebug() << "PR_ATTR_HIDDEN:" << ( *((uint8_t*) *Iter) ? "true":"false" );
      break;
    case PR_ATTR_SYSTEM:
      // PT_BOOLEAN
      qDebug() << "PR_ATTR_SYSTEM:" << ( *((const uint8_t*) *Iter) ? "true":"false" );
      break;
    case PR_ATTR_READONLY:
      // PT_BOOLEAN
      qDebug() << "PR_ATTR_READONLY:" << ( *((const uint8_t*) *Iter) ? "true":"false" );
      break;
    case PR_DISPLAY_NAME:
      // PT_STRING8
      contact->setNameFromString( (const char*) *Iter );
      break;
    case PR_ACCOUNT:
      // PT_STRING8
      qDebug() << "PR_ACCOUNT:" << (const char*) *Iter;
      break;
    case PR_OFFICE_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Work ) );
      break;
    case PR_CALLBACK_TELEPHONE_NUMBER:
      // PT_STRING8
      qDebug() << "PR_CALLBACK_TELEPHONE_NUMBER:" << (const char*) *Iter;
      // contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Work ) );
      break;
    case PR_HOME_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Home ) );
      break;
    case PR_INITIALS:
      // PT_STRING8
      qDebug() << "PR_INITIALS:" << (const char*) *Iter;
      break;
    case PR_KEYWORD:
      // PT_STRING8
      qDebug() << "PR_KEYWORD:" << (const char*) *Iter;
      break;
    case PR_LANGUAGE:
      // PT_STRING8
      qDebug() << "PR_LANGUAGE:" << (const char*) *Iter;
      break;
    case PR_LOCATION:
      // PT_STRING8
      qDebug() << "PR_LOCATION:" << (const char*) *Iter;
      break;
    case PR_SURNAME:
      // PT_STRING8
      contact->setFamilyName( (const char*) *Iter );
      break;
    case PR_GENERATION:
      // PT_STRING8
      // This is a bit of a stretch.
      contact->setSuffix( (const char*) *Iter );
      break;
    case PR_GIVEN_NAME:
      // PT_STRING8
      contact->setGivenName( (const char*) *Iter );
      break;
    case PR_POSTAL_ADDRESS:
      // PT_STRING8
      qDebug() << "PR_POSTAL_ADDRESS:" << (const char*) *Iter;
      break;
    case PR_COMPANY_NAME:
      // PT_STRING8
      contact->setOrganization( (const char*) *Iter );
      break;
    case PR_TITLE:
      // PT_STRING8
      contact->setRole( (const char*) *Iter );
      break;
    case PR_OFFICE_LOCATION:
      // PT_STRING8
      contact->insertCustom( "KADDRESSBOOK", "Office", (const char*) *Iter );
      break;
    case PR_PRIMARY_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Pref ) );
      break;
    case PR_OFFICE2_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Work ) );
      break;
    case PR_RADIO_TELEPHONE_NUMBER:
      // ignored
      break;
    case PR_PAGER_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Pager ) );
      break;
    case PR_TELEX_NUMBER:
      // ignored
      break;
    case PR_HOME2_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Home ) );
      break;
    case PR_DEPARTMENT_NAME:
      // PT_STRING8
      // FIXME: this probably shouldn't be required. kdepimlibs needs to be cleaned up for KDE5.
      contact->setDepartment( (const char*) *Iter );
      contact->insertCustom( "KADDRESSBOOK", "Department", (const char*) *Iter );
      break;
    case PR_MOBILE_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Cell ) );
      break;
    case PR_BUSINESS_FAX_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Work | KABC::PhoneNumber::Fax ) );
      break;
    case PR_COUNTRY:
      // PT_STRING8
      workAddress.setCountry( (const char*) *Iter );
      break;
    case PR_LOCALITY:
      // PT_STRING8
      workAddress.setLocality( (const char*) *Iter );
      break;
    case PR_STREET_ADDRESS :
      // PT_STRING8
      workAddress.setStreet( (const char*) *Iter );
      break;
    case PR_STATE_OR_PROVINCE:
      // PT_STRING8
      workAddress.setRegion( (const char*) *Iter );
      break;
    case PR_POSTAL_CODE:
      // PT_STRING8
      workAddress.setPostalCode( (const char*) *Iter );
      break;
    case PR_POST_OFFICE_BOX:
      // PT_STRING8
      workAddress.setPostOfficeBox( (const char*) *Iter );
      break;
    case PR_ISDN_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Isdn ) );
      break;
    case PR_CAR_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Car ) );
      break;
    case PR_OTHER_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Voice ) );
      break;
    case PR_HOME_FAX_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Fax | KABC::PhoneNumber::Home ) );
      break;
    case PR_PRIMARY_FAX_NUMBER:
      // PT_STRING8
      qDebug() << "PR_PRIMARY_FAX_NUMBER:" << (const char*) *Iter;
      break;
    case PR_ASSISTANT:
      // PT_STRING8
      contact->insertCustom( "KADDRESSBOOK", "AssistantName", (const char*) *Iter );
      break;
    case PR_WEDDING_ANNIVERSARY:
      // PT_SYSTIME
      contact->insertCustom( "KADDRESSBOOK", "Anniversary", convertSysTime( *((const FILETIME*) *Iter) ).toString("%Y-%m-%d") );
      break;
    case PR_BIRTHDAY:
      // PT_SYSTIME
      contact->setBirthday( convertSysTime( *((const FILETIME*) *Iter) ).toUtc().dateTime() );
      break;
    case PR_MIDDLE_NAME:
      // PT_STRING8
      contact->setAdditionalName( (const char*) *Iter );
      break;
    case PR_DISPLAY_NAME_PREFIX:
      // PT_STRING8
      contact->setPrefix( (const char*) *Iter );
      break;
    case PR_PROFESSION:
      // PT_STRING8
      contact->insertCustom( "KADDRESSBOOK", "Profession", (const char*) *Iter );
      break;
    case PR_SPOUSE_NAME:
      // PT_STRING8
      contact->insertCustom( "KADDRESSBOOK", "SpouseName", (const char*) *Iter );
      break;
    case PR_TTYTDD_PHONE_NUMBER:
      // ignored
      break;
    case PR_MANAGER_NAME:
      // PT_STRING8
      contact->insertCustom( "KADDRESSBOOK", "Manager", (const char*) *Iter );
      break;
    case PR_NICKNAME:
      // PT_STRING8
      contact->setNickName( (const char*) *Iter );
      break;
    case PR_BUSINESS_HOME_PAGE:
      // PT_STRING8
      contact->setUrl( KUrl( (const char*) *Iter ) );
      break;
    case PR_COMPANY_MAIN_PHONE_NUMBER:
      // PT_STRING8
      qDebug() << "PR_COMPANY_MAIN_PHONE_NUMBER:" << (const char*) *Iter;
      break;
    case PR_HOME_ADDRESS_CITY:
      // PT_STRING8
      homeAddress.setLocality( (const char*) *Iter );
      break;
    case PR_HOME_ADDRESS_COUNTRY:
      // PT_STRING8
      homeAddress.setCountry( (const char*) *Iter );
      break;
    case PR_HOME_ADDRESS_POSTAL_CODE:
      // PT_STRING8
      homeAddress.setPostalCode( (const char*) *Iter );
      break;
    case PR_HOME_ADDRESS_STATE_OR_PROVINCE:
      // PT_STRING8
      homeAddress.setRegion( (const char*) *Iter );
      break;
    case PR_HOME_ADDRESS_STREET:
      // PT_STRING8
      homeAddress.setStreet( (const char*) *Iter );
      break;
    case PR_CREATOR_NAME:
      // PT_STRING8
      qDebug() << "PR_CREATOR_NAME:" << (const char*) *Iter;
      break;
    case PR_LAST_MODIFIER_NAME:
      // PT_STRING8
      qDebug() << "PR_LAST_MODIFIER_NAME:" << (const char*) *Iter;
      break;
    case PR_CREATOR_ENTRYID:
      // PT_BINARY
      qDebug() << "PR_CREATOR_ENTRYID size:" << ((const SBinary*) *Iter)->cb;
      qDebug() << "PR_CREATOR_ENTRYID data:" << QByteArray( (char*)((const SBinary*) *Iter)->lpb, ((const SBinary*) *Iter)->cb ).toHex();
      break;
    case PR_HAS_NAMED_PROPERTIES:
      // PT_BOOLEAN
      qDebug() << "PR_HAS_NAMED_PROPERTIES:" << ( *((const uint8_t*) *Iter) ? "true":"false" );
      break;
    case PR_URL_NAME:
      // PT_STRING8
      qDebug() << "PR_URL_NAME:" << (const char*) *Iter;
      break;
    case PR_INTERNET_MESSAGE_ID:
      // PT_STRING8
      qDebug() << "PR_INTERNET_MESSAGE_ID:" << (const char*) *Iter;
      break;

    case 0x8001001e:
      // PT_STRING8
      qDebug() << "0x8001001e:" << (const char*) *Iter;
      break;
    case 0x8004000b:
      // PT_BOOLEAN
      qDebug() << "0x8004000b:" << (  *((const uint8_t*) *Iter) ? "true":"false" );
      break;
    case 0x8008101e:
    {
      struct SLPSTRArray value = *((const SLPSTRArray*) *Iter);
      for (unsigned int j=0; j < value.cValues; ++j) {
        qDebug() << "0x8008101e:" << value.strings[j]->lppszA;
      }
      break;
    }
    case 0x800d001e:
      // PT_STRING8
      qDebug() << "0x800d001e:" << (const char*) *Iter;
      break;
    case 0x800e001e:
      // PT_STRING8
      qDebug() << "0x800e001e:" << (const char*) *Iter;
      break;
    case 0x800f001e:
      // PT_STRING8
      qDebug() << "0x800f001e:" << (const char*) *Iter;
      break;
    case 0x8010001e:
      // PT_STRING8
      contact->insertEmail( (const char*) *Iter, true ); // preferred
      break;
    case 0x8011001e:
      // PT_STRING8
      contact->insertEmail( (const char*) *Iter );
      break;
    case 0x8012001e:
      // PT_STRING8
      contact->insertEmail( (const char*) *Iter );
      break;
    case 0x8013001e:
      // PT_STRING8
      qDebug() << "Display as (email):" << (const char*) *Iter;
      break;
    case 0x8014001e:
      // PT_STRING8
      qDebug() << "Display as (email2):" << (const char*) *Iter;
      break;
    case 0x8015001e:
      // PT_STRING8
      qDebug() << "Display as (email3):" << (const char*) *Iter;
      break;
    case 0x8016001e:
      // PT_STRING8
      qDebug() << "0x8016001e:" << (const char*) *Iter;
      break;
    case 0x8019101e:
    {
      struct SLPSTRArray value = *((const SLPSTRArray*) *Iter); 
      for (unsigned int j=0; j < value.cValues; ++j) {
	qDebug() << "0x8019101e:" << value.strings[j]->lppszA;
      }
      break;
    }
    case 0x801f001e:
      // PT_STRING8
      qDebug() << "0x801f001e:" << (const char*) *Iter;
      break;
    case 0x8020001e:
      // PT_STRING8
      qDebug() << "0x8020001e:" << (const char*) *Iter;
      break;
    case 0x8021001e:
      // PT_STRING8
      qDebug() << "0x8021001e:" << (const char*) *Iter;
      break;
    case 0x8022001e:
      // PT_STRING8
      contact->insertCustom( "KADDRESSBOOK", "IMAddress", (const char*) *Iter );
      break;
    case 0x8023001e:
      // PT_STRING8
      qDebug() << "Web page address:" << (const char*) *Iter;
      break;
    case 0x80b7001e:
      // PT_STRING8
      qDebug() << "0x80b7001e:" << (const char*) *Iter;
      break;
    case 0x80bf001e:
      // PT_STRING8
      qDebug() << "0x80b7001e:" << (const char*) *Iter;
      break;
    case 0x80c5001e:
      // PT_STRING8
      qDebug() << "0x80c5001e:" << (const char*) *Iter;
      break;
    case 0x80c9001e:
      // PT_STRING8
      qDebug() << "0x80c9001e:" << (const char*) *Iter;
      break;
    case 0x80cc001e:
      // PT_STRING8
      qDebug() << "0x80cc001e:" << (const char*) *Iter;
      break;
    case 0x80cd001e:
      // PT_STRING8
      qDebug() << "0x80cd001e:" << (const char*) *Iter;
      break;
    case 0x81a7000b:
      // PT_BOOLEAN
      qDebug() << "0x81a7000b:" << ( *((const uint8_t*) *Iter) ? "true":"false" );
      break;
    case 0x81cd001e:
      // PT_STRING8
      qDebug() << "0x81cd001e:" << (const char*) *Iter;
      break;
    case 0x81ce0102:
      // PT_BINARY
      qDebug() << "0x81ce0102 size:" << ((const SBinary*) *Iter)->cb;
      qDebug() << "0x81ce0102 data:" << QByteArray( (char*)((const SBinary*) *Iter)->lpb, ((const SBinary*) *Iter)->cb ).toHex();

      break;
    case 0x81cf001e:
      // PT_STRING8
      qDebug() << "0x81cf001e:" << (const char*) *Iter;
      break;
    case 0x81d00102:
      // PT_BINARY
      qDebug() << "0x81d00102 size:" << ((const SBinary*) *Iter)->cb;
      qDebug() << "0x81d00102 data:" << QByteArray( (char*)((const SBinary*) *Iter)->lpb, ((const SBinary*) *Iter)->cb ).toHex();

      break;
    case 0x81d1001e:
      // PT_STRING8
      qDebug() << "0x81d1001e:" << (const char*) *Iter;
      break;
    case 0x81d20102:
      // PT_BINARY
      qDebug() << "0x81d20102 size:" << ((const SBinary*) *Iter)->cb;
      qDebug() << "0x81d20102 data:" << QByteArray( (char*)((const SBinary*) *Iter)->lpb, ((const SBinary*) *Iter)->cb ).toHex();
      break;
    case 0x827c001e:
      // PT_STRING8
      qDebug() << "0x827c001e:" << (const char*) *Iter;
      break;
    case 0x827f000b:
      // PT_BOOLEAN
      qDebug() << "0x827f000b:" << ( *((const uint8_t*) *Iter) ? "true":"false" );
      break;
    case 0x8281001e:
      // PT_STRING8
      qDebug() << "0x8281001e:" << (const char*) *Iter;
      break;
    case 0x8282001e:
      // PT_STRING8
      qDebug() << "0x8282001e:" << (const char*) *Iter;
      break;
    case 0x8284001e:
      // PT_STRING8
      qDebug() << "0x8284001e:" << (const char*) *Iter;
      break;
    case 0x82b9000b:
      // PT_BOOLEAN
      qDebug() << "0x82b9000b:" << ( *((const uint8_t*) *Iter) ? "true":"false" );
      break;
    case 0x82ef000b:
      // PT_BOOLEAN
      qDebug() << "0x82ef000b:" << ( *((const uint8_t*) *Iter) ? "true":"false" );
      break;
    case 0x82f5001e:
      // PT_STRING8
      qDebug() << "0x82f5001e:" << (const char*) *Iter;
      break;
    case 0x82f60102:
      // PT_BINARY
      qDebug() << "0x82f60102 size:" << ((const SBinary*) *Iter)->cb;
      qDebug() << "0x82f60102 data:" << QByteArray( (char*)((const SBinary*) *Iter)->lpb, ((const SBinary*) *Iter)->cb ).toHex();
      break;
    case 0x82f70102:
      // PT_BINARY
      qDebug() << "0x82f70102 size:" << ((const SBinary*) *Iter)->cb;
      qDebug() << "0x82f70102 data:" << QByteArray( (char*)((const SBinary*) *Iter)->lpb, ((const SBinary*) *Iter)->cb ).toHex();
      break;
    case 0x8386001e:
      // PT_STRING8
      qDebug() << "0x8386001e:" << (const char*) *Iter;
      break;
    case 0x8387001e:
      // PT_STRING8
      qDebug() << "0x8387001e:" << (const char*) *Iter;
      break;
    case 0x8388001e:
      // PT_STRING8
      qDebug() << "0x8388001e:" << (const char*) *Iter;
      break;
    case 0x83890102:
      // PT_BINARY
      qDebug() << "0x83890102 size:" << ((const SBinary*) *Iter)->cb;
      qDebug() << "0x83890102 data:" << QByteArray( (char*)((const SBinary*) *Iter)->lpb, ((const SBinary*) *Iter)->cb ).toHex();
      break;
    case 0x838e001e:
      // PT_STRING8
      qDebug() << "0x838e001e:" << (const char*) *Iter;
      break;
    case 0x838f001e:
      // PT_STRING8
      qDebug() << "0x838f001e:" << (const char*) *Iter;
      break;
    case 0x8390001e:
      // PT_STRING8
      qDebug() << "0x8390001e:" << (const char*) *Iter;
      break;
    case 0x83910102:
      // PT_BINARY
      qDebug() << "0x83910102 size:" << ((const SBinary*) *Iter)->cb;
      qDebug() << "0x83910102 data:" << QByteArray( (char*)((const SBinary*) *Iter)->lpb, ((const SBinary*) *Iter)->cb ).toHex();
      break;
    case 0x8396001e:
      // PT_STRING8
      qDebug() << "0x8396001e:" << (const char*) *Iter;
      break;
    case 0x8397001e:
      // PT_STRING8
      qDebug() << "0x8397001e:" << (const char*) *Iter;
      break;
    case 0x8398001e:
      // PT_STRING8
      qDebug() << "0x8398001e:" << (const char*) *Iter;
      break;
    case 0x83990102:
      // PT_BINARY
      qDebug() << "0x83990102 size:" << ((const SBinary*) *Iter)->cb;
      qDebug() << "0x83990102 data:" << QByteArray( (char*)((const SBinary*) *Iter)->lpb, ((const SBinary*) *Iter)->cb ).toHex();
      break;

    default:
      qDebug() << "Unhandled in collection: " << QString::number( Iter.get_tag(), 16 );
    }
  }
  contact->insertAddress( workAddress );
  contact->insertAddress( homeAddress );
  item.setPayload<KABC::Addressee>( *contact );
  ItemCreateJob *append = new ItemCreateJob( item, collection );
  if ( !append->exec() ) {
    emit percent( 0 );
    emit status( Broken, QString( "Appending new message failed: %1" ).arg( append->errorString() ) );
  }
}

void OCResource::retrieveItems( const Akonadi::Collection & collection )
{
  qDebug() << "currently synchronizeCollections() is incomplete";

  ItemFetchJob *fetch = new ItemFetchJob( collection );
  if ( !fetch->exec() ) {
    emit status( Broken,
                 QString( "Unable to fetch listing of collection '%1': %2" ).arg( collection.name() ).arg( fetch->errorString() ) );
    return;
  }

  emit percent( 0 );

  Item::List items = fetch->items();

  folder aFolder(m_session->get_message_store(), collection.remoteId().toULongLong());
  folder::message_container_type messages = aFolder.fetch_messages();
  
  for (unsigned int i = 0; i < messages.size(); ++i) {

    if ( collection.contentMimeTypes().contains( "text/vcard" ) ) {
      appendContactToCollection( *messages[i], collection );
    } else {
      appendMessageToCollection( *messages[i], collection );
    }
  }

  // fetchFolder( collection );

  emit percent( 100 );
  itemsRetrievalDone();
}

AKONADI_RESOURCE_MAIN( OCResource )

#include "ocresource.moc"
