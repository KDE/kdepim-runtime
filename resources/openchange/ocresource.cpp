/*
    Copyright (c) 2007 Brad Hards <bradh@frogmouth.net>

    Significant amounts of this code adapted from the openchange client utility,
    which is Copyright (C) Julien Kerihuel 2007.

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
#include <QPushButton>
#include <QVBoxLayout>

#include <boost/shared_ptr.hpp>

#include <kmime/kmime_message.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemappendjob.h>

typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;

ProfileDialog::ProfileDialog( OCResource *resource, QWidget *parent )
  : QDialog( parent ), m_resource( resource )
{
  setWindowTitle( "Profile Configuration" );

  QVBoxLayout *mainLayout = new QVBoxLayout;

  QLabel *label = new QLabel( "OpenChange Profiles" );
  mainLayout->addWidget( label );


  QHBoxLayout *lowerLayout = new QHBoxLayout;

  QVBoxLayout *leftLayout = new QVBoxLayout;
  m_listOfProfiles = new QListWidget;
  leftLayout->addWidget( m_listOfProfiles );

  leftLayout->addStretch();


  QVBoxLayout *rightLayout = new QVBoxLayout;

  QPushButton *addButton = new QPushButton( "Add Profile" );
  connect( addButton, SIGNAL( clicked() ),
           this, SLOT( addNewProfile() ) );
  rightLayout->addWidget( addButton );

  QPushButton *editButton = new QPushButton( "Edit Profile" );
  connect( editButton, SIGNAL( clicked() ),
           this, SLOT( editExistingProfile() ) );
  rightLayout->addWidget( editButton );

  QPushButton *setAsDefaultButton = new QPushButton( "Make default" );
  connect( setAsDefaultButton, SIGNAL( clicked() ),
           this, SLOT( setAsDefaultProfile() ) );
  rightLayout->addWidget( setAsDefaultButton );

  QPushButton *removeButton = new QPushButton( "Remove Profile" );
  connect( removeButton, SIGNAL( clicked() ),
           this, SLOT( deleteSelectedProfile() ) );
  rightLayout->addWidget( removeButton );

  QPushButton *closeButton = new QPushButton( "Close" );
  connect( closeButton, SIGNAL( clicked() ),
           this, SLOT( close() ) );
  rightLayout->addWidget( closeButton );

  rightLayout->addStretch();


  lowerLayout->addLayout( leftLayout );
  lowerLayout->addLayout( rightLayout );
  mainLayout->addLayout( lowerLayout );
  setLayout( mainLayout );

  fillProfileList();
}

void ProfileDialog::fillProfileList()
{
  enum MAPISTATUS retval;
  struct SRowSet  proftable;
  uint32_t        count = 0;

  memset(&proftable, 0, sizeof (struct SRowSet));
  if ((retval = GetProfileTable(&proftable, 0)) != MAPI_E_SUCCESS) {
    mapi_errstr("GetProfileTable", GetLastError());
    exit (1);
  }

  // qDebug() << "Profiles in the database:" << proftable.cRows;

  for (count = 0; count != proftable.cRows; count++) {
    const char      *name = NULL;
    uint32_t        dflt = 0;

    name = proftable.aRow[count].lpProps[0].value.lpszA;
    dflt = proftable.aRow[count].lpProps[1].value.l;

    if (dflt) {
      QString profileName( QString( name ) + QString( " [default]" ) );
      QListWidgetItem *item = new QListWidgetItem( profileName, m_listOfProfiles );
      m_listOfProfiles->setCurrentItem( item );
    } else {
      new QListWidgetItem( QString( name ), m_listOfProfiles );
    }

  }
}

void ProfileDialog::deleteSelectedProfile()
{
  qDebug() << "selected profile: " << m_listOfProfiles->currentItem()->text();

  QListWidgetItem *selectedEntry = m_listOfProfiles->currentItem();
  QString selectedProfileName( selectedEntry->text() );
  // the profile name might have " [default]" on the end - check for that
  if ( selectedProfileName.endsWith ( " [default]" ) )
  {
    int endOfName = selectedProfileName.lastIndexOf( " [default]" );
    QString realProfileName = selectedProfileName.mid( 0, endOfName );
    enum MAPISTATUS retval;
    if ((retval = DeleteProfile(realProfileName.toLatin1().data(), 0)) != MAPI_E_SUCCESS) {
      mapi_errstr("DeleteProfile", GetLastError());
      exit (1);
    }
    qDebug() << realProfileName << "deleted (default)";
  } else {
    enum MAPISTATUS retval;
    if ((retval = DeleteProfile(selectedProfileName.toLatin1().data(), 0)) != MAPI_E_SUCCESS) {
      mapi_errstr("DeleteProfile", GetLastError());
      exit (1);
    }
    qDebug() << selectedProfileName << "deleted";
  }
  m_listOfProfiles->clear();
  fillProfileList();
}

void ProfileDialog::addNewProfile()
{
  qDebug() << "Adding profiles not yet implemented";
}


void ProfileDialog::editExistingProfile()
{
  qDebug() << "Profile editing not yet implemented";
}

void ProfileDialog::setAsDefaultProfile()
{
  qDebug() << "selected profile: " << m_listOfProfiles->currentItem()->text();

  QListWidgetItem *selectedEntry = m_listOfProfiles->currentItem();
  QString selectedProfileName( selectedEntry->text() );

  if ( selectedProfileName.endsWith ( " [default]" ) )
    return; // since this is already the default profile

  if ( SetDefaultProfile(selectedProfileName.toLatin1().data(), 0) != MAPI_E_SUCCESS) {
    mapi_errstr("SetDefaultProfile", GetLastError());
  }
  m_listOfProfiles->clear();
  fillProfileList();
}


OCResource::OCResource( const QString &id )
  :ResourceBase( id )
{
  setName( "OC Resource" );

  m_mapiMemoryContext = talloc_init("openchangeclient");

  // Make this a setting somehow
  m_profileDatabase = QDir::homePath() + QString( "/.openchange/profiles.ldb" );

  int retval = MAPIInitialize( m_profileDatabase.toAscii().data() );
  if (retval != MAPI_E_SUCCESS) {
    mapi_errstr("MAPIInitialize", GetLastError());
    exit (1);
  }

  const char *profileName;
  retval = GetDefaultProfile(&profileName, 0);
  if (retval != MAPI_E_SUCCESS) {
    mapi_errstr("GetDefaultProfile", GetLastError());
    exit (1);
  }

  retval = MapiLogonEx(&m_session, profileName );
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
}

OCResource::~ OCResource()
{
  mapi_object_release(&m_mapiStore);

  MAPIUninitialize();

  talloc_free(m_mapiMemoryContext);
}

bool OCResource::requestItemDelivery( const Akonadi::DataReference &ref, const QStringList &parts, const QDBusMessage &msg )
{
  qDebug() << "currently ignoring requestItemDelivery";
  return true;
}


void OCResource::aboutToQuit()
{
  qDebug() << "currently ignoring aboutToQuit()";
}

void OCResource::configure()
{
  ProfileDialog configDialog( this );
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

void OCResource::itemRemoved(const Akonadi::DataReference & ref)
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
  // TODO: PR_CONTAINER_CLASS
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
      // qDebug() << "Folder id: " << *(uint64_t *)find_SPropValue_data(&rowset.aRow[index], PR_FID);
      thisFolder.setRemoteId( QString::number( *(uint64_t *)find_SPropValue_data(&rowset.aRow[index], PR_FID ) ) );
      thisFolder.setName( ( const char * ) find_SPropValue_data(&rowset.aRow[index], PR_DISPLAY_NAME) );
      thisFolder.setCachePolicyId( 1 );
      thisFolder.setType( Collection::Folder );
      QStringList folderMimeType;
      if (*( (uint32_t *)find_SPropValue_data(&rowset.aRow[index], PR_FOLDER_CHILD_COUNT) ) > 0 ) {
        // if it has children, needs this mimetype:
        folderMimeType << "inode/directory";
        uint64_t *fid = (uint64_t *)find_SPropValue_data(&rowset.aRow[index], PR_FID);
        getChildFolders(&objFolder, *fid, thisFolder, collections);
      } else {
        // no children - must have real contents
        // // TODO: add mime conversion from PR_CONTAINER_CLASS in here.
        folderMimeType << "message/rfc822";
      }
      thisFolder.setContentTypes( folderMimeType );
      collections.append( thisFolder );
    }
  }
}

void OCResource::retrieveCollections()
{
  qDebug() << "retrieving collections";

  struct SPropTagArray *sPropTagArray;
  struct SPropValue *lpProps;
  uint32_t cValues;

  // for now we only work on the default profile
  enum MAPISTATUS retval;
  const char      *profname;
  if ((retval = GetDefaultProfile(&profname, 0)) != MAPI_E_SUCCESS) {
    mapi_errstr("GetDefaultProfile", GetLastError());
    exit( 1 );
  }

  QString profileName( profname );

  Collection::List collections;

  /* Retrieve the mailbox folder name */
  sPropTagArray = set_SPropTagArray(m_mapiMemoryContext, 0x1,
                                    PR_DISPLAY_NAME);
  retval = GetProps(&m_mapiStore, sPropTagArray, &lpProps, &cValues);
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
  account.setRemoteId( profileName );
  account.setName( QString( lpProps[0].value.lpszA ) + QString( " (OpenChange)" ) );
  account.setType( Collection::Resource );
  QStringList mimeTypes;
  mimeTypes << "inode/directory";
  account.setContentTypes( mimeTypes );
  account.setCachePolicyId( 1 ); // ### just for testing
  collections.append( account );

  mapi_id_t id_mailbox;
  /* Prepare the directory listing */
  retval = GetDefaultFolder(&m_mapiStore, &id_mailbox, olFolderTopInformationStore);
  if (retval != MAPI_E_SUCCESS) {
    qDebug() << "failed to get default folder:" << retval;
    exit( 1 );
  }

  getChildFolders( &m_mapiStore, id_mailbox, account, collections );

  // qDebug() << "about to announce collections";
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
  uint32_t messagesCount;
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

  if ( ( retval = GetRowCount(&obj_table, &messagesCount) ) != MAPI_E_SUCCESS )
  {
    mapi_errstr( "GetRowCount", GetLastError() );
    return retval;
  }

  // qDebug() << "Message count:" << messagesCount;

  while ( ( (retval = QueryRows(&obj_table, 0xa, TBL_ADVANCE, &rowset)) != MAPI_E_NOT_FOUND ) && rowset.cRows) {
    for (unsigned int i = 0; i < rowset.cRows; i++) {
      mapi_object_init(&obj_message);
      retval = OpenMessage(&m_mapiStore,
                           rowset.aRow[i].lpProps[0].value.d,
                           rowset.aRow[i].lpProps[1].value.d,
                           &obj_message);
      if (retval == MAPI_E_SUCCESS) {
        // qDebug() << "building message";
        retval = GetPropsAll(&obj_message, &properties_array);
        if (retval != MAPI_E_SUCCESS) return retval;
        uint8_t *has_attach = (uint8_t *) find_mapi_SPropValue_data(&properties_array, PR_HASATTACH);

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
            break;
          case PR_SENT_REPRESENTING_ENTRYID:
            // PT_BINARY
            break;
          case PR_SENT_REPRESENTING_NAME:
            // PT_STRING8
            if ( msg_ptr->from()->isEmpty() ) {
              // we'll use PR_SENT_NAME for preference
              msg_ptr->from()->from7BitString( properties_array.lpProps[i].value.lpszA );
            }
            break;
          case PR_RCVD_REPRESENTING_ENTRYID:
            // PT_BINARY
            break;
          case PR_RCVD_REPRESENTING_NAME:
            // PT_STRING8
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
          case PR_ACTION:
            // PT_LONG
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
          default:
            qDebug() << "Unhandled: " << QString::number( properties_array.lpProps[i].ulPropTag, 16 );
          }
        }



        Item item( DataReference( -1, "itemnumber" ) );
        item.setMimeType( "message/rfc822" );
        item.setPayload<MessagePtr>( msg_ptr );
        ItemAppendJob *append = new ItemAppendJob( item, collection, session() );
        if ( !append->exec() ) {
          changeProgress( 0 );
          changeStatus( Error, QString( "Appending new message failed: %1" ).arg( append->errorString() ) );
          return MAPI_E_CALL_FAILED;
        }

        /* If we have attachments, retrieve them */
        if (has_attach && *has_attach) {
          // TODO
        }
      }
      mapi_object_release(&obj_message);
    }
  }

  mapi_object_release(&obj_table);
  mapi_object_release(&objFolder);

  return MAPI_E_SUCCESS;

}

void OCResource::synchronizeCollection(const Akonadi::Collection & collection)
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
  collectionSynchronized();
}

#include "ocresource.moc"
