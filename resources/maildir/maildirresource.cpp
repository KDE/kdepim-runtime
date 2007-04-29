/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

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

#include "maildirresource.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtDBus/QDBusConnection>

#include <kdebug.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <klocale.h>

#include <libakonadi/collectionlistjob.h>
#include <libakonadi/collectionmodifyjob.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>


using namespace Akonadi;

MaildirResource::MaildirResource( const QString &id )
    :ResourceBase( id )
{
  loadDirectory();
}

MaildirResource::~ MaildirResource()
{
}

bool MaildirResource::requestItemDelivery( const Akonadi::DataReference &ref, int, const QDBusMessage &msg )
{
  qDebug() << "MaildirResource::requestItemDelivery()";
  /*
  Incidence *incidence = mCalendar->incidence( ref.remoteId() );
  if ( !incidence ) {
    error( QString("Incidence with uid '%1' not found!").arg( ref.remoteId() ) );
    return false;
  } else {
    MaildirFormat format;
    QByteArray data = format.toString( incidence ).toUtf8();

    ItemStoreJob *job = new ItemStoreJob( ref, session() );
    job->setData( data );
    return deliverItem( job, msg );
  }
   */
}

void MaildirResource::aboutToQuit()
{
  kDebug() << "Implement me: " << k_funcinfo << endl;
}

void MaildirResource::configure()
{
  QString oldDir = settings()->value( "General/Path" ).toString();
  KUrl url;
  if ( !oldDir.isEmpty() )
    url = KUrl::fromPath( oldDir );
  else
    url = KUrl::fromPath( QDir::homePath() );
  QString newDir = KFileDialog::getExistingDirectory( url, 0, i18n("Select Mail Directory") );
  if ( newDir.isEmpty() )
    return;
  if ( oldDir == newDir )
    return;
  settings()->setValue( "General/Path", newDir );
  loadDirectory();
  synchronize();
}

void MaildirResource::loadDirectory()
{
  kDebug() << "Implement me: " << k_funcinfo << endl;
}

void MaildirResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& )
{
  kDebug() << "Implement me: " << k_funcinfo << endl;
}

void MaildirResource::itemChanged( const Akonadi::Item&, const QStringList& )
{
  kDebug() << "Implement me: " << k_funcinfo << endl;
}

void MaildirResource::itemRemoved(const Akonadi::DataReference & ref)
{
  kDebug() << "Implement me: " << k_funcinfo << endl;
 }

void MaildirResource::retrieveCollections()
{
  Collection c;
  c.setParent( Collection::root() );
  c.setRemoteId( settings()->value( "General/Path" ).toString() );
  c.setName( name() );
  QStringList mimeTypes;
  mimeTypes << "message/rfc822";
  c.setContentTypes( mimeTypes );
  c.setCachePolicyId( 1 ); // ### just for testing
  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

void MaildirResource::synchronizeCollection(const Akonadi::Collection & col)
{
/*
  ItemFetchJob *fetch = new ItemFetchJob( col, session() );
  if ( !fetch->exec() ) {
    changeStatus( Error, i18n("Unable to fetch listing of collection '%1': %2", col.name(), fetch->errorString()) );
    return;
  }

  changeProgress( 0 );

  Item::List items = fetch->items();
  Incidence::List incidences = mCalendar->incidences();

  int counter = 0;
  foreach ( Incidence *incidence, incidences ) {
    QString uid = incidence->uid();
    bool found = false;
    foreach ( Item item, items ) {
      if ( item.reference().remoteId() == uid ) {
        found = true;
        break;
      }
    }
    if ( found )
      continue;
    Item item( DataReference( -1, uid ) );
    item.setMimeType( "text/calendar" );
    ItemAppendJob *append = new ItemAppendJob( item, col, session() );
    if ( !append->exec() ) {
      changeProgress( 0 );
      changeStatus( Error, i18n("Appending new incidence failed: %1", append->errorString()) );
      return;
    }

    counter++;
    int percentage = (counter * 100) / incidences.count();
    changeProgress( percentage );
  }

  collectionSynchronized();
 */
}

#include "maildirresource.moc"
