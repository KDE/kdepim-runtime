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

#include <kmime/kmime_message.h>
#include <kabc/vcardconverter.h>

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/session.h>

#include "profiledialog.h"

typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;

OCResource::OCResource( const QString &id )
  :ResourceBase( id )
{
  // setName( "OC Resource" );

  m_mapiMemoryContext = talloc_init("openchangeclient");

  // Make this a setting somehow
  m_profileDatabase = QDir::homePath() + QString( "/.openchange/profiles.ldb" );

  int retval = MAPIInitialize( m_profileDatabase.toUtf8().data() );
  qDebug() << "initialize result: " << retval;
  if (retval != MAPI_E_SUCCESS) {
    if (GetLastError() != MAPI_E_NOT_FOUND ) {
      // It wasn't just a missing database....
      qDebug() << "unhandled initialisation problem";
      mapi_errstr("MAPIInitialize", GetLastError());
      exit (1);
    }

    // we need to create the database
    qDebug() << "trying to create database";
    QFileInfo profileFileInfo( m_profileDatabase );
    QDir parentDir = profileFileInfo.dir();
    if (! parentDir.exists() ) {
      // we have to create the parent
      qDebug() << "trying to create parent for database";
      bool success = parentDir.mkpath( profileFileInfo.path() );
      if (! success) {
	qDebug() << "Could not create parent path for profile file";
	exit(1);
      }
    }
    // we now have the parent directory
    retval = CreateProfileStore( m_profileDatabase.toUtf8().data(), "/usr/local/share/setup" );
    if (retval != MAPI_E_SUCCESS) {
      mapi_errstr( "CreateProfileStore", GetLastError());
      exit(1);
    }
    int retval = MAPIInitialize( m_profileDatabase.toUtf8().data() );
    if (retval != MAPI_E_SUCCESS) {
      // We've already tried to create the database, just bail here
      mapi_errstr("MAPIInitialize", GetLastError());
      exit (1);
    }
  }
}

OCResource::~ OCResource()
{
  mapi_object_release(&m_mapiStore);

  MAPIUninitialize();

  talloc_free(m_mapiMemoryContext);
}

bool OCResource::retrieveItem( const Akonadi::Item &item, const QStringList &parts )
{
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

void OCResource::itemChanged( const Akonadi::Item&, const QStringList& )
{
  qDebug() << "currently ignoring itemChanged()";
}

void OCResource::itemRemoved(const Akonadi::Item & item)
{
  qDebug() << "currently ignoring itemRemoved()";
}

void OCResource::getChildFolders( mapi_object_t *parentFolder, mapi_id_t id,
                                  const Akonadi::Collection &parentCollection,
                                  Akonadi::Collection::List &collections)
{
  mapi_object_t objFolder;
  mapi_object_init( &objFolder );
  enum MAPISTATUS retval = OpenFolder( parentFolder, id, &objFolder );
  if ( retval != MAPI_E_SUCCESS ) {
    qDebug() << "Failed to open folder in quest for child folders: " << retval;
    return;
  }

  mapi_object_t objHierarchyTable;
  mapi_object_init( &objHierarchyTable );
  retval = GetHierarchyTable( &objFolder, &objHierarchyTable );
  if ( retval != MAPI_E_SUCCESS ) {
    qDebug() << "Failed to get hierachy table in quest for child folders: " << retval;
    return;
  }

  struct SPropTagArray* sPropTagArray;
  sPropTagArray = set_SPropTagArray(m_mapiMemoryContext, 0x4,
                                    PR_DISPLAY_NAME,
                                    PR_FID,
                                    PR_COMMENT,
                                    PR_FOLDER_CHILD_COUNT);

  retval = SetColumns( &objHierarchyTable, sPropTagArray );
  MAPIFreeBuffer( sPropTagArray );
  if ( retval != MAPI_E_SUCCESS ) {
    qDebug() << "Failed to set columns in quest for child folders: " << retval;
    return;
  }

  struct SRowSet rowset;
  while ( ( QueryRows(&objHierarchyTable, 0x32, TBL_ADVANCE, &rowset ) != MAPI_E_NOT_FOUND) && rowset.cRows ) {
    // qDebug() << "num rows: " << rowset.cRows;
    for (unsigned int index = 0; index < rowset.cRows; index++) {
      Collection thisFolder;
      thisFolder.setParent( parentCollection );
      uint64_t *fid = (uint64_t *)find_SPropValue_data(&rowset.aRow[index], PR_FID);
      thisFolder.setRemoteId( QString::number( *fid ) );
      thisFolder.setName( ( const char * ) find_SPropValue_data(&rowset.aRow[index], PR_DISPLAY_NAME) );
      thisFolder.setType( Collection::Folder );
      QStringList folderMimeType;
      if (*( (uint32_t *)find_SPropValue_data(&rowset.aRow[index], PR_FOLDER_CHILD_COUNT) ) > 0 ) {
        // if it has children, needs this mimetype:
        folderMimeType << "inode/directory";
        uint64_t *fid = (uint64_t *)find_SPropValue_data(&rowset.aRow[index], PR_FID);
        getChildFolders(&objFolder, *fid, thisFolder, collections);
      } else {
        // no children - must have real contents
        mapi_object_t obj_folder;
        mapi_object_init(&obj_folder);
        retval = OpenFolder(parentFolder, *fid, &obj_folder);
        if ( retval != MAPI_E_SUCCESS ) {
          qDebug() << "Failed to open folder in quest for child folders container class: " << retval;
          return;
        }
        struct SPropTagArray* propertiesTagArray = set_SPropTagArray(m_mapiMemoryContext, 0x1, PR_CONTAINER_CLASS);
        uint32_t count;
        struct SPropValue *containerProperties;
        retval = GetProps(&obj_folder, propertiesTagArray, &containerProperties, &count);
        MAPIFreeBuffer(propertiesTagArray);
        if ( (retval != MAPI_E_SUCCESS) || (containerProperties[0].ulPropTag != PR_CONTAINER_CLASS) ) {
          qDebug() << "failed query for container class for " << thisFolder.name() << ", assuming rfc822 mailbox" << endl;
          folderMimeType << "message/rfc822";
        } else {
	  folderMimeType << mimeTypeForFolderType( containerProperties[0].value.lpszA );
	  qDebug() << "Folder " << thisFolder.name() << ", as mimetype " << folderMimeType << endl;
        }
      }
      thisFolder.setContentTypes( folderMimeType );
      collections.append( thisFolder );
    }
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

  // TODO: handle password (third argument)
  retval = MapiLogonEx(&m_session, profName, 0 );
  if (retval != MAPI_E_SUCCESS) {
    mapi_errstr("MapiLogonEx", GetLastError());
    exit (1);
  }

  mapi_object_init(&m_mapiStore);
  retval = OpenMsgStore(&m_mapiStore);
  if (retval != MAPI_E_SUCCESS) {
    mapi_errstr("OpenMsgStore", GetLastError());
    exit (1);
  }

  m_profileName = QString( profName );
}

void OCResource::retrieveCollections()
{
  qDebug() << "retrieving collections";

  login();

  struct SPropTagArray *sPropTagArray;
  struct SPropValue *lpProps;
  uint32_t cValues;

  Collection::List collections;

  /* Retrieve the mailbox folder name */
  sPropTagArray = set_SPropTagArray( m_mapiMemoryContext, 0x1, PR_DISPLAY_NAME );
  enum MAPISTATUS retval = GetProps(&m_mapiStore, sPropTagArray, &lpProps, &cValues);
  MAPIFreeBuffer(sPropTagArray);
  if (retval != MAPI_E_SUCCESS) {
    qDebug() << "failed to get properties:" << retval;
    exit( 1 );
  }

  if ( ! lpProps[0].value.lpszA ) {
    qDebug() << "null mailbox name";
    exit( 1 );
  } else {
    // qDebug() << "mailbox name: " << QString( lpProps[0].value.lpszA );
  }

  Collection account;
  account.setParent( Collection::root() );
  account.setRemoteId( m_profileName );
  account.setName( QString( lpProps[0].value.lpszA ) + QString( " (OpenChange)" ) );
  account.setType( Collection::Resource );
  QStringList mimeTypes;
  mimeTypes << "inode/directory";
  account.setContentTypes( mimeTypes );
  collections.append( account );

  mapi_id_t id_mailbox;
  /* Prepare the directory listing */
  retval = GetDefaultFolder(&m_mapiStore, &id_mailbox, olFolderTopInformationStore);
  if (retval != MAPI_E_SUCCESS) {
    qDebug() << "failed to get default folder:" << retval;
    exit( 1 );
  }

  getChildFolders( &m_mapiStore, id_mailbox, account, collections );

  collectionsRetrieved( collections );
}

static KDateTime convertSysTime(const struct mapi_SPropValue &lpProp)
{
  struct FILETIME filetime = lpProp.value.ft;
  NTTIME nt_time = filetime.dwHighDateTime;
  nt_time = nt_time << 32;
  nt_time |= filetime.dwLowDateTime;
  KDateTime kdeTime;
  kdeTime.setTime_t( nt_time_to_unix( nt_time ) );
  // qDebug() << "time: " << kdeTime.toString();
  return kdeTime;
}

enum MAPISTATUS OCResource::fetchFolder(const Akonadi::Collection & collection)
{
  enum MAPISTATUS retval;
  mapi_object_t objFolder;
  mapi_object_t obj_message;
  mapi_object_t obj_table;
  struct SPropTagArray *sPropTagArray;
  struct SRowSet rowset;
  struct mapi_SPropValue_array properties_array;

  mapi_object_init(&objFolder);
  mapi_object_init(&obj_table);

  if ( ( retval = OpenFolder(&m_mapiStore, collection.remoteId().toULongLong(), &objFolder) ) != MAPI_E_SUCCESS )
  {
    mapi_errstr( "OpenFolder",  GetLastError() );
    return retval;
  }

  if ( ( retval = GetContentsTable(&objFolder, &obj_table) ) != MAPI_E_SUCCESS )
  {
    mapi_errstr( "GetContentsTable", GetLastError() );
    return retval;
  }

  sPropTagArray = set_SPropTagArray(m_mapiMemoryContext, 0x5,
                                    PR_FID,
                                    PR_MID,
                                    PR_INST_ID,
                                    PR_INSTANCE_NUM,
                                    PR_SUBJECT);

  if ( ( retval = SetColumns(&obj_table, sPropTagArray ) ) != MAPI_E_SUCCESS )
  {
    mapi_errstr( "SetColumns",  GetLastError() );
    return retval;
  }
  MAPIFreeBuffer(sPropTagArray);

  while ( ( QueryRows(&obj_table, 0xa, TBL_ADVANCE, &rowset) == MAPI_E_SUCCESS ) && rowset.cRows ) {
    qDebug() << "Message count, this round:" << rowset.cRows;
    for (unsigned int i = 0; i < rowset.cRows; i++) {
      mapi_object_init(&obj_message);
      mapi_id_t *fid = (mapi_id_t *)find_SPropValue_data(&(rowset.aRow[i]), PR_FID);
      mapi_id_t *mid = (mapi_id_t *)find_SPropValue_data(&(rowset.aRow[i]), PR_MID);
      retval = OpenMessage(&m_mapiStore, *fid, *mid, &obj_message, 0);
      if (retval == MAPI_E_SUCCESS) {
        qDebug() << "building message";
        retval = GetPropsAll(&obj_message, &properties_array);
        if (retval != MAPI_E_SUCCESS) return retval;

	if ( collection.contentTypes().contains( "text/vcard" ) ) {
	  qDebug() << "Ooh, a contact!";
	  appendContactToCollection( properties_array, collection, &obj_message );
	} else {
	  appendMessageToCollection( properties_array, collection );
	}
      }
      mapi_object_release(&obj_message);
    }
  }

  mapi_object_release(&obj_table);
  mapi_object_release(&objFolder);

  return MAPI_E_SUCCESS;

}

void OCResource::appendMessageToCollection( struct mapi_SPropValue_array &properties_array, const Akonadi::Collection & collection)
{
  // mapidump_message(&properties_array);
  MessagePtr msg_ptr ( new KMime::Message );
  for ( uint32_t i = 0; i < properties_array.cValues; ++i ) {
    switch( properties_array.lpProps[i].ulPropTag ) {
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
      msg_ptr->date()->setDateTime( convertSysTime( properties_array.lpProps[i] ) );
      break;
    case PR_SENT_REPRESENTING_SEARCH_KEY:
      // PT_BINARY
      break;
    case PR_RECEIVED_BY_ENTRYID:
      // PT_BINARY
      break;
    case PR_RECEIVED_BY_NAME:
      // PT_STRING8
      qDebug() << "Received by:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_SENT_REPRESENTING_ENTRYID:
      // PT_BINARY
      break;
    case PR_SENT_REPRESENTING_NAME:
      // PT_STRING8
      msg_ptr->from()->from7BitString( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_RCVD_REPRESENTING_ENTRYID:
      // PT_BINARY
      break;
    case PR_RCVD_REPRESENTING_NAME:
      // PT_STRING8
      qDebug() << "Representing name:" << properties_array.lpProps[i].value.lpszA;
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
      qDebug() << "Sent representing email addr:" << properties_array.lpProps[i].value.lpszA;
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
      msg_ptr->from()->from7BitString( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_SENDER_SEARCH_KEY:
      // PT_BINARY
      break;
    case PR_SENDER_ADDRTYPE:
      // PT_STRING8
      break;
    case PR_SENDER_EMAIL_ADDRESS:
      // PT_STRING8
      qDebug() << "Sender email addr:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_DELETE_AFTER_SUBMIT:
      // PT_BOOLEAN
      break;
    case PR_DISPLAY_BCC:
      // PT_STRING8
      msg_ptr->bcc()->from7BitString( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_DISPLAY_CC:
      // PT_STRING8
      msg_ptr->cc()->from7BitString( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_DISPLAY_TO:
      // PT_STRING8
      // This (and BCC+CC) need to be split on semicolons, and combined with
      // the email address
      msg_ptr->to()->from7BitString( properties_array.lpProps[i].value.lpszA );
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
	msg_ptr->setBody( properties_array.lpProps[i].value.lpszA );
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
	struct SBinary_short *html = (struct SBinary_short *) find_mapi_SPropValue_data(&properties_array, PR_HTML );
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
	msg_ptr->date()->setDateTime( convertSysTime( properties_array.lpProps[i] ) );
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
      msg_ptr->subject()->from7BitString( properties_array.lpProps[i].value.lpszA );
      break;
    case 0x82f1001e:
      qDebug() << "82f1001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x82f2001e:
      qDebug() << "82f2001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0xe28001e:
      qDebug() << "0xe28001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0xe29001e:
      qDebug() << "0xe29001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8001001e:
      qDebug() << "0x8001001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    default:
      qDebug() << "Unhandled: " << QString::number( properties_array.lpProps[i].ulPropTag, 16 );
    }
  }

  uint8_t *has_attach = (uint8_t *) find_mapi_SPropValue_data(&properties_array, PR_HASATTACH);

  /* If we have attachments, retrieve them */
  if (has_attach && *has_attach) {
    // TODO
  }

  Item item;
  item.setRemoteId( "itemnumber" );
  item.setMimeType( "message/rfc822" );
  msg_ptr->from()->from7BitString( "from@someone.local" );
  item.setPayload<MessagePtr>( msg_ptr );
  ItemCreateJob *append = new ItemCreateJob( item, collection, session() );
  if ( !append->exec() ) {
    changeProgress( 0 );
    changeStatus( Error, QString( "Appending new message failed: %1" ).arg( append->errorString() ) );
  }
}

void OCResource::appendContactToCollection( struct mapi_SPropValue_array &properties_array, const Akonadi::Collection & collection, mapi_object_t *collection_object)
{
  enum MAPISTATUS retval;
  struct SPropTagArray *attachmentPropertiesTagArray;
  KABC::Addressee *contact = new KABC::Addressee;
  struct mapi_SPropValue_array attachPropertiesArray;

  Item item;
  item.setRemoteId( "itemnumber" );
  item.setMimeType( "text/vcard" );

  KABC::Address workAddress;
  workAddress.setType( KABC::Address::Work );
  KABC::Address homeAddress;
  homeAddress.setType( KABC::Address::Home );
  for ( uint32_t i = 0; i < properties_array.cValues; ++i ) {
    switch( properties_array.lpProps[i].ulPropTag ) {
    case PR_ALTERNATE_RECIPIENT_ALLOWED:
      // PT_BOOLEAN
      if ( properties_array.lpProps[i].value.b ) {
	contact->setSecrecy( KABC::Secrecy( KABC::Secrecy::Public) );
      } else {
	contact->setSecrecy( KABC::Secrecy( KABC::Secrecy::Private) );
      }
      break;
    case PR_IMPORTANCE:
      // PT_LONG
      qDebug() << "PR_IMPORTANCE:" << properties_array.lpProps[i].value.l << QString("(0x%1)").arg((uint)properties_array.lpProps[i].value.l, (int)4, (int)16, QChar('0') );
      break;
    case PR_MESSAGE_CLASS:
      // PT_STRING8
      qDebug() << "PR_MESSAGE_CLASS:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_ORIGINATOR_DELIVERY_REPORT_REQUESTED:
      // PT_BOOLEAN
      qDebug() << "PR_ORIGINATOR_DELIVERY_REPORT_REQUESTED:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case PR_READ_RECEIPT_REQUESTED:
      // PT_BOOLEAN
      qDebug() << "PR_READ_RECEIPT_REQUESTED:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case PR_SENT_REPRESENTING_NAME:
      // PT_STRING8
      qDebug() << "PR_SENT_REPRESENTING_NAME:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_SENT_REPRESENTING_ADDRTYPE:
      // PT_STRING8
      qDebug() << "PR_SENT_REPRESENTING_ADDRTYPE:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_SENT_REPRESENTING_EMAIL_ADDRESS:
      // PT_STRING8
      qDebug() << "PR_SENT_REPRESENTING_EMAIL_ADDRESS:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_CONVERSATION_TOPIC:
      // PT_STRING8
      qDebug() << "PR_CONVERSATION_TOPIC:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_SENDER_NAME:
      // PT_STRING8
      qDebug() << "PR_SENDER_NAME:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_SENDER_ADDRTYPE:
      // PT_STRING8
      qDebug() << "PR_SENDER_ADDRTYPE:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_SENDER_EMAIL_ADDRESS:
      // PT_STRING8
      qDebug() << "PR_SENDER_EMAIL_ADDRESS:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_DELETE_AFTER_SUBMIT:
      // PT_BOOLEAN
      qDebug() << "PR_DELETE_AFTER_SUBMIT:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case PR_DISPLAY_BCC:
      // PT_STRING8
      qDebug() << "PR_DISPLAY_BCC:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_DISPLAY_CC:
      // PT_STRING8
      qDebug() << "PR_DISPLAY_CC:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_DISPLAY_TO:
      // PT_STRING8
      qDebug() << "PR_DISPLAY_TO:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_HASATTACH:
      // PT_BOOLEAN
      qDebug() << "PR_HASATTACH:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      mapi_object_t obj_tb_attach;
      mapi_object_init( &obj_tb_attach );
      retval = GetAttachmentTable( collection_object, &obj_tb_attach );

      if (retval != MAPI_E_SUCCESS) {
	qDebug() << "GetAttachmentTable failed";
	break;
      }

      attachmentPropertiesTagArray = set_SPropTagArray( m_mapiMemoryContext, 0x1, PR_ATTACH_NUM );
      retval = SetColumns( &obj_tb_attach, attachmentPropertiesTagArray );
      if (retval != MAPI_E_SUCCESS) {
	qDebug() << "SetColumns failed";
	break;
      }
      MAPIFreeBuffer( attachmentPropertiesTagArray);

      struct SRowSet rowset_attach;
      retval = QueryRows( &obj_tb_attach, 0xa, TBL_ADVANCE, &rowset_attach );
      if (retval != MAPI_E_SUCCESS) {
	qDebug() << "QueryRows failed";
	break;
      }

      qDebug() << "Num attachments:" << rowset_attach.cRows;

      for ( uint32_t j = 0; j < rowset_attach.cRows; ++j ) {
	const uint32_t *attach_num = (const uint32_t *)find_SPropValue_data( &(rowset_attach.aRow[j]), PR_ATTACH_NUM );
	mapi_object_t obj_attach;
	mapi_object_init(&obj_attach);
	retval = OpenAttach( collection_object, *attach_num, &obj_attach );
	if (retval != MAPI_E_SUCCESS) {
	  qDebug() << "OpenAttach failed";
	  break;
	}

	retval = GetPropsAll(&obj_attach, &attachPropertiesArray);

	QString attachFilename = QString( (const char*)find_mapi_SPropValue_data( &attachPropertiesArray, PR_ATTACH_FILENAME ) );

	if ( attachFilename.isEmpty() ) {
	  attachFilename = QString( (const char*)find_mapi_SPropValue_data( &attachPropertiesArray, PR_ATTACH_LONG_FILENAME ) );
	}

	qDebug() << "attachment Filename:" << attachFilename;

	const uint32_t *attachSize = (const uint32_t *)find_mapi_SPropValue_data( &attachPropertiesArray, PR_ATTACH_SIZE );
	qDebug() << "file size: " << *attachSize;

	if ( attachFilename == "ContactPicture.jpg" ) {
	  struct SBinary *attachData = (struct SBinary*)find_mapi_SPropValue_data( &attachPropertiesArray, PR_ATTACH_DATA_BIN );
	  QImage photoData = QImage::fromData( (const uchar*)attachData->lpb, attachData->cb );
	  if ( !photoData.isNull() ) {
	    qDebug() << "Photo: " << photoData.size();
	    contact->setPhoto( KABC::Picture( photoData) );
	  }
	}
	mapi_object_release(&obj_attach);

      }
      break;
    case PR_RTF_IN_SYNC:
      // PT_BOOLEAN
      qDebug() << "PR_RTF_IN_SYNC:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case PR_URL_COMP_NAME_SET:
      // PT_BOOLEAN
      qDebug() << "PR_URL_COMP_NAME_SET:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case PR_RTF_COMPRESSED:
      // PT_BINARY
      qDebug() << "PR_RTF_COMPRESSED size:" << properties_array.lpProps[i].value.bin.cb;
      qDebug() << "PR_RTF_COMPRESSED data:" << QByteArray( (char*)properties_array.lpProps[i].value.bin.lpb, properties_array.lpProps[i].value.bin.cb ).toHex();
      break;
    case PR_URL_COMP_NAME:
      // PT_STRING8
      qDebug() << "PR_URL_COMP_NAME:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_ATTR_HIDDEN:
      // PT_BOOLEAN
      qDebug() << "PR_ATTR_HIDDEN:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case PR_ATTR_SYSTEM:
      // PT_BOOLEAN
      qDebug() << "PR_ATTR_SYSTEM:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case PR_ATTR_READONLY:
      // PT_BOOLEAN
      qDebug() << "PR_ATTR_READONLY:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case PR_DISPLAY_NAME:
      // PT_STRING8
      contact->setNameFromString( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_ACCOUNT:
      // PT_STRING8
      qDebug() << "PR_ACCOUNT:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_OFFICE_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( properties_array.lpProps[i].value.lpszA, KABC::PhoneNumber::Work ) );
      break;
    case PR_CALLBACK_TELEPHONE_NUMBER:
      // PT_STRING8
      qDebug() << "PR_CALLBACK_TELEPHONE_NUMBER:" << properties_array.lpProps[i].value.lpszA;
      // contact->insertPhoneNumber( KABC::PhoneNumber( properties_array.lpProps[i].value.lpszA, KABC::PhoneNumber::Work ) );
      break;
    case PR_HOME_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( properties_array.lpProps[i].value.lpszA, KABC::PhoneNumber::Home ) );
      break;
    case PR_INITIALS:
      // PT_STRING8
      qDebug() << "PR_INITIALS:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_KEYWORD:
      // PT_STRING8
      qDebug() << "PR_KEYWORD:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_LANGUAGE:
      // PT_STRING8
      qDebug() << "PR_LANGUAGE:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_LOCATION:
      // PT_STRING8
      qDebug() << "PR_LOCATION:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_SURNAME:
      // PT_STRING8
      contact->setFamilyName( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_GENERATION:
      // PT_STRING8
      // This is a bit of a stretch.
      contact->setSuffix( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_GIVEN_NAME:
      // PT_STRING8
      contact->setGivenName( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_POSTAL_ADDRESS:
      // PT_STRING8
      qDebug() << "PR_POSTAL_ADDRESS:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_COMPANY_NAME:
      // PT_STRING8
      contact->setOrganization( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_TITLE:
      // PT_STRING8
      contact->setRole( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_OFFICE_LOCATION:
      // PT_STRING8
      contact->insertCustom( "KADDRESSBOOK", "Office", properties_array.lpProps[i].value.lpszA );
      break;
    case PR_PRIMARY_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( properties_array.lpProps[i].value.lpszA, KABC::PhoneNumber::Pref ) );
      break;
    case PR_OFFICE2_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( properties_array.lpProps[i].value.lpszA, KABC::PhoneNumber::Work ) );
      break;
    case PR_RADIO_TELEPHONE_NUMBER:
      // ignored
      break;
    case PR_PAGER_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( properties_array.lpProps[i].value.lpszA, KABC::PhoneNumber::Pager ) );
      break;
    case PR_TELEX_NUMBER:
      // ignored
      break;
    case PR_HOME2_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( properties_array.lpProps[i].value.lpszA, KABC::PhoneNumber::Home ) );
      break;
    case PR_DEPARTMENT_NAME:
      // PT_STRING8
      // FIXME: this probably shouldn't be required. kdepimlibs needs to be cleaned up for KDE5.
      contact->setDepartment( properties_array.lpProps[i].value.lpszA );
      contact->insertCustom( "KADDRESSBOOK", "Department", properties_array.lpProps[i].value.lpszA );
      break;
    case PR_MOBILE_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( properties_array.lpProps[i].value.lpszA, KABC::PhoneNumber::Cell ) );
      break;
    case PR_BUSINESS_FAX_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( properties_array.lpProps[i].value.lpszA, KABC::PhoneNumber::Work | KABC::PhoneNumber::Fax ) );
      break;
    case PR_COUNTRY:
      // PT_STRING8
      workAddress.setCountry( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_LOCALITY:
      // PT_STRING8
      workAddress.setLocality( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_STREET_ADDRESS :
      // PT_STRING8
      workAddress.setStreet( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_STATE_OR_PROVINCE:
      // PT_STRING8
      workAddress.setRegion( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_POSTAL_CODE:
      // PT_STRING8
      workAddress.setPostalCode( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_POST_OFFICE_BOX:
      // PT_STRING8
      workAddress.setPostOfficeBox( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_ISDN_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( properties_array.lpProps[i].value.lpszA, KABC::PhoneNumber::Isdn ) );
      break;
    case PR_CAR_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( properties_array.lpProps[i].value.lpszA, KABC::PhoneNumber::Car ) );
      break;
    case PR_OTHER_TELEPHONE_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( properties_array.lpProps[i].value.lpszA, KABC::PhoneNumber::Voice ) );
      break;
    case PR_HOME_FAX_NUMBER:
      // PT_STRING8
      contact->insertPhoneNumber( KABC::PhoneNumber( properties_array.lpProps[i].value.lpszA, KABC::PhoneNumber::Fax | KABC::PhoneNumber::Home ) );
      break;
    case PR_PRIMARY_FAX_NUMBER:
      // PT_STRING8
      qDebug() << "PR_PRIMARY_FAX_NUMBER:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_ASSISTANT:
      // PT_STRING8
      contact->insertCustom( "KADDRESSBOOK", "AssistantName", properties_array.lpProps[i].value.lpszA );
      break;
    case PR_WEDDING_ANNIVERSARY:
      // PT_SYSTIME
      contact->insertCustom( "KADDRESSBOOK", "Anniversary", convertSysTime( properties_array.lpProps[i] ).toString("%Y-%m-%d") );
      break;
    case PR_BIRTHDAY:
      // PT_SYSTIME
      contact->setBirthday( convertSysTime( properties_array.lpProps[i] ).toUtc().dateTime() );
      break;
    case PR_MIDDLE_NAME:
      // PT_STRING8
      contact->setAdditionalName( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_DISPLAY_NAME_PREFIX:
      // PT_STRING8
      contact->setPrefix( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_PROFESSION:
      // PT_STRING8
      contact->insertCustom( "KADDRESSBOOK", "Profession", properties_array.lpProps[i].value.lpszA );
      break;
    case PR_SPOUSE_NAME:
      // PT_STRING8
      contact->insertCustom( "KADDRESSBOOK", "SpouseName", properties_array.lpProps[i].value.lpszA );
      break;
    case PR_TTYTDD_PHONE_NUMBER:
      // ignored
      break;
    case PR_MANAGER_NAME:
      // PT_STRING8
      contact->insertCustom( "KADDRESSBOOK", "Manager", properties_array.lpProps[i].value.lpszA );
      break;
    case PR_NICKNAME:
      // PT_STRING8
      contact->setNickName( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_BUSINESS_HOME_PAGE:
      // PT_STRING8
      contact->setUrl( KUrl( properties_array.lpProps[i].value.lpszA ) );
      break;
    case PR_COMPANY_MAIN_PHONE_NUMBER:
      // PT_STRING8
      qDebug() << "PR_COMPANY_MAIN_PHONE_NUMBER:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_HOME_ADDRESS_CITY:
      // PT_STRING8
      homeAddress.setLocality( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_HOME_ADDRESS_COUNTRY:
      // PT_STRING8
      homeAddress.setCountry( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_HOME_ADDRESS_POSTAL_CODE:
      // PT_STRING8
      homeAddress.setPostalCode( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_HOME_ADDRESS_STATE_OR_PROVINCE:
      // PT_STRING8
      homeAddress.setRegion( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_HOME_ADDRESS_STREET:
      // PT_STRING8
      homeAddress.setStreet( properties_array.lpProps[i].value.lpszA );
      break;
    case PR_CREATOR_NAME:
      // PT_STRING8
      qDebug() << "PR_CREATOR_NAME:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_LAST_MODIFIER_NAME:
      // PT_STRING8
      qDebug() << "PR_LAST_MODIFIER_NAME:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_CREATOR_ENTRYID:
      // PT_BINARY
      qDebug() << "PR_CREATOR_ENTRYID size:" << properties_array.lpProps[i].value.bin.cb;
      qDebug() << "PR_CREATOR_ENTRYID data:" << QByteArray( (char*)properties_array.lpProps[i].value.bin.lpb, properties_array.lpProps[i].value.bin.cb ).toHex();
      break;
    case PR_HAS_NAMED_PROPERTIES:
      // PT_BOOLEAN
      qDebug() << "PR_HAS_NAMED_PROPERTIES:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case PR_URL_NAME:
      // PT_STRING8
      qDebug() << "PR_URL_NAME:" << properties_array.lpProps[i].value.lpszA;
      break;
    case PR_INTERNET_MESSAGE_ID:
      // PT_STRING8
      qDebug() << "PR_INTERNET_MESSAGE_ID:" << properties_array.lpProps[i].value.lpszA;
      break;

    case 0x8001001e:
      // PT_STRING8
      qDebug() << "0x8001001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8004000b:
      // PT_BOOLEAN
      qDebug() << "0x8004000b:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case 0x8008101e:
      for (unsigned int j=0; j < properties_array.lpProps[i].value.MVszA.cValues; ++j) {
	qDebug() << "0x8008101e:" << properties_array.lpProps[i].value.MVszA.strings[j].lppszA;
      }
      break;
    case 0x800d001e:
      // PT_STRING8
      qDebug() << "0x800d001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x800e001e:
      // PT_STRING8
      qDebug() << "0x800e001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x800f001e:
      // PT_STRING8
      qDebug() << "0x800f001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8010001e:
      // PT_STRING8
      contact->insertEmail( properties_array.lpProps[i].value.lpszA, true ); // preferred
      break;
    case 0x8011001e:
      // PT_STRING8
      contact->insertEmail( properties_array.lpProps[i].value.lpszA );
      break;
    case 0x8012001e:
      // PT_STRING8
      contact->insertEmail( properties_array.lpProps[i].value.lpszA );
      break;
    case 0x8013001e:
      // PT_STRING8
      qDebug() << "Display as (email):" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8014001e:
      // PT_STRING8
      qDebug() << "Display as (email2):" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8015001e:
      // PT_STRING8
      qDebug() << "Display as (email3):" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8016001e:
      // PT_STRING8
      qDebug() << "0x8016001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8019101e:
      for (unsigned int j=0; j < properties_array.lpProps[i].value.MVszA.cValues; ++j) {
	qDebug() << "0x8019101e:" << properties_array.lpProps[i].value.MVszA.strings[j].lppszA;
      }
      break;
    case 0x801f001e:
      // PT_STRING8
      qDebug() << "0x801f001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8020001e:
      // PT_STRING8
      qDebug() << "0x8020001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8021001e:
      // PT_STRING8
      qDebug() << "0x8021001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8022001e:
      // PT_STRING8
      contact->insertCustom( "KADDRESSBOOK", "IMAddress", properties_array.lpProps[i].value.lpszA );
      break;
    case 0x8023001e:
      // PT_STRING8
      qDebug() << "Web page address:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x80b7001e:
      // PT_STRING8
      qDebug() << "0x80b7001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x80bf001e:
      // PT_STRING8
      qDebug() << "0x80b7001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x80c5001e:
      // PT_STRING8
      qDebug() << "0x80c5001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x80c9001e:
      // PT_STRING8
      qDebug() << "0x80c9001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x80cc001e:
      // PT_STRING8
      qDebug() << "0x80cc001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x80cd001e:
      // PT_STRING8
      qDebug() << "0x80cd001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x81a7000b:
      // PT_BOOLEAN
      qDebug() << "0x81a7000b:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case 0x81cd001e:
      // PT_STRING8
      qDebug() << "0x81cd001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x81ce0102:
      // PT_BINARY
      qDebug() << "0x81ce0102 size:" << properties_array.lpProps[i].value.bin.cb;
      qDebug() << "0x81ce0102 data:" << QByteArray( (char*)properties_array.lpProps[i].value.bin.lpb, properties_array.lpProps[i].value.bin.cb ).toHex();
      break;
    case 0x81cf001e:
      // PT_STRING8
      qDebug() << "0x81cf001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x81d00102:
      // PT_BINARY
      qDebug() << "0x81d00102 size:" << properties_array.lpProps[i].value.bin.cb;
      qDebug() << "0x81d00102 data:" << QByteArray( (char*)properties_array.lpProps[i].value.bin.lpb, properties_array.lpProps[i].value.bin.cb ).toHex();
      break;
    case 0x81d1001e:
      // PT_STRING8
      qDebug() << "0x81d1001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x81d20102:
      // PT_BINARY
      qDebug() << "0x81d20102 size:" << properties_array.lpProps[i].value.bin.cb;
      qDebug() << "0x81d20102 data:" << QByteArray( (char*)properties_array.lpProps[i].value.bin.lpb, properties_array.lpProps[i].value.bin.cb ).toHex();
      break;
    case 0x827c001e:
      // PT_STRING8
      qDebug() << "0x827c001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x827f000b:
      // PT_BOOLEAN
      qDebug() << "0x827f000b:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case 0x8281001e:
      // PT_STRING8
      qDebug() << "0x8281001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8282001e:
      // PT_STRING8
      qDebug() << "0x8282001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8284001e:
      // PT_STRING8
      qDebug() << "0x8284001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x82b9000b:
      // PT_BOOLEAN
      qDebug() << "0x82b9000b:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case 0x82ef000b:
      // PT_BOOLEAN
      qDebug() << "0x82ef000b:" << ( properties_array.lpProps[i].value.b ? "true":"false" );
      break;
    case 0x82f5001e:
      // PT_STRING8
      qDebug() << "0x82f5001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x82f60102:
      // PT_BINARY
      qDebug() << "0x82f60102 size:" << properties_array.lpProps[i].value.bin.cb;
      qDebug() << "0x82f60102 data:" << QByteArray( (char*)properties_array.lpProps[i].value.bin.lpb, properties_array.lpProps[i].value.bin.cb ).toHex();
      break;
    case 0x82f70102:
      // PT_BINARY
      qDebug() << "0x82f70102 size:" << properties_array.lpProps[i].value.bin.cb;
      qDebug() << "0x82f70102 data:" << QByteArray( (char*)properties_array.lpProps[i].value.bin.lpb, properties_array.lpProps[i].value.bin.cb ).toHex();
      break;
    case 0x8386001e:
      // PT_STRING8
      qDebug() << "0x8386001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8387001e:
      // PT_STRING8
      qDebug() << "0x8387001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8388001e:
      // PT_STRING8
      qDebug() << "0x8388001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x83890102:
      // PT_BINARY
      qDebug() << "0x83890102 size:" << properties_array.lpProps[i].value.bin.cb;
      qDebug() << "0x83890102 data:" << QByteArray( (char*)properties_array.lpProps[i].value.bin.lpb, properties_array.lpProps[i].value.bin.cb ).toHex();
      break;
    case 0x838e001e:
      // PT_STRING8
      qDebug() << "0x838e001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x838f001e:
      // PT_STRING8
      qDebug() << "0x838f001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8390001e:
      // PT_STRING8
      qDebug() << "0x8390001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x83910102:
      // PT_BINARY
      qDebug() << "0x83910102 size:" << properties_array.lpProps[i].value.bin.cb;
      qDebug() << "0x83910102 data:" << QByteArray( (char*)properties_array.lpProps[i].value.bin.lpb, properties_array.lpProps[i].value.bin.cb ).toHex();
      break;
    case 0x8396001e:
      // PT_STRING8
      qDebug() << "0x8396001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8397001e:
      // PT_STRING8
      qDebug() << "0x8397001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x8398001e:
      // PT_STRING8
      qDebug() << "0x8398001e:" << properties_array.lpProps[i].value.lpszA;
      break;
    case 0x83990102:
      // PT_BINARY
      qDebug() << "0x83990102 size:" << properties_array.lpProps[i].value.bin.cb;
      qDebug() << "0x83990102 data:" << QByteArray( (char*)properties_array.lpProps[i].value.bin.lpb, properties_array.lpProps[i].value.bin.cb ).toHex();
      break;

    default:
      qDebug() << "Unhandled in collection: " << QString::number( properties_array.lpProps[i].ulPropTag, 16 );
    }
  }
  contact->insertAddress( workAddress );
  contact->insertAddress( homeAddress );
  item.setPayload<KABC::Addressee>( *contact );
  ItemCreateJob *append = new ItemCreateJob( item, collection, session() );
  if ( !append->exec() ) {
    changeProgress( 0 );
    changeStatus( Error, QString( "Appending new message failed: %1" ).arg( append->errorString() ) );
  }
}

void OCResource::retrieveItems(const Akonadi::Collection & collection, const QStringList &parts)
{
  qDebug() << "currently synchronizeCollections() is incomplete";

  ItemFetchJob *fetch = new ItemFetchJob( collection, session() );
  if ( !fetch->exec() ) {
    changeStatus( Error,
                  QString( "Unable to fetch listing of collection '%1': %2" ).arg( collection.name() ).arg( fetch->errorString() ) );
    return;
  }

  changeProgress( 0 );

  Item::List items = fetch->items();

  fetchFolder( collection );

  changeProgress( 100 );
  itemsRetrieved();
}

AKONADI_RESOURCE_MAIN( OCResource )

#include "ocresource.moc"
