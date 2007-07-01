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
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/item.h>

#include <strigi/qtdbus/strigiclient.h>

#include <QDebug>

#include "mailthreaderagent.h"

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;

class MailThreaderAgent::Private
{
  public:
  MailThreaderAgent* mParent;
  StrigiClient strigi;

  Private( MailThreaderAgent *parent )
      : mParent( parent )
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

  DataReference findParent( DataReference ref )
  {
    DataReference parent;
    if ( ref.isNull() ) return parent;

    ItemFetchJob *job = new ItemFetchJob( ref, mParent->session() );
    job->addFetchPart( Item::PartEnvelope );
    if ( !job->exec() )
      return parent;

    Item item = job->items()[0];
    MessagePtr msg = item.payload<MessagePtr>();

    // Try to fetch his perfect parent using Strigi
    parent = findUsingStrigi( QString::fromLatin1("content.ID"), msg->inReplyTo()->asUnicodeString() );

    if ( !parent.isNull() )
      return parent;


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
      return parent;

    // An imperfect parent was not found using References. Try using the Subject field.
    parent = findParentBySubject( ref );

    if (!parent.isNull() )
      return parent;

    qDebug() << "There is nothing to see here, move along !" ;
    return DataReference();
  }

  DataReference findParentBySubject( DataReference ref )
  {
    DataReference parent;
    if ( ref.isNull() ) return parent;

    // Retrieve item and associated message
    ItemFetchJob *job = new ItemFetchJob( ref, mParent->session() );
    job->addFetchPart( Item::PartEnvelope );
    if ( !job->exec() )
      return parent;
    Item item = job->items()[0];
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

MailThreaderAgent::MailThreaderAgent( const QString &id )
  : AgentBase( id ),
    d( new Private( this ) )
{
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
}

void MailThreaderAgent::itemChanged( const Akonadi::Item &item, const QStringList& )
{
  qDebug() << "mailthreaderagent: looking for parent for " << item.url();
  DataReference ref = d->findParent( item.reference() );
  if ( !ref.isNull() )
  {
    ItemFetchJob *job = new ItemFetchJob( ref, session() );
    job->addFetchPart( Item::PartEnvelope );
    if ( job->exec() )
    {
      Item item = job->items()[0];
      qDebug() << "mailthreaderagent: parent found:" << item.url();
    }
  }
}

void MailThreaderAgent::itemRemoved(const Akonadi::DataReference & ref)
{
}

#include "mailthreaderagent.moc"
