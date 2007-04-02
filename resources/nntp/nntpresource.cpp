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

#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>

#include <QDir>
#include <QInputDialog>
#include <QLineEdit>

using namespace Akonadi;

NntpResource::NntpResource(const QString & id)
  : ResourceBase( id )
{
  mConfig = settings()->value( "General/Url", QString("nntp://localhost/") ).toString();
}

NntpResource::~ NntpResource()
{
}

bool NntpResource::requestItemDelivery(const Akonadi::DataReference & ref, int type, const QDBusMessage & msg)
{
  Q_UNUSED( type );
  mCurrentRef = ref;
  mCurrentMessage = msg;
  KIO::Job* job = KIO::storedGet( KUrl( ref.externalUrl().toString() ), false, false );
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
  QStringList contentTypes;
  contentTypes << Collection::collectionMimeType();
  rootCollection.setContentTypes( contentTypes );
  remoteCollections << rootCollection;

  KUrl url = KUrl( baseUrl() );
  KIO::Job* job = KIO::listDir( url, false, true );
  connect( job, SIGNAL(entries(KIO::Job*, const KIO::UDSEntryList&)),
           SLOT(listGroups(KIO::Job*, const KIO::UDSEntryList&)) );
  connect( job, SIGNAL( result(KJob*) ), SLOT( listGroupsResult(KJob*) ) );
}

void NntpResource::synchronizeCollection(const Akonadi::Collection & col)
{
  mCurrentCollection = col;
  KUrl url = KUrl( baseUrl() );
  url.setPath( col.remoteId() );
  url.addQueryItem( "max", "100" );
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
    name = (*it).stringValue( KIO::UDS_NAME );
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
  }
  collectionsRetrieved( remoteCollections );
}

void NntpResource::listGroup(KIO::Job * job, const KIO::UDSEntryList & list)
{
  Q_UNUSED( job );
  foreach ( const KIO::UDSEntry &entry, list ) {
    ItemAppendJob *append = new ItemAppendJob( mCurrentCollection, "message/news", session() );
    append->setRemoteId( baseUrl() + mCurrentCollection.remoteId() + QDir::separator() + entry.stringValue( KIO::UDS_NAME ) );
  }
}

void NntpResource::listGroupResult(KJob * job)
{
  if ( job->error() ) {
    error( job->errorString() );
    return;
  }
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
  ItemStoreJob *store = new ItemStoreJob( mCurrentRef, session() );
  store->setData( j->data() );
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
