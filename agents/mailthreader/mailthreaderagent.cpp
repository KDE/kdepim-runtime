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

#include "mailthreaderagent.h"

#include <akonadi/attributefactory.h>
#include <akonadi/session.h>
#include <akonadi/monitor.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/item.h>
#include <akonadi/kmime/messageparts.h>
#include <akonadi/kmime/messagethreadingattribute.h>

#include <strigi/qtdbus/strigiclient.h>

#include <kdebug.h>
#include <kurl.h>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;

class MailThreaderAgent::Private
{
  public:
  MailThreaderAgent* mParent;
  Collection mCollection;

  StrigiClient strigi;

  Private( MailThreaderAgent *parent )
      : mParent( parent )
  {
  }

  ~Private()
  {
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
      kWarning( 5258 ) << "bigRegExp = \""
                       << bigRegExp << "\"" << endl
                       << "prefix regexp is invalid!";
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

  QList<Item::Id> findUsingStrigi( const QString& property, const QString& value )
  {
    QString query = property + QString::fromLatin1(":") + value;
    QList<StrigiHit> hits = strigi.getHits( query, 50, 0 );
    QList<Item::Id> result;

    foreach( StrigiHit hit, hits ) {
      const Item item = Item::fromUrl( hit.uri );
      if ( item.isValid() ) {
        result << item.id();
      }
    }
    return result;
  }

  QList<Item::Id> findParentBySubject( const Item& item )
  {
    QList<Item::Id> result;
    Item ref = item;
    Item parent;
    MessagePtr msg = item.payload<MessagePtr>();

    QString strippedSubject = stripOffPrefixes( msg->subject()->asUnicodeString() );
    if ( strippedSubject.isEmpty() ) // Not worth trying !
      return result;

    // Let's try by subject, but only if the  subject is prefixed.
    // This is necessary to make for example cvs commit mailing lists
    // work as expected without having to turn threading off alltogether.
    if ( msg->subject()->asUnicodeString() == strippedSubject ) // true if not prefixed
        return result;

    QString query = QString::fromLatin1("email.subject:") + strippedSubject; // ###
    QList<StrigiHit> hits = strigi.getHits( query, 50, 0 ); // max = 50. A way to change that to unlimited ? Ask Strigi developer
    /* Iterate over the list of potential parents with the same
     * subject, and take the closest one by date. */
    foreach ( StrigiHit hit, hits ) {
      // make sure it's not ourselves
      const Item parentRef = Item::fromUrl( hit.uri );

      if ( !parentRef.isValid() )
        continue;

      if ( parentRef == ref )
        continue;

      result << parentRef.id();
    }

    return result;
  }


  bool subjectIsPrefixed( const Item& item )
  {
    // ### Implement me
    return true;
  }

  /*
    Merges @p newIds into @p list and returns if there has been any change.
  */
  bool mergeIdLists( QList<Item::Id> &list, const QList<Item::Id> &newIds )
  {
    bool changed = false;
    foreach ( const Item::Id id, newIds ) {
      if ( !list.contains( id ) ) {
        list << id;
        changed = true;
      }
    }
    return changed;
  }

  /*
   * - Fetches the item before
   * - One integer instead of one integer list
   */
  bool saveUnperfectThreadingInfo( Item::Id id, Item::Id partValue )
  {
    ItemFetchJob *job = new ItemFetchJob( Item( id ) );
    job->fetchScope().fetchAttribute<MessageThreadingAttribute>();

    if ( job->exec() ) {
      Item item = job->items()[0];
      MessageThreadingAttribute *attr = item.attribute<MessageThreadingAttribute>( Item::AddIfMissing );
      QList<Item::Id> merged = attr->unperfectParents();
      bool change = mergeIdLists( merged, QList<Item::Id>() << partValue );

      // Store the new parents of this item if there was a change
      if ( change )
      {
        attr->setUnperfectParents( merged );
        ItemModifyJob *job = new ItemModifyJob( item );
        job->storePayload();
        if (!job->exec()) {
          kDebug( 5258 ) << "Unable to store threading parts!";
          return false;
        }
      }

      return true;
    }

    return false;
  }

  /*
   * Save the threading information in an item.
   */
  bool saveThreadingInfo( Item& item, QList<Item::Id> perfectParents,
                                  QList<Item::Id> unperfectParents,
                                  QList<Item::Id> subjectParents )
  {
    MessageThreadingAttribute *attr = item.attribute<MessageThreadingAttribute>( Item::AddIfMissing );
    bool changed = false;

    QList<Item::Id> merged = attr->perfectParents();
    changed = mergeIdLists( merged, perfectParents );
    attr->setPerfectParents( merged );

    merged = attr->unperfectParents();
    changed = mergeIdLists( merged, unperfectParents ) || changed;
    attr->setUnperfectParents( merged );

    merged = attr->subjectParents();
    changed = mergeIdLists( merged, subjectParents ) || changed;
    attr->setSubjectParents( merged );

    ItemModifyJob *job = new ItemModifyJob( item );
    job->storePayload();
    if (!job->exec()) {
      kDebug( 5258 ) << "Unable to store the threading parts!";
      return false;
    }
    return true;
  }

  /*
    Main algorithm to find the parent
    Returns a SortCacheItem structure with the parentId if found,
    but in each case with a mark.
   */
  void findParent( Item& item )
  {
    Item parent;
    Item ref = item;
    int mark = 0;

    if ( !ref.isValid() )
      return;

    /*
     * Extract useful info
     */
    MessagePtr msg = item.payload<MessagePtr>();
    QString messageID = msg->messageID()->asUnicodeString();
    QString inReplyTo = msg->inReplyTo()->asUnicodeString();
    QString secondReplyId = msg->references()->asUnicodeString();
    // references contains two items, use the first one
    // (the second to last reference)
    const int rightAngle = secondReplyId.indexOf( '>' );
    if( rightAngle != -1 )
      secondReplyId.truncate( rightAngle + 1 );

    bool subjectPrefixed = subjectIsPrefixed( item );
    QString strippedSubject;
    if ( subjectPrefixed )
      strippedSubject = stripOffPrefixes( msg->subject()->asUnicodeString() );

    /*
     * Search parent using Strigi
     */
    // List which will receive the parent ids
    QList<Item::Id> perfectParents;
    QList<Item::Id> unperfectParents;
    QList<Item::Id> subjectParents;

    // Try to fetch his perfect parent using Strigi
    if ( !inReplyTo.isEmpty() ) {
      perfectParents << findUsingStrigi( QString::fromLatin1("content.ID"), inReplyTo );
    }

    // Find unperfect parents using the References field
    if ( !secondReplyId.isEmpty() ) {
      // Try to fetch his imperfect parent using Strigi
      unperfectParents << findUsingStrigi( QString::fromLatin1("content.ID"), secondReplyId );
    }

    if ( subjectIsPrefixed( item ) )
    {
      // An imperfect parent was not found using References. Try using the Subject field.
      subjectParents << findParentBySubject( item );
    }

    /*
     * Store the retrieved info in the item parts.
     */
    saveThreadingInfo( item, perfectParents, unperfectParents, subjectParents );


    /*
     * Search children using Strigi
     * TODO: Strigi seems to lack a way to distinguish perfect
     * and unperfect parents in the links field. That's why parts of the code
     * below are commented out.
     */
    if ( !messageID.isEmpty() )
    {
      // QList<int> perfectChildren = findUsingStrigi( QString::fromLatin1( "content.links" ), messageId );
      QList<Item::Id> unperfectChildren = findUsingStrigi( QString::fromLatin1( "content.links" ), messageID );
      // This one will be more complex because we need to check the subject returned by strigi to have
      // no prefix and to be *exactly* strippedSubject
      // QList<int> subjectChildren = findUsingStrigi( QString::fromLatin1( "content.subject" ), strippedSubject );

      // foreach( int childId, perfectChildren ) {
      //   saveThreadingInfo( childId, PartPerfectParents, ref.id() );
      // }
      foreach( Item::Id childId, unperfectChildren ) {
        saveUnperfectThreadingInfo( childId, ref.id() );
      }
      //foreach( int childId, subjectChildren ) {
      //  saveThreadingInfo( childId, PartSubjectParents, ref.id() );
      //}
    }
  }




};

MailThreaderAgent::MailThreaderAgent( const QString &id )
  : AgentBase( id ),
    d( new Private( this ) )
{
  AttributeFactory::registerAttribute<MessageThreadingAttribute>();
  kDebug( 5258 ) << "mailtheaderagent: at your order, sir!" ;

  // Fetch the whole list, thread it.
}

MailThreaderAgent::~MailThreaderAgent()
{
  delete d;
}

void MailThreaderAgent::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
{
  Item modifiedItem = Item( item );

  d->findParent( modifiedItem );
}

void MailThreaderAgent::itemRemoved(const Akonadi::Item & ref)
{
  // TODO Find all item who have the removed one as parent _in the parts_ and remove this parent
  // from their threading parts
  // It requires Strigi to index the different parts
}

void MailThreaderAgent::collectionChanged( const Akonadi::Collection &col )
{
  // FIXME This doesn't seem to work: attribute empty
  if ( !col.hasAttribute( "MailThreaderSort" ) )
    return;

  if ( col.attribute( "MailThreaderSort" ) != QByteArray( "sort" ) )
    return;

  threadCollection( col );

  Collection c( col );
  MailThreaderAttribute *a = c.attribute<MailThreaderAttribute>( Collection::AddIfMissing );
  a->deserialize( QByteArray() );
  CollectionModifyJob *job = new CollectionModifyJob( c );
  if ( !job->exec() )
    kDebug( 5258 ) << "Unable to modify collection";
}

void MailThreaderAgent::threadCollection( const Akonadi::Collection &_col )
{
  // List collection content
  Collection col( _col );
  ItemFetchJob *fjob = new ItemFetchJob( col );
  fjob->fetchScope().fetchAttribute<MessageThreadingAttribute>();
  fjob->fetchScope().fetchPayloadPart( MessagePart::Header );
  if ( !fjob->exec() )
    return;

  Item::List items = fjob->items();

  foreach( Item item, items ) {
    d->findParent( item );
  }
}

AKONADI_AGENT_MAIN( MailThreaderAgent )

#include "mailthreaderagent.moc"
