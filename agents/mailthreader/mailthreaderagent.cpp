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

#include <akonadi/session.h>
#include <akonadi/monitor.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/item.h>

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
      if ( Item::urlIsValid( hit.uri ) ) {
        const Item item = Item::fromUrl( hit.uri );
        if ( item.isValid() )
        {
          result << item.id();
        }
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
      if ( !Item::urlIsValid( hit.uri ) ) continue;
      Item parentRef = Item::fromUrl( hit.uri );

      if ( !parentRef.isValid() ) continue;
      if ( parentRef == ref ) continue;

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
   * Joint the integers in the list in a comma-separated string, and add it to item part
   * @param item The method uses this item to not add already existing parents
   * @param part The part the list will be added to
   * @param list the parent list
   * @return true if the part was changed: this to avoid unnecessary store jobs.
   */
  bool buildPartFromList( Item& item, const QLatin1String part, QList<Item::Id> list )
  {
    bool change = false;
    // Convert the current parent list to integer list
    QList<Item::Id> currentParents;
    QList<QByteArray> currentParentsStringList = item.part( part ).split( ',' );
    foreach( QByteArray s, currentParentsStringList )
      currentParents << s.toLongLong();

    QString result;
    foreach( Item::Id i, list ) {
      if ( currentParents.indexOf( i ) == -1 ) // Only add it if it's not already in
      {
        change = true;
        result += QString::number( i ) + QLatin1String( "," );
      }
    }

    item.addPart( part, result.toLatin1() );

    return change;
  }

  /*
   * - Fetches the item before
   * - One integer instead of one integer list
   */
  bool saveThreadingInfo( Item::Id id, QLatin1String part, Item::Id partValue )
  {
    ItemFetchJob *job = new ItemFetchJob( Item( id ), mParent->session() );
    job->addFetchPart( part );

    if ( job->exec() ) {
      Item item = job->items()[0];
      bool change = buildPartFromList( item, part, QList<Item::Id>() << partValue );

      // Store the new parents of this item if there was a change
      if ( change )
      {
        ItemModifyJob *job = new ItemModifyJob( item, mParent->session() );
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
    bool pC = buildPartFromList( item, PartPerfectParents, perfectParents );
    bool uC = buildPartFromList( item, PartUnperfectParents, unperfectParents );
    bool fC = buildPartFromList( item, PartSubjectParents, subjectParents );

    // If there was a change, store it
    if ( pC || uC || fC )
    {
      ItemModifyJob *job = new ItemModifyJob( item, mParent->session() );
      job->storePayload();
      if (!job->exec()) {
        kDebug( 5258 ) << "Unable to store the threading parts!";
        return false;
      }
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
        saveThreadingInfo( childId, PartUnperfectParents, ref.id() );
      }
      //foreach( int childId, subjectChildren ) {
      //  saveThreadingInfo( childId, PartSubjectParents, ref.id() );
      //}
    }
  }




};

const QLatin1String MailThreaderAgent::PartPerfectParents = QLatin1String( "AkonadiMailThreaderAgentPerfectParents" );
const QLatin1String MailThreaderAgent::PartUnperfectParents = QLatin1String( "AkonadiMailThreaderAgentUnperfectParents" );
const QLatin1String MailThreaderAgent::PartSubjectParents = QLatin1String( "AkonadiMailThreaderAgentSubjectParents" );

MailThreaderAgent::MailThreaderAgent( const QString &id )
  : AgentBase( id ),
    d( new Private( this ) )
{
  kDebug( 5258 ) << "mailtheaderagent: at your order, sir!" ;

  // Fetch the whole list, thread it.
}

MailThreaderAgent::~MailThreaderAgent()
{
  delete d;
}

void MailThreaderAgent::aboutToQuit()
{

}

void MailThreaderAgent::configure( WId windowId )
{
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
  MailThreaderAttribute *a = c.attribute<MailThreaderAttribute>( true );
  a->setData( QByteArray() );
  CollectionModifyJob *job = new CollectionModifyJob( c );
  if ( !job->exec() )
    kDebug( 5258 ) << "Unable to modify collection";
}

void MailThreaderAgent::threadCollection( const Akonadi::Collection &_col )
{
  // List collection content
  Collection col( _col );
  ItemFetchJob *fjob = new ItemFetchJob( col, session() );
  fjob->addFetchPart( PartPerfectParents );
  fjob->addFetchPart( PartUnperfectParents );
  fjob->addFetchPart( PartSubjectParents );
  fjob->fetchAllParts(); // ### Why should I use this to have the message in-reply-to and so on (PartEnvelope doesn't work)
  if ( !fjob->exec() )
    return;

  Item::List items = fjob->items();

  foreach( Item item, items ) {
    d->findParent( item );
  }
}

#include "mailthreaderagent.moc"
