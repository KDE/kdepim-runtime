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
#include "configdialog.h"
#include "settings.h"

#include <libakonadi/collectionattributefactory.h>
#include <libakonadi/collectionmodifyjob.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>

#include <kmime/kmime_message.h>
#include <kmime/kmime_newsarticle.h>
#include <kmime/kmime_util.h>

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
}

NntpResource::~ NntpResource()
{
}

bool NntpResource::retrieveItem(const Akonadi::Item& item, const QStringList &parts)
{
  Q_UNUSED( parts );
  KIO::Job* job = KIO::storedGet( KUrl( item.reference().remoteId() ), KIO::NoReload, KIO::HideProgressInfo );
  setupKioJob( job );
  connect( job, SIGNAL( result(KJob*) ), SLOT( fetchArticleResult(KJob*) ) );
  return true;
}

void NntpResource::retrieveCollections()
{
  remoteCollections.clear();
  Collection rootCollection;
  rootCollection.setParent( Collection::root() );
  rootCollection.setRemoteId( baseUrl().url() );
  rootCollection.setName( name() );
  rootCollection.setCachePolicyId( 1 );
  QStringList contentTypes;
  contentTypes << Collection::collectionMimeType();
  rootCollection.setContentTypes( contentTypes );
  remoteCollections << rootCollection;

  KUrl url = baseUrl();
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

  KIO::Job* job = KIO::listDir( url, KIO::HideProgressInfo, true );
  setupKioJob( job );
  connect( job, SIGNAL(entries(KIO::Job*, const KIO::UDSEntryList&)),
           SLOT(listGroups(KIO::Job*, const KIO::UDSEntryList&)) );
  connect( job, SIGNAL( result(KJob*) ), SLOT( listGroupsResult(KJob*) ) );
}

void NntpResource::retrieveItems(const Akonadi::Collection & col, const QStringList &parts)
{
  Q_UNUSED( parts );
  KUrl url = baseUrl();
  url.setPath( col.remoteId() );

  NntpCollectionAttribute *attr = col.attribute<NntpCollectionAttribute>();
  if ( attr && attr->lastArticle() > 0 )
    url.addQueryItem( "first", QString::number( attr->lastArticle() + 1 ) );
  else
    url.addQueryItem( "max", QString::number( Settings::self()->maxDownload() ) );

  KIO::Job* job = KIO::listDir( url, KIO::HideProgressInfo, true );
  setupKioJob( job );
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
    c.setParentRemoteId( baseUrl().url() );
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
    KUrl url = baseUrl();
    url.setPath( currentCollection().remoteId() + "/" + entry.stringValue( KIO::UDSEntry::UDS_NAME ) );
    DataReference ref( -1, url.url() );
    Item item( ref );
    item.setMimeType( "message/news" );

    KMime::NewsArticle *art = new KMime::NewsArticle();
    foreach ( uint field, entry.listFields() ) {
      if ( field >= KIO::UDSEntry::UDS_EXTRA && field <= KIO::UDSEntry::UDS_EXTRA_END ) {
        const QString value = entry.stringValue( field );
        int pos = value.indexOf( ':' );
        if ( pos >= value.length() - 1 )
          continue; // value is empty
        const QString hdrName = value.left( pos );
        const QString hdrValue = value.right( value.length() - ( hdrName.length() + 2 ) );

        if ( hdrName == "Subject" ) {
          art->subject()->from7BitString( hdrValue.toLatin1() );
          if ( art->subject()->isEmpty() )
            art->subject()->fromUnicodeString( i18n("no subject"), art->defaultCharset() );
        } else if ( hdrName == "From" ) {
          art->from()->from7BitString( hdrValue.toLatin1() );
        } else if ( hdrName == "Date" ) {
          art->date()->from7BitString( hdrValue.toLatin1() );
        } else if ( hdrName == "Message-ID" ) {
          art->messageID()->from7BitString( hdrValue.simplified().toLatin1() );
        } else if ( hdrName == "References" ) {
          if( !hdrValue.isEmpty() )
            art->references()->from7BitString( hdrValue.toLatin1() );
        } else if ( hdrName == "Lines" ) {
          art->lines()->setNumberOfLines( hdrValue.toInt() );
        } else {
          // optional extra headers
          art->setHeader( new KMime::Headers::Generic( hdrName.toLatin1(), art, hdrValue.toLatin1() ) );
        }
      }
    }

    item.setPayload( MessagePtr( art ) );
    ItemAppendJob *append = new ItemAppendJob( item, currentCollection(), session() );
    // TODO: check error
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

  itemsRetrieved();
}

KUrl NntpResource::baseUrl() const
{
  KUrl url;
 if ( Settings::self()->encryption() == Settings::SSL )
    url.setProtocol( "nntps" );
  else
    url.setProtocol( "nntp" );
  url.setHost( Settings::self()->server() );
  url.setPort( Settings::self()->port() );
  if ( Settings::self()->requiresAuthentication() ) {
    url.setUser( Settings::self()->userName() );
    url.setPass( Settings::self()->password() );
  }
  return url;
}

void NntpResource::fetchArticleResult(KJob * job)
{
  if ( job->error() ) {
    error( job->errorString() );
    return;
  }
  KIO::StoredTransferJob *j = static_cast<KIO::StoredTransferJob*>( job );
  KMime::Message *msg = new KMime::Message();
  msg->setContent( KMime::CRLFtoLF( j->data() ) );
  msg->parse();
  Item item = currentItem();
  item.setMimeType( "message/news" );
  item.setPayload( MessagePtr( msg ) );
  itemRetrieved( item );
}

void NntpResource::configure()
{
  ConfigDialog dlg;
  dlg.exec();
}

void NntpResource::setupKioJob(KIO::Job * job) const
{
  Q_ASSERT( job );
  if ( Settings::self()->encryption() == Settings::TLS )
    job->addMetaData( "tls", "on" );
  else
    job->addMetaData( "tls", "off" );
  // TODO connect percent and status message signals to something
}

#include "nntpresource.moc"
