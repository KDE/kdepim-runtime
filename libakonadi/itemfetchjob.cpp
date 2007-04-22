/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "collection.h"
#include "collectionselectjob.h"
#include "imapparser.h"
#include "itemfetchjob.h"

#include <QtCore/QDebug>

using namespace Akonadi;

class ItemFetchJob::Private
{
  public:
    Private( ItemFetchJob *parent )
      : mParent( parent )
    {
    }

    void startFetchJob();
    void selectDone( KJob * job );

    ItemFetchJob *mParent;
    Collection collection;
    DataReference uid;
    QList<QByteArray> fields;
    Item::List items;
    bool fetchData;
};

void ItemFetchJob::Private::startFetchJob()
{
  QByteArray command = mParent->newTag();
  if ( uid.isNull() )
    command += " FETCH 1:*";
  else
    command += " UID FETCH " + QByteArray::number( uid.id() );

  command += " (UID REMOTEID FLAGS";
  if ( fetchData )
    command += " RFC822";

  foreach ( QByteArray f, fields )
    command += ' ' + f;
  command += ')';
  mParent->writeData( command );
}

void ItemFetchJob::Private::selectDone( KJob * job )
{
  if ( !job->error() )
    // the collection is now selected, fetch the message(s)
    startFetchJob();
}

ItemFetchJob::ItemFetchJob(QObject * parent) :
    Job( parent ),
    d( new Private( this ) )
{
  d->fetchData = false;
}

ItemFetchJob::ItemFetchJob( const Collection &collection, QObject * parent ) :
    Job( parent ),
    d( new Private( this ) )
{
  d->collection = collection;
  d->fetchData = false;
}

ItemFetchJob::ItemFetchJob(const DataReference & ref, QObject * parent) :
    Job( parent ),
    d( new Private( this ) )
{
  setUid( ref );
  d->fetchData = true;
}

ItemFetchJob::~ ItemFetchJob( )
{
  delete d;
}

void ItemFetchJob::doStart()
{
  if ( d->uid.isNull() ) { // collection content listing
    CollectionSelectJob *job = new CollectionSelectJob( d->collection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(selectDone(KJob*)) );
    addSubjob( job );
  } else
    d->startFetchJob();
}

void ItemFetchJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == "*" ) {
    int begin = data.indexOf( "FETCH" );
    if ( begin >= 0 ) {

      // split fetch response into key/value pairs
      QList<QByteArray> fetch;
      ImapParser::parseParenthesizedList( data, fetch, begin + 6 );

      // create a new message object
      DataReference ref = parseUid( fetch );
      if ( ref.isNull() ) {
        qWarning() << "No UID found in fetch response - skipping";
        return;
      }

      Item item( ref );

      // parse fetch response fields
      for ( int i = 0; i < fetch.count() - 1; i += 2 ) {
        // flags
        if ( fetch[i] == "FLAGS" )
          parseFlags( fetch[i + 1], item );
        else if ( fetch[i] == "RFC822" ) {
          item.setData( fetch[i + 1] );
        } else if ( fetch[i] == "MIMETYPE" ) {
          item.setMimeType( QString::fromUtf8( fetch[i + 1] ) );
        }

      }

      d->items.append( item );
      return;
    }
  }
  qDebug() << "Unhandled response in message fetch job: " << tag << data;
}

Item::List ItemFetchJob::items() const
{
  return d->items;
}

DataReference ItemFetchJob::parseUid( const QList< QByteArray > & fetchResponse )
{
  int index = fetchResponse.indexOf( "UID" );
  if ( index < 0 ) {
    qWarning() << "Fetch response doesn't contain a UID field!";
    return DataReference();
  }
  if ( index == fetchResponse.count() - 1 ) {
    qWarning() << "Broken fetch response: No value for UID field!";
    return DataReference();
  }

  QString remoteId;
  int rindex = fetchResponse.indexOf( "REMOTEID" );
  if ( rindex >= 0 && fetchResponse.count() > rindex + 1 )
    remoteId = QString::fromLatin1( fetchResponse[ rindex + 1 ] );

  return DataReference( fetchResponse[index + 1].toInt(), remoteId );
}

void ItemFetchJob::addFetchField(const QByteArray & field)
{
  d->fields.append( field );
}

void ItemFetchJob::parseFlags(const QByteArray & flagData, Item &item)
{
  QList<QByteArray> flags;
  ImapParser::parseParenthesizedList( flagData, flags );
  foreach ( const QByteArray flag, flags ) {
    item.setFlag( flag );
  }
}

void ItemFetchJob::setCollection(const Collection &collection)
{
  d->collection = collection;
  d->uid = DataReference();
}

void Akonadi::ItemFetchJob::setUid(const DataReference & ref)
{
  d->collection = Collection::root();
  d->uid = ref;
}

void ItemFetchJob::fetchData(bool fetch)
{
  d->fetchData = fetch;
}

#include "itemfetchjob.moc"
