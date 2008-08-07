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

#include <QtDBus/QDBusConnection>

#include <QtCore/QDebug>
#include <QtCore/QChar>
#include <QtCore/QLatin1Char>

#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <KWindowSystem>

#include <boost/shared_ptr.hpp>

#include <kmime/kmime_message.h>
#include <kabc/vcardconverter.h>
#include <kcal/incidence.h>
#include <kcal/event.h>

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/session.h>

#include <KMimeType>
#include <QTextCodec>
#include <KCodecs>
#include <KLocale>

// Do not move this header include without compile testing.
// It is here to avoid conflicts between Qt headers and 
// the libmapi headers that get included from the libmapi++
// headers
#include "ocresource.h"

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
typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

using namespace Akonadi;
using namespace libmapipp;
using KMime::Content;

OCResource::OCResource( const QString &id ) 
  : ResourceBase( id )
{
}

OCResource::~OCResource()
{
}

bool OCResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED(parts);

  QStringList ids = item.remoteId().split('-');
  mapi_id_t message_id = ids[0].toULongLong();
  mapi_id_t folder_id = ids[1].toULongLong();

  libmapipp::message* messagePointer = NULL;
  try {
   messagePointer = new libmapipp::message(m_session, folder_id, message_id);
  }
  catch (mapi_exception e) {
    cancelTask(e.what());
  }
  
  Item i;
  i.setRemoteId( item.remoteId());
  if (item.mimeType() == "text/vcard") {
    i.setMimeType( "text/vcard" );
    appendMessageToItem( *messagePointer, i ); 
  } else if ( item.mimeType() == "message/rfc822") {
    i.setMimeType( "message/rfc822" );
    appendMessageToItem( *messagePointer, i );
  } else if ( item.mimeType() == "text/calendar" ) {
    property_container messageProperty = messagePointer->get_property_container();
    messageProperty << PR_MESSAGE_CLASS;
    messageProperty.fetch();
    if ( *messageProperty.begin() && QString( (const char*) *messageProperty.begin() ) == "IPM.Appointment" ) {
      appendEventToItem( *messagePointer, i );
    } else {
      qDebug() << "retrieveItem() not implemented for calendars yet.";
      return true;
    }
  } else {
    cancelTask( "Unknown type of message" );
    return false;
  }
  
  itemRetrieved( i );

  return true;
}


void OCResource::aboutToQuit()
{
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
  // Item id is composed of "messageID-folderID"
  QStringList ids = item.remoteId().split('-');
  mapi_id_t message_id = ids[0].toULongLong();
  mapi_id_t folder_id = ids[1].toULongLong();

  folder aFolder(m_session.get_message_store(), folder_id);

  aFolder.delete_message(message_id);

  changeProcessed();
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
    return QString( "text/calendar" );
  } else if ( folderType == IPF_NOTE ) {
    return QString( "message/rfc822" );
  } else if ( folderType == IPF_STICKYNOTE ) {
    // TODO: find a better mime type for this
    return QString( "message/directory" );
  } else if ( folderType == IPF_TASK ) {
    return QString( "text/calendar" );
  }

  qDebug() << "Unexpected result for container class: " << folderTypeValue;
  return QString( "message/x-unknown" );
}

void OCResource::login()
{
  try {
    // TODO: handle password (third argument)
    m_session.login();
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

  property_container storeProperties = m_session.get_message_store().get_property_container();
  try {
    /* Retrieve the mailbox folder name */
    storeProperties << PR_DISPLAY_NAME;
    storeProperties.fetch();
  }
  catch(std::runtime_error e)
  {
    qDebug() << "MAPI EXception: " << e.what();
    emit error(e.what());
  }

  Collection account;
  account.setParent( Collection::root() );
  account.setRemoteId( m_session.get_profile_name().c_str() );
  account.setName( QString( (const char*) *(storeProperties.begin()) ) + QString( " (OpenChange)" ) );
  QStringList mimeTypes;
  mimeTypes << "inode/directory";
  account.setContentMimeTypes( mimeTypes );
  collections.append( account );

  folder* top_folder= NULL;
  try {
    /* Prepare the directory listing */
    message_store& store = m_session.get_message_store();
    mapi_id_t topFolderId = store.get_default_folder(olFolderTopInformationStore);
    top_folder = new folder(store, topFolderId);
  }
  catch(std::runtime_error e)
  {
    qDebug() << "MAPI EXception: " << e.what();
    emit error(e.what());
  }

  getChildFolders( *top_folder, account, collections );

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

const char * encoding( const QString& data )
{
    static QTextCodec *local = KGlobal::locale()->codecForEncoding();
    qDebug() << "user has: " << local->name();

    if ( local->canEncode( data ) )
        return local->name();
    else
        return "utf-8";
}

/// \brief Returns a resolved MAPI name.
/// \return QString.isNull() == true if name is not found.
QString resolveMapiName(const char* username)
{
  TALLOC_CTX* mem_ctx = talloc_init("OCResource_resolveMapiName_CTX");

  SPropTagArray* tagArray = set_SPropTagArray(mem_ctx, 0x2, PR_SMTP_ADDRESS, PR_SMTP_ADDRESS_UNICODE);
  FlagList* flagList = NULL;
  SRowSet* rowSet = NULL;
  const char* usernames[2] = { username, NULL };
  if (ResolveNames(usernames, tagArray, &rowSet, &flagList, 0) != MAPI_E_SUCCESS) {
    if (ResolveNames(usernames, tagArray, &rowSet, &flagList, MAPI_UNICODE) != MAPI_E_SUCCESS) {
       qDebug() << "*** COULD NOT RESOLVE EXCHANGE EMAIL ADDRESS ***";
    }
  }

  const char* smtpAddress = NULL;
  if (rowSet && rowSet->cRows == 1) {
    smtpAddress = (const char *) find_SPropValue_data(rowSet->aRow, PR_SMTP_ADDRESS);

    if (!smtpAddress)
      smtpAddress = (const char *) find_SPropValue_data(rowSet->aRow, PR_SMTP_ADDRESS_UNICODE);
  }

  talloc_free(mem_ctx);

  return smtpAddress ? QString(smtpAddress) : QString();
}

#define MSG_MIMETYPE_MULTIPART	"multipart/mixed"
#define MSG_MIMETYPE_PLAIN_TEXT	"text/plain"
#define MSG_MIMETYPE_HTML_TEXT	"text/html"

void OCResource::appendMessageToItem( libmapipp::message& mapi_message, Akonadi::Item & item)
{
  property_container message_properties = mapi_message.get_property_container();
  message_properties << PR_CLIENT_SUBMIT_TIME << PR_SENDER_NAME << PR_SENDER_EMAIL_ADDRESS << PR_DISPLAY_BCC << PR_DISPLAY_CC << PR_DISPLAY_TO
                     << PR_MESSAGE_SIZE << PR_HASATTACH << PR_BODY << PR_BODY_UNICODE << PR_HTML << PR_CREATION_TIME << PR_CONVERSATION_TOPIC;

  message_properties.fetch();
  MessagePtr msg_ptr ( new KMime::Message );

  const char* senderNameStr = (const char*) message_properties[PR_SENDER_NAME];
  QString senderName;
  if (senderNameStr) {
    senderName = senderNameStr;
    qDebug() << "senderName: " << senderName;
  }

  const char* senderEmailAddressStr = (const char*) message_properties[PR_SENDER_EMAIL_ADDRESS];
  QString senderEmailAddress;
  if (senderEmailAddressStr) {
    if (!strchr(senderEmailAddressStr, '@')) {
      senderEmailAddress = resolveMapiName(senderEmailAddressStr);
    } else {
      senderEmailAddress = senderEmailAddressStr;
    }

    qDebug() << "senderEmailAddress: " << senderEmailAddress;
  }

  KMime::Headers::From *fromAddress = new KMime::Headers::From();
  if (senderNameStr && senderEmailAddressStr) {
    fromAddress->addAddress( senderEmailAddress.toLatin1(), senderName.toLatin1() );
    msg_ptr->setHeader( fromAddress );
  } else  if (senderEmailAddressStr) {
    fromAddress->addAddress(senderEmailAddress.toLatin1());
    msg_ptr->setHeader( fromAddress );
  } else {
    delete fromAddress;
    qDebug() << "*** DID NOT GET SENDER NAME AND EMAIL ADDRESS ***";
  }

  if (message_properties[PR_DISPLAY_BCC])
    qDebug() << "PR_DISPLAY_BCC: " << (const char*) message_properties[PR_DISPLAY_BCC];

  const char* displayTo = (const char*) message_properties[PR_DISPLAY_TO];
  KMime::Headers::To *toAddress = new KMime::Headers::To();
  if (displayTo && strlen(displayTo) > 0)
  {
    QStringList toUsernames = QString(displayTo).split(';');
    foreach (QString username, toUsernames) {
      username = username.simplified();
      if ( (username.contains('@')) ) {
        toAddress->addAddress(username.toLatin1());
        continue;
      } else {
        QString resolvedName = resolveMapiName(username.toAscii().constData());
        if (!resolvedName.isNull()) {
          toAddress->addAddress(resolvedName.toLatin1(), username.toLatin1());
        }
      }
    }
  }

  if (toAddress->isEmpty()) {
    delete toAddress;
  } else {
    msg_ptr->setHeader(toAddress);
  }

  const char* displayCC = (const char*) message_properties[PR_DISPLAY_CC];
  KMime::Headers::Cc *CCAddress = new KMime::Headers::Cc();
  if (displayCC && strlen(displayCC) > 0)
  {
    QStringList CCUsernames = QString(displayCC).split(';');
    foreach (QString username, CCUsernames) {
      username = username.simplified();
      if ( (username.contains('@')) ) {
        CCAddress->addAddress(username.toLatin1());
        continue;
      } else {
        QString resolvedName = resolveMapiName(username.toAscii().constData());
        if (!resolvedName.isNull()) {
          CCAddress->addAddress(resolvedName.toLatin1(), username.toLatin1());
        }
      }
    }
  }

  if (CCAddress->isEmpty()) {
    delete CCAddress;
  } else {
    msg_ptr->setHeader(CCAddress);
  }

  const char* displayBCC = (const char*) message_properties[PR_DISPLAY_BCC];
  KMime::Headers::Bcc *bccAddress = new KMime::Headers::Bcc();
  if (displayBCC && strlen(displayBCC) > 0)
  {
    QStringList bccUsernames = QString(displayBCC).split(';');
    foreach (QString username, bccUsernames) {
      username = username.simplified();
      if ( (username.contains('@')) ) {
        bccAddress->addAddress(username.toLatin1());
        continue;
      } else {
        QString resolvedName = resolveMapiName(username.toAscii().constData());
        if (!resolvedName.isNull()) {
          bccAddress->addAddress(resolvedName.toLatin1(), username.toLatin1());
        }
      }
    }
  }

  if (bccAddress->isEmpty()) {
    delete bccAddress;
  } else {
    msg_ptr->setHeader(bccAddress);
  }

  // Check if we have any attachments.
  uint8_t *has_attach = (uint8_t *) message_properties[PR_HASATTACH];

  Content* plainContent = NULL;
  const char* plain = (const char*) message_properties[PR_BODY];
  if (!plain)
    plain = (const char*) message_properties[PR_BODY_UNICODE];

  if (plain) {
    qDebug() << "BODY: " << plain;
    plainContent = new Content();
    plainContent->contentType(true)->setMimeType( MSG_MIMETYPE_PLAIN_TEXT );
    plainContent->fromUnicodeString( plain );
  }
  else
    qDebug() << "*** DID NOT GET PLAIN BODY ***";

  Content* htmlContent = NULL;
  SBinary_short *html = (SBinary_short *) message_properties[PR_HTML];
  if ( html ) {
    QByteArray body = QByteArray( ( char* ) html->lpb, html->cb );
    body.append( "\n" );
    htmlContent = new Content();
    htmlContent->contentType(true)->setMimeType( MSG_MIMETYPE_HTML_TEXT );
    htmlContent->setBody(body);
  }


  if (plain && html) {
    qDebug() << "*** Got plain body AND html body ***";
    
    Content* mainContent = new Content();
    mainContent->contentType(true)->setMimeType( "multipart/alternative" );
    mainContent->contentType()->setBoundary( KMime::multiPartBoundary() );
    mainContent->addContent(plainContent);
    mainContent->addContent(htmlContent);
    mainContent->assemble();

     msg_ptr->addContent(mainContent, true);
  } else if (plain) {
    qDebug() << "*** GOT PLAIN BODY ONLY ***";

    msg_ptr->addContent(plainContent, true);
  } else if (html) {
   qDebug() << "*** GOT HTML BODY ONLY ***";


    msg_ptr->addContent(htmlContent, true);
  } 

  /* If we have attachments, retrieve them */
  if (has_attach && *has_attach) {
    message::attachment_container_type attachments = mapi_message.fetch_attachments();
    
    qDebug() << "Num attachments:" << attachments.size();
    for (message::attachment_container_type::iterator Iterator = attachments.begin(); Iterator != attachments.end(); ++Iterator) {
      if (!(*Iterator)->get_data()) {
        continue;
        qDebug() << "*** DID NOT GET ATTACH DATA BIN ***";
      }

      qDebug() << "size of binary data is: " <<  (*Iterator)->get_data_size();
      QByteArray data((const char*) (*Iterator)->get_data(), (*Iterator)->get_data_size());

      if (data.isEmpty() || data.isNull()) {
        qDebug() << "*** ATTACH DATA BIN IS EMPTY OR NULL ***";
        continue;
      }

      // encode attachment (base 64)
      QByteArray final = KCodecs::base64Encode( data, true );
      Content* c = new Content();
      c->setBody( final+"\n\n" );

      QString attachFilename = QString( (*Iterator)->get_filename().c_str() );

      // Add a content type / find mime-type
      KMimeType::Ptr type = KMimeType::findByNameAndContent( attachFilename, data  );
      KMime::Headers::ContentType *ctt= new KMime::Headers::ContentType( c );
      ctt->fromUnicodeString( type->name(),encoding( type->name() ) );
      ctt->setName( attachFilename, "" );
      c->setHeader( ctt );

      // Set the encoding.
      KMime::Headers::ContentTransferEncoding *cte= new KMime::Headers::ContentTransferEncoding( c );
      cte->setEncoding( KMime::Headers::CEbase64 );
      cte->setDecoded( false );
      c->setHeader( cte );
      c->assemble();

 
      msg_ptr->addContent( c );
    }
  }

  const FILETIME* submitTime = (const FILETIME*) message_properties[PR_CLIENT_SUBMIT_TIME];
  if (submitTime)
    msg_ptr->date()->setDateTime(convertSysTime(*submitTime));

  const FILETIME* clientSubmitTime = (const FILETIME*) message_properties[PR_CREATION_TIME];
  if ( !submitTime && clientSubmitTime && msg_ptr->date()->isEmpty() ) {
    // we want to use PR_CLIENT_SUBMIT_TIME, but this will do instead
    msg_ptr->date()->setDateTime( convertSysTime(*clientSubmitTime) );
  }

  const char* conversationTopicStr = (const char*) message_properties[PR_CONVERSATION_TOPIC];
  if (conversationTopicStr) {
    msg_ptr->subject()->from7BitString(conversationTopicStr);
  }

  item.setPayload( msg_ptr );
}

void OCResource::appendContactToItem( libmapipp::message& mapi_message, Akonadi::Item & item)
{
  property_container message_properties = mapi_message.get_property_container();
  message_properties.fetch_all();

  KABC::Addressee *contact = new KABC::Addressee;

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
    case PR_MESSAGE_CLASS:
      // PT_STRING8
      qDebug() << "PR_MESSAGE_CLASS:" << (const char*) *Iter;
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

      property_container attachment_properties = attachments[0]->get_property_container();
      attachment_properties.fetch_all();

      const uint32_t *attachSize = (const uint32_t *) attachment_properties[PR_ATTACH_SIZE];
      qDebug() << "file size: " << *attachSize;

      if ( attachments[0]->get_filename() == "ContactPicture.jpg" ) {
        QImage photoData = QImage::fromData( (const uchar*)attachments[0]->get_data(), attachments[0]->get_data_size() );
          if ( !photoData.isNull() ) {
            qDebug() << "Photo: " << photoData.size();
            contact->setPhoto( KABC::Picture(photoData) );
          }
      }

      break;
    }
    case PR_DISPLAY_NAME:
      // PT_STRING8
      contact->setNameFromString( (const char*) *Iter );
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
    case PR_PAGER_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( (const char*) *Iter, KABC::PhoneNumber::Pager ) );
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
    case 0x8022001e:
      // PT_STRING8
      contact->insertCustom( "KADDRESSBOOK", "IMAddress", (const char*) *Iter );
      break;

    case 0x81b0001e:
      contact->insertEmail((const char*) *Iter);
      break;

    default:

/*    
      if (Iter.get_type() == PT_STRING8 || Iter.get_type() == PT_UNICODE)
        qDebug() << "Unhandled in collection: (" << QString::number( Iter.get_tag(), 16 ) << ") with value: " << (const char*) *Iter;
      else
        qDebug() << "Unhandled in collection: " << QString::number( Iter.get_tag(), 16 );
*/

      break;
    }
  }
  contact->insertAddress( workAddress );
  contact->insertAddress( homeAddress );
  item.setPayload<KABC::Addressee>( *contact );
}

// Declare this here temporarily.
typedef enum {
    olNormal = 0,
    olPersonal = 1,
    olPrivate = 2,
    olConfidential = 3
} OlSensitivity;

KCal::Incidence::Secrecy getIncidentSecrecy (const uint32_t prop)
{
  switch (prop) {
    case olPersonal:
    case olPrivate: 
      return KCal::Incidence::SecrecyPrivate;
			
    case olConfidential: 
      return KCal::Incidence::SecrecyConfidential;
			
    case olNormal: 
    default:
      return KCal::Incidence::SecrecyPublic;
  }
}

void OCResource::appendEventToItem( libmapipp::message & mapi_message, Akonadi::Item & item )
{
  property_container messageProperties = mapi_message.get_property_container();
  // messageProperties << PR_START_DATE << PR_END_DATE << PR_CONVERSATION_TOPIC << PR_CREATION_TIME << PR_BODY << PR_SENSITIVITY << PR_HASATTACH << 0x80fa000b;
  messageProperties.fetch_all();

  KCal::Event* event = new KCal::Event;
  const FILETIME* date = (const FILETIME*) messageProperties[PR_START_DATE];
  if ( date ) {
    event->setDtStart( convertSysTime( *date ) );
    date = NULL;
  }

  date = (const FILETIME*) messageProperties[PR_END_DATE];
  if ( date ) {
    event->setDtEnd( convertSysTime( *date ) );
    date = NULL;
  }

  date = (const FILETIME*) messageProperties[PR_CREATION_TIME];
  if ( date ) {
    event->setCreated( convertSysTime( *date ) );
    date = NULL;
  }
	
  date = (const FILETIME*) messageProperties[PR_LAST_MODIFICATION_TIME];
  if ( date ) {
    event->setLastModified( convertSysTime( *date ) );
    date = NULL;
  }

  // Set Alarm
  const uint8_t* ui8 = (const uint8_t*) messageProperties[0x81ee000b];
  if ( ui8 && *ui8 ) {
    date = (const FILETIME*) messageProperties[0x81eb0040];
    if ( date ) {
      KCal::Alarm* alarm = new KCal::Alarm( dynamic_cast<KCal::Incidence*>( event ) );
      alarm->setTime( convertSysTime( *date ) );
      alarm->setEnabled( true );
      event->addAlarm( alarm );
      date = NULL;
    }
  }
  ui8 = NULL;

  const char* charString = (const char*) messageProperties[PR_CONVERSATION_TOPIC];
  if ( charString && *charString) {
    event->setSummary( charString );
    charString = NULL;
  }
	
  const uint32_t* ui32 = (const uint32_t*) messageProperties[PR_SENSITIVITY];
  if ( ui32 ) {
    event->setSecrecy( getIncidentSecrecy( *ui32 ) );
    ui32 = NULL;
  }
 
  ui32 = (const uint32_t*) messageProperties[PR_PRIORITY];
  if ( ui32 ) {
    switch ( *ui32 ) {
      case 0xffffffff: // Low Priority
        event->setPriority( 9 );
        break;
      case 0x00000000: // Normal Priority
        event->setPriority( 5 );
        break;
      case 0x00000001: // High Priority
        event->setPriority( 1 );
        break;
      default:
        event->setPriority( 0 ); // Unknown
    }
    ui32 = NULL;
  }
  
  ui32 = (const uint32_t*) messageProperties[0x81870003];
  if ( ui32 ) {
    event->setDuration( KCal::Duration( (*ui32) * 60 ) ); // OpenChange stores duration in minutes, KCal::Duration stores them in seconds
    ui32 = NULL;
  }

  ui8 = (const uint8_t*) messageProperties[0x80fa000b];
  if (ui8) {
    event->setAllDay( *ui8 );
    ui8 = NULL;
  }
	
  charString = (const char*) messageProperties[PR_BODY];
  if ( charString) {
    event->setDescription( charString );
    charString = NULL;
  } else {
    mapi_object_t objStream;
    mapi_object_init(&objStream);
    OpenStream(&mapi_message.data(), PR_BODY, 0, &objStream);

    uint32_t dataSize;
    GetStreamSize(&objStream, &dataSize);

    uint8_t* binData = new uint8_t[dataSize];

    uint32_t pos = 0;
    uint16_t bytesRead = 0;
    do {
      ReadStream(&objStream, binData+pos, 1024, &bytesRead);

      pos += bytesRead;

    } while (bytesRead && pos < dataSize);
		
    event->setDescription( (const char*) binData );

    mapi_object_release(&objStream);
  }

  // Get Attendees.
  SPropTagArray propertyTagArray;

  SRowSet rowSet;
  if ( GetRecipientTable( &mapi_message.data(), &rowSet, &propertyTagArray ) != MAPI_E_SUCCESS )
    qDebug() << "Error opening GetRecipientTable()";
	
  for (unsigned int i = 0; i < rowSet.cRows; ++i) {
    QString name, email;
    uint32_t *trackStatus = NULL, *flags = NULL, *type = NULL;
    for (unsigned int j = 0; j < rowSet.aRow[i].cValues; ++j) {
      switch ( rowSet.aRow[i].lpProps[j].ulPropTag ) {
        case 0x5ff6001f: // Should be PR_DISPLAY_NAME
          name = QString( rowSet.aRow[i].lpProps[j].value.lpszA );
          break;
        case 0x39fe001f: // Should be PR_SMTP_ADDRESS
	  email = QString( rowSet.aRow[i].lpProps[j].value.lpszA );
	  break;
        case PR_RECIPIENT_TRACKSTATUS:
          trackStatus = &rowSet.aRow[i].lpProps[j].value.l;
          break;
        case PR_RECIPIENTS_FLAGS:
          flags = &rowSet.aRow[i].lpProps[j].value.l;
          break;
        case PR_RECIPIENT_TYPE:
          type = &rowSet.aRow[i].lpProps[j].value.l;
          break;
        default:
          break;
      }
    }
			
    // Look at Microsoft's documentation for these values.

    if ( flags && ( *flags & 0x0000002 ) ) {
      event->setOrganizer( KCal::Person( name, email ) );
      continue;
    }

    KCal::Attendee* attendee = new KCal::Attendee(name, email);

    if ( type ) {
      if ( *type == 0x1 )
        attendee->setRole( KCal::Attendee::ReqParticipant );
      else if ( *type == 0x2 )
        attendee->setRole( KCal::Attendee::OptParticipant );
      else if ( *type == 0x03 )
        attendee->setRole( KCal::Attendee::NonParticipant );
    }
		
    if ( trackStatus ) {
      switch ( *trackStatus ) {
        case 0x00000000:
	  attendee->setStatus( KCal::Attendee::NeedsAction );
          break;
        case 0x00000001: // This object belongs to organizer (so assume he accepts)
          attendee->setStatus( KCal::Attendee::Accepted );
          break;
        case 0x00000002:
          attendee->setStatus( KCal::Attendee::Tentative );
          break;
        case 0x00000003:
          attendee->setStatus( KCal::Attendee::Accepted );
	  break;
        case 0x00000004:
          attendee->setStatus( KCal::Attendee::Declined );
          break;
        case 0x00000005:
          attendee->setStatus( KCal::Attendee::NeedsAction );
          break;
        default:
          break;
      }  
    }		
    event->addAttendee( attendee );
  }
			
  // Get attachments
  ui8 = (const uint8_t*) messageProperties[PR_HASATTACH];
  if ( ui8 && *ui8 ) {
    message::attachment_container_type attachments = mapi_message.fetch_attachments();

    for (message::attachment_container_type::iterator Iterator = attachments.begin(); Iterator != attachments.end(); ++Iterator) {
      QByteArray data( (const char*) (*Iterator)->get_data(), (*Iterator)->get_data_size() );
      QString attachFilename = (*Iterator)->get_filename().c_str();
      KMimeType::Ptr type = KMimeType::findByNameAndContent( attachFilename, data  );
      QByteArray final = KCodecs::base64Encode( data, true );
			
      KCal::Attachment *attachment = new KCal::Attachment( (const char*) final.data(), type->name() );
      attachment->setLabel( attachFilename );

      event->addAttachment( attachment );
    }
  }
  ui8 = NULL;

/* Disable this for now. I get different results using outlook and libmapi (with PR_ACCESS 0x2 read) (with PR_ACCESS_LEVEL 0x0 read-only)
  In outlook I get WRITE, READ & MODIFY and MAPI_MODIFY for PR_ACCESS_LEVEL
  // Leave this at the end so we can write all information to it even if it's read-only
  ui32 = (const uint32_t*) messageProperties[PR_ACCESS_LEVEL];
  if ( ui32 ) {
    qDebug() << "Access Level:" << QString::number( *ui32, 16 );
    if ( (*ui32) == 0x00000001 ) {
      qDebug() << "Event is not read-only";
      event->setReadOnly( false );
    } else {
      qDebug() << "Event is read-only";
      event->setReadOnly( true );
    }
    ui32 = NULL;
  }
*/

  IncidencePtr incidence( dynamic_cast<KCal::Incidence*>(event) );
  
  item.setPayload<IncidencePtr>( incidence );
}

void OCResource::appendTodoToItem( libmapipp::message & mapi_message, Akonadi::Item & item )
{
}

void OCResource::appendJournalToItem( libmapipp::message & mapi_message, Akonadi::Item & item )
{
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

  folder* folderPtr = NULL;
  folder::message_container_type* messages = NULL;
  QString contentType;
  try {
    folderPtr = new folder(m_session.get_message_store(), collection.remoteId().toULongLong());
    messages = new folder::message_container_type(folderPtr->fetch_messages());
    property_container folderProperties = folderPtr->get_property_container();
    folderProperties << PR_CONTAINER_CLASS;
    folderProperties.fetch();

    if ( folderProperties.size() ) {
      contentType = (const char*) *folderProperties.begin();
    } else {
      cancelTask( "Error: Did not get folder type for folder id " + QString::number( folderPtr->get_id() )  );
      return;
    }
  }
  catch(std::runtime_error e)
  {
    if (folderPtr)
      delete folderPtr;

    if (messages)
      delete messages;

    qDebug() << "MAPI EXception: " << e.what();
    cancelTask(e.what());
  }

  for (unsigned int i = 0; i < messages->size(); ++i) {

    Item item;
    // NOTE: This is a workaround. Have to ask akonadi developers if they are willing to add
    // a remoteParentID to Items like collections have. For now separate both ids with a  '-' character.
    item.setRemoteId( QString::number( (*messages)[i]->get_id()) + '-' + QString::number( (*messages)[i]->get_folder_id()) );

    if ( contentType == IPF_CONTACT ) {
      item.setMimeType( "text/vcard" );
      appendContactToItem( *(*messages)[i], item );
    }
    else if ( collection.contentMimeTypes().contains( "text/calendar" ) ) {
      item.setMimeType( "text/calendar" );

      if ( contentType == IPF_APPOINTMENT )
        appendEventToItem( *(*messages)[i], item );
      else if ( contentType == IPF_TASK )
        appendTodoToItem( *(*messages)[i], item  );
      else if ( contentType == IPF_JOURNAL )
        appendJournalToItem( *(*messages)[i], item );
    } else {
      item.setMimeType("message/rfc822");
      appendMessageToItem( *(*messages)[i], item );
    }

    emit percent( 50 );

    ItemCreateJob *append = new ItemCreateJob( item, collection );
    if ( !append->exec() ) {
      emit percent( 0 );
      emit status( Broken, QString( "Appending new message failed: %1" ).arg( append->errorString() ) );
    }
  }

  delete folderPtr;
  delete messages;

  emit percent( 100 );
  itemsRetrievalDone();
}

AKONADI_RESOURCE_MAIN( OCResource )

#include "ocresource.moc"
