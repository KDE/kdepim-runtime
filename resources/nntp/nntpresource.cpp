/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "nntpresource.h"
#include "nntpcollectionattribute.h"

#include <libakonadi/collectionattributefactory.h>
#include <libakonadi/collectionmodifyjob.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>

#include <kmime/kmime_message.h>

#include <QDate>
#include <QDir>
#include <QInputDialog>
#include <QLineEdit>

#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;

NntpResource::NntpResource(const QString & id)
  : ResourceBase( id )
{
  CollectionAttributeFactory::registerAttribute<NntpCollectionAttribute>();

  mConfig = settings()->value( "General/Url", QString("nntp://localhost/") ).toString();
}

NntpResource::~ NntpResource()
{
}

bool NntpResource::requestItemDelivery(const Akonadi::DataReference & ref, const QStringList &parts, const QDBusMessage & msg)
{
  Q_UNUSED( parts );
  mCurrentRef = ref;
  mCurrentMessage = msg;
  KIO::Job* job = KIO::storedGet( KUrl( ref.remoteId() ), false, false );
  connect( job, SIGNAL( result(KJob*) ), SLOT( fetchArticleResult(KJob*) ) );
  return true;
}

void NntpResource::retrieveCollections()
{
  remoteCollections.clear();
  Collection rootCollection;
  rootCollection.setParent( Collection::root() );
  rootCollection.setRemoteId( baseUrl() );
  rootCollection.setName( name() );
  rootCollection.setCachePolicyId( 1 );
  QStringList contentTypes;
  contentTypes << Collection::collectionMimeType();
  rootCollection.setContentTypes( contentTypes );
  remoteCollections << rootCollection;

  KUrl url = KUrl( baseUrl() );
  QDate lastList = settings()->value( "General/LastGroupList" ).toDate();
  if ( lastList.isValid() ) {
    mIncremental = true;
    url.addQueryItem( "since",  QString("%1%2%3 000000")
        .arg( lastList.year() % 100, 2, 10, QChar( '0' ) )
        .arg( lastList.month(), 2, 10, QChar( '0' ) )
        .arg( lastList.day(), 2, 10, QChar( '0' ) ) );
  } else {
    mIncremental = false;
  }

  KIO::Job* job = KIO::listDir( url, false, true );
  connect( job, SIGNAL(entries(KIO::Job*, const KIO::UDSEntryList&)),
           SLOT(listGroups(KIO::Job*, const KIO::UDSEntryList&)) );
  connect( job, SIGNAL( result(KJob*) ), SLOT( listGroupsResult(KJob*) ) );
}

void NntpResource::synchronizeCollection(const Akonadi::Collection & col)
{
  KUrl url = KUrl( baseUrl() );
  url.setPath( col.remoteId() );

  NntpCollectionAttribute *attr = col.attribute<NntpCollectionAttribute>();
  if ( attr && attr->lastArticle() > 0 )
    url.addQueryItem( "first", QString::number( attr->lastArticle() + 1 ) );
  else
    url.addQueryItem( "max", "5" );

  KIO::Job* job = KIO::listDir( url, false, true );
  connect( job, SIGNAL(entries(KIO::Job*, const KIO::UDSEntryList&)),
           SLOT(listGroup(KIO::Job*, const KIO::UDSEntryList&)) );
  connect( job, SIGNAL( result(KJob*) ), SLOT( listGroupResult(KJob*) ) );
}

void NntpResource::listGroups(KIO::Job * job, const KIO::UDSEntryList & list)
{
  Q_UNUSED( job );
  QString name;
  QStringList contentTypes;
  contentTypes << "message/news";
  for( KIO::UDSEntryList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it ) {
    name = (*it).stringValue( KIO::UDSEntry::UDS_NAME );
    if ( name.isEmpty() )
      continue;

    Collection c;
    c.setName( name );
    c.setRemoteId( name );
    c.setParentRemoteId( baseUrl() );
    c.setContentTypes( contentTypes );

    remoteCollections << c;
  }
}

void NntpResource::listGroupsResult(KJob * job)
{
  if ( job->error() ) {
    error( job->errorString() );
    return;
  } else {
    settings()->setValue( "General/LastGroupList", QDate::currentDate() );
  }
  if ( mIncremental )
    collectionsRetrievedIncremental( remoteCollections, Collection::List() );
  else
    collectionsRetrieved( remoteCollections );
}

void NntpResource::listGroup(KIO::Job * job, const KIO::UDSEntryList & list)
{
  Q_UNUSED( job );
  foreach ( const KIO::UDSEntry &entry, list ) {
    DataReference ref( -1, baseUrl() + currentCollection().remoteId() + QDir::separator() + entry.stringValue( KIO::UDSEntry::UDS_NAME ) );
    Item item( ref );
    item.setMimeType( "message/news" );
    ItemAppendJob *append = new ItemAppendJob( item, currentCollection(), session() );
  }
}

void NntpResource::listGroupResult(KJob * job)
{
  if ( job->error() ) {
    error( job->errorString() );
  } else {
    // store last serial number
    Collection col = currentCollection();
    NntpCollectionAttribute *attr = col.attribute<NntpCollectionAttribute>( true );
    KIO::Job *j = static_cast<KIO::Job*>( job );
    if ( j->metaData().contains( "LastSerialNumber" ) )
      attr->setLastArticle( j->metaData().value("LastSerialNumber").toInt() );
    CollectionModifyJob *modify = new CollectionModifyJob( col, session() );
    modify->setAttribute( attr );
    // TODO: check result signal
  }

  collectionSynchronized();
}

QString NntpResource::baseUrl() const
{
  if ( !mConfig.endsWith( "/" ) )
    return mConfig + '/';
  return mConfig;
}

void NntpResource::fetchArticleResult(KJob * job)
{
  if ( job->error() ) {
    error( job->errorString() );
    return;
  }
  KIO::StoredTransferJob *j = static_cast<KIO::StoredTransferJob*>( job );
  KMime::Message *msg = new KMime::Message();
  msg->setContent( j->data() );
  msg->parse();
  Item item( mCurrentRef );
  item.setMimeType( "message/news" );
  item.setPayload( MessagePtr( msg ) );
  ItemStoreJob *store = new ItemStoreJob( item, session() );
  store->storePayload();
  deliverItem( store, mCurrentMessage );
}

void NntpResource::configure()
{
  const QString tmp = QInputDialog::getText( 0, "Configuration", "Url:", QLineEdit::Normal, mConfig );
  if ( !tmp.isEmpty() ) {
    mConfig = tmp;
    settings()->setValue( "General/Url", mConfig );
  }
}

#include "nntpresource.moc"
