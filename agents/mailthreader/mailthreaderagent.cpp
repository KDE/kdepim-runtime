/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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


#include <libakonadi/session.h>
#include <libakonadi/monitor.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/item.h>

#include <strigi/qtdbus/strigiclient.h>

#include <QDebug>

#include "mailthreaderagent.h"

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;

class SortCacheItem
{
public:
  explicit SortCacheItem( DataReference r )
  {
    ref = r;
  }

  DataReference ref;
  int mark; // 0, perfect sort; -1, to sort; -2, don't even try to sort it
  int parentId;
  QList<SortCacheItem*> perfectChildren;
  QMap<int,SortCacheItem*> unperfectChildren;
};

class MailThreaderAgent::Private
{
  public:
  MailThreaderAgent* mParent;
  Collection mCollection;

  StrigiClient strigi;

  Item::List mItems;

  QList<SortCacheItem*> mRootChildren;
  QMap<int, SortCacheItem*> mItemMap;
  QMap<int, SortCacheItem*> mImperfectlyThreadedItems;

  Private( MailThreaderAgent *parent )
      : mParent( parent )
  {
  }

  ~Private()
  {
    clearCache();
  }

  void clearCache()
  {
    mRootChildren.clear();
    mImperfectlyThreadedItems.clear();

    foreach(SortCacheItem *item, mItemMap)
      delete item;
    mItemMap.clear();
  }

  /*
    Reads the elements in the current collection. Loads in the cache structures
    the information which is in PartSort and PartParent.
  */
  void readCache()
  {
    if ( !mCollection.isValid() )
      return;

    ItemFetchJob *fjob = new ItemFetchJob( mCollection, mParent->session() );
    fjob->addFetchPart( PartSort );
    fjob->addFetchPart( PartParent );
    fjob->addFetchPart( Item::PartAll ); // ### Why should I use this to have the message in-reply-to and so on (PartEnvelope doesn't work)
    if ( !fjob->exec() )
      return;

    mItemMap.clear();
    mItems = fjob->items();
    foreach( Item it, mItems ) {
      SortCacheItem *sItem = new SortCacheItem( it.reference() );
      mItemMap[ it.reference().id() ] = sItem; // ### Avoid copying the item using pointers?
    }

    /*
      First reading the "cache" stored in the item parts
    */
    foreach( Item it, mItems ) {
      bool ok;
      int parentId = it.part( PartParent ).toInt( &ok );
      int sortState = it.part( PartSort ).toInt();
      if ( ok )
      {
        if ( mItemMap[ parentId ]->ref.isNull() ) // has a parent part but parent not in collection anymore
        {
          mItemMap[ it.reference().id() ]->mark = -1;
        }
        else
        {
          mItemMap[ it.reference().id() ]->mark = sortState; // 0  or -1
          mItemMap[ it.reference().id() ]->parentId = parentId;
          if ( sortState == 0 )
            mItemMap[ parentId ]->perfectChildren << mItemMap[ it.reference().id() ];
          else if ( sortState == -1 )
            mItemMap[ parentId ]->unperfectChildren[ it.reference().id() ] = mItemMap[ it.reference().id()];
        }
      }
      else // no parent part
        if ( sortState == -2 ) // keep this mark if already set
          mItemMap[ it.reference().id() ]->mark = -2;
        else
          mItemMap[ it.reference().id() ]->mark = -1;
    }

  }


  // From kmmsgbase.cpp
  QString stripOffPrefixes( const QString& str )
  {
    // ### Used to be configurable in KMail (see kmmsgbase)
    QStringList sReplySubjPrefixes, sForwardSubjPrefixes;
    sReplySubjPrefixes << "Re\\s*:" << "Re\\[\\d+\\]:" << "Re\\d+:";
    sForwardSubjPrefixes << "Fwd:" << "FW:";

    return replacePrefixes( str, sReplySubjPrefixes + sForwardSubjPrefixes,
                            true, QString() ).trimmed();
  }

  // From kmmsgbase.cpp
  QString replacePrefixes( const QString& str,
                                      const QStringList& prefixRegExps,
                                      bool replace,
                                      const QString& newPrefix )
  {
    bool recognized = false;
    // construct a big regexp that
    // 1. is anchored to the beginning of str (sans whitespace)
    // 2. matches at least one of the part regexps in prefixRegExps
    QString bigRegExp = QString::fromLatin1("^(?:\\s+|(?:%1))+\\s*")
                        .arg( prefixRegExps.join(")|(?:") );
    QRegExp rx( bigRegExp, Qt::CaseInsensitive );
    if ( !rx.isValid() ) {
      kWarning(5006) << "KMMessage::replacePrefixes(): bigRegExp = \""
                      << bigRegExp << "\"\n"
                      << "prefix regexp is invalid!" << endl;
      // try good ole Re/Fwd:
      recognized = str.startsWith( newPrefix );
    } else { // valid rx
      QString tmp = str;
      if ( rx.indexIn( tmp ) == 0 ) {
        recognized = true;
        if ( replace )
          return tmp.replace( 0, rx.matchedLength(), newPrefix + ' ' );
      }
    }
    if ( !recognized )
      return newPrefix + ' ' + str;
    else
      return str;
  }

  DataReference findUsingStrigi( QString property, QString value )
  {
    QString query = property + QString::fromLatin1(":") + value;
    QList<StrigiHit> hits = strigi.getHits( query, 1, 0 );
    if ( hits.count() )
    {
      StrigiHit hit = hits[0];
      return Item::fromUrl( hit.uri );
    }
    else
      return DataReference();
  }

  DataReference findParentInCache( const Item& item )
  {
    QByteArray partParent = item.part( PartParent );
    if ( !partParent.isEmpty() )
      return DataReference( item.part( partParent ).toInt(), QString() );
  }

  bool subjectIsPrefixed( const Item& item )
  {
    // ### Implement me
    return false;
  }

  DataReference findParent( const Item& item, int *mark )
  {
    DataReference parent;
    DataReference ref = item.reference();

    MessagePtr msg = item.payload<MessagePtr>();

    // Try to fetch his perfect parent using Strigi
    QString inReplyTo = msg->inReplyTo()->asUnicodeString();
    parent = findUsingStrigi( QString::fromLatin1("content.ID"), inReplyTo );
    if ( !parent.isNull() )
    {
      *mark = 0;
      return parent;
    }

    // From now on you can't be perfectly threaded
    *mark = -1;

    // A perfect parent was not found: try to find one using the References field
    QString secondReplyId = msg->references()->asUnicodeString();
    // references contains two items, use the first one
    // (the second to last reference)
    const int rightAngle = secondReplyId.indexOf( '>' );
    if( rightAngle != -1 )
      secondReplyId.truncate( rightAngle + 1 );

    // Try to fetch his imperfect parent using Strigi
    parent = findUsingStrigi( QString::fromLatin1("content.ID"), secondReplyId );
    if ( !parent.isNull() )
    {
      return parent;
    }

    if ( subjectIsPrefixed( item ) )
    {
      // An imperfect parent was not found using References. Try using the Subject field.
      parent = findParentBySubject( item );
      if (!parent.isNull() )
        return parent;
      else
        return DataReference();
    }

    if ( !inReplyTo.isEmpty() || !secondReplyId.isEmpty() )
      return DataReference(); // Still a hope, *mark = -1

    // Desperate case
    *mark = -2;
    return DataReference();
  }

  DataReference findParentBySubject( const Item& item )
  {
    DataReference ref = item.reference();
    DataReference parent;
    MessagePtr msg = item.payload<MessagePtr>();

    QString strippedSubject = stripOffPrefixes( msg->subject()->asUnicodeString() );
    if ( strippedSubject.isEmpty() ) // Not worth trying !
      return parent;

    // Let's try by subject, but only if the  subject is prefixed.
    // This is necessary to make for example cvs commit mailing lists
    // work as expected without having to turn threading off alltogether.
    if ( msg->subject()->asUnicodeString() == strippedSubject ) // true if not prefixed
        return parent;

    QString query = QString::fromLatin1("email.subject:") + strippedSubject; // ###
    QList<StrigiHit> hits = strigi.getHits( query, 50, 0 ); // max = 50. A way to change that to unlimited ? Ask Strigi developer
    if ( hits.count() ) {
        /* Iterate over the list of potential parents with the same
         * subject, and take the closest one by date. */
        foreach ( StrigiHit hit, hits ) {
            // make sure it's not ourselves
            DataReference parentRef = Item::fromUrl( hit.uri );
            if ( parentRef == ref ) continue;

            ItemFetchJob *job = new ItemFetchJob( parentRef, mParent->session() );
            job->addFetchPart( Item::PartEnvelope );
            if ( !job->exec() ) continue;

            Item potentialParentItem = job->items()[0];
            MessagePtr mb = potentialParentItem.payload<MessagePtr>();

            int delta = mb->date()->dateTime().secsTo( msg->date()->dateTime() );
            // delta == 0 is not allowed, to avoid circular threading
            // with duplicates.
            if (delta > 0 ) {
                // Don't use parents more than 6 weeks older than us.
                if (delta < 3628899)
                    parent = parentRef;
                break;
            }
        }
    }
    return parent;
  }


};

const QLatin1String MailThreaderAgent::PartParent = QLatin1String( "AkonadiMailThreaderAgentParent" );
const QLatin1String MailThreaderAgent::PartSort = QLatin1String( "AkonadiMailThreaderAgentSort" );

MailThreaderAgent::MailThreaderAgent( const QString &id )
  : AgentBase( id ),
    d( new Private( this ) )
{
  qDebug() << "mailtheaderagent: at your order, sir!" ;


  // Fetch the whole list, thread it.
}

MailThreaderAgent::~MailThreaderAgent()
{
  delete d;
}

void MailThreaderAgent::aboutToQuit()
{

}

void MailThreaderAgent::configure()
{
}

void MailThreaderAgent::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
{
  qDebug() << "youhouuuuuu" ;
  findParentAndMark( item );
}

void MailThreaderAgent::itemChanged( const Akonadi::Item &item, const QStringList& )
{
  // TODO We only have something to do here if the collection has changed for the item
  qDebug() << "youhouuuuu2";
  findParentAndMark( item );
}

void MailThreaderAgent::itemRemoved(const Akonadi::DataReference & ref)
{
  // TODO Find all item who have the removed one as parent and remove their PartParent

}

void MailThreaderAgent::findParentAndMark( const Akonadi::Item &item )
{
  Item modifiedItem = item;
  int mark;

  qDebug() << "mailthreaderagent: looking for parent for new item " << item.url();

  DataReference ref = d->findParent( item, &mark );
  if ( !ref.isNull() )
  {
    qDebug() << "mailthreaderagent: parent found with id " << ref.id();
    Item modifiedItem = item;
    modifiedItem.addPart( PartParent, QByteArray::number( ref.id() ) );
    modifiedItem.addPart( PartSort, QByteArray::number( mark ) );
    ItemStoreJob *sJob = new ItemStoreJob( modifiedItem, session() );
    sJob->storePayload();
    if (sJob->exec())
      qDebug() << "... and this has been notified to the storage !";
    else
      qDebug() << "Unable to store the parent of this item in the storage !";
  }
}

QList<DataReference> MailThreaderAgent::childrenOf( const Item& item )
{
  QList<DataReference> items; //preallocate
  foreach( SortCacheItem* sItem, d->mItemMap[ item.reference().id() ]->perfectChildren ) {
    items << sItem->ref;
  }
  foreach( SortCacheItem* sItem, d->mItemMap[ item.reference().id() ]->unperfectChildren ) {
    items << sItem->ref;
  }

  return items;
}

void MailThreaderAgent::setCollection( const Akonadi::Collection &col )
{
  d->clearCache();

  d->mCollection = col;
  d->readCache();

  threadCollection();
}

// TODO Emit a signal if a parent is eventually found for an item
void MailThreaderAgent::threadCollection()
{
  /*
    Find the parent for items whose part is -1
  */
  foreach( Item item, d->mItems ) {
    if ( d->mItemMap[ item.reference().id() ]->mark == 0 )
      continue; // has already his perfect parent !
    int mark;
    qDebug() << item.url();
    DataReference parentRef = d->findParent( item, &mark );
    if ( mark == 0 )
    {
      d->mItemMap[ item.reference().id() ]->mark = 0;
      d->mItemMap[ item.reference().id() ]->parentId = parentRef.id();
      d->mItemMap[ parentRef.id() ]->perfectChildren << d->mItemMap[ item.reference().id() ];
      d->mItemMap[ parentRef.id() ]->unperfectChildren.remove( item.reference().id() );
    }
    else if ( mark == -1 )
    {
      d->mItemMap[ item.reference().id() ]->mark = -1;
      d->mItemMap[ item.reference().id() ]->parentId = parentRef.id();
      d->mItemMap[ parentRef.id() ]->unperfectChildren[ item.reference().id() ] = d->mItemMap[ item.reference().id()];
      d->mImperfectlyThreadedItems[ item.reference().id() ] = d->mItemMap[ item.reference().id() ];
    }
    else
    {
      d->mItemMap[ item.reference().id() ]->mark = -2;
      d->mRootChildren << d->mItemMap[ item.reference().id()];
    }
  }

  // TODO Write the cache
}

#include "mailthreaderagent.moc"
