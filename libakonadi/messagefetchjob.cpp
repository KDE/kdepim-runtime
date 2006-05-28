/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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
#include "messagefetchjob.h"

#include "kmime_message.h"

#include <QDebug>

using namespace PIM;

class PIM::MessageFetchJobPrivate
{
  public:
    QByteArray path;
    DataReference uid;
    Message::List messages;
    QByteArray tag;
};

PIM::MessageFetchJob::MessageFetchJob( const QByteArray & path, QObject * parent ) :
    Job( parent ),
    d( new MessageFetchJobPrivate )
{
  d->path = path;
}

PIM::MessageFetchJob::MessageFetchJob( const DataReference & ref, QObject * parent ) :
    Job( parent ),
    d( new MessageFetchJobPrivate )
{
  d->path = Collection::root();
  d->uid = ref;
}

PIM::MessageFetchJob::~ MessageFetchJob( )
{
  delete d;
}

void PIM::MessageFetchJob::doStart()
{
  QByteArray selectPath;
  if ( d->uid.isNull() ) // collection content listing
    selectPath = d->path;
  else                   // message fetching
    selectPath = /*Collection::root()*/ "res1/foo"; // ### just until the server is fixed
  CollectionSelectJob *job = new CollectionSelectJob( selectPath, this );
  connect( job, SIGNAL(done(PIM::Job*)), SLOT(selectDone(PIM::Job*)) );
  job->start();
}

void PIM::MessageFetchJob::handleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == d->tag ) {
    if ( !data.startsWith( "OK" ) )
      setError( Unknown );
    emit done( this );
    return;
  }
  if ( tag == "*" ) {
    int begin = data.indexOf( "FETCH" );
    if ( begin >= 0 ) {

      // split fetch response into key/value pairs
      QList<QByteArray> fetch = ImapParser::parseParentheziedList( data, begin + 6 );

      // create a new message object
      DataReference ref = parseUid( fetch );
      if ( ref.isNull() ) {
        qWarning() << "No UID found in fetch response - skipping";
        return;
      }
      Message* msg = new Message( ref );
      KMime::Message* mime = new KMime::Message();
      msg->setMime( mime );
      d->messages.append( msg );

      // parse fetch response fields
      for ( int i = 0; i < fetch.count() - 1; i += 2 ) {
        // flags
        if ( fetch[i] == "FLAGS" ) {
          QList<QByteArray> flags = ImapParser::parseParentheziedList( fetch[i + 1] );
          foreach ( const QByteArray flag, flags )
            msg->setFlag( flag );
        }
        // envelope
        else if ( fetch[i] == "ENVELOPE" ) {
          QList<QByteArray> env = ImapParser::parseParentheziedList( fetch[i + 1] );
          Q_ASSERT( env.count() >= 10 );
          // date
          mime->date()->from7BitString( env[0] );
          // subject
          mime->subject()->from7BitString( env[1] );
          // from
          QList<QByteArray> addrList = ImapParser::parseParentheziedList( env[2] );
          if ( addrList.count() >= 1 ) {
            QList<QByteArray> addr = ImapParser::parseParentheziedList( addrList[0] );
            Q_ASSERT( addr.count() == 4 );
            mime->from()->setName( addr[0] );
            mime->from()->setEmail( addr[2] + "@" + addr[3] );
          }
          // sender
          // not supported by kmime, skipp it
          // reply-to
          addrList = ImapParser::parseParentheziedList( env[4] );
          if ( addrList.count() >= 1 ) {
            QList<QByteArray> addr = ImapParser::parseParentheziedList( addrList[0] );
            Q_ASSERT( addr.count() == 4 );
            mime->replyTo()->setNameFrom7Bit( addr[0] );
            mime->replyTo()->setEmail( addr[2] + "@" + addr[3] );
          }
          // to
          addrList = ImapParser::parseParentheziedList( env[5] );
          foreach ( const QByteArray ba, addrList ) {
            QList<QByteArray> addr = ImapParser::parseParentheziedList( ba );
            Q_ASSERT( addr.count() == 4 );
            KMime::Headers::AddressField addrField;
            addrField.setNameFrom7Bit( addr[0] );
            addrField.setEmail( addr[2] + "@" + addr[3] );
            mime->to()->addAddress( addrField );
          }
          // cc
          addrList = ImapParser::parseParentheziedList( env[6] );
          foreach ( const QByteArray ba, addrList ) {
            QList<QByteArray> addr = ImapParser::parseParentheziedList( ba );
            Q_ASSERT( addr.count() == 4 );
            KMime::Headers::AddressField addrField;
            addrField.setNameFrom7Bit( addr[0] );
            addrField.setEmail( addr[2] + "@" + addr[3] );
            mime->cc()->addAddress( addrField );
          }
          // bcc
          addrList = ImapParser::parseParentheziedList( env[7] );
          foreach ( const QByteArray ba, addrList ) {
            QList<QByteArray> addr = ImapParser::parseParentheziedList( ba );
            Q_ASSERT( addr.count() == 4 );
            KMime::Headers::AddressField addrField;
            addrField.setNameFrom7Bit( addr[0] );
            addrField.setEmail( addr[2] + "@" + addr[3] );
            mime->bcc()->addAddress( addrField );
          }
          // in-reply-to
          // not yet supported by KMime
          // message id
          mime->messageID()->from7BitString( env[9] );
        }
        // rfc822 body
        else if ( fetch[i] == "RFC822" ) {
          mime->setContent( fetch[i + 1] );
        }
      }
      Q_ASSERT( msg );
      return;
    }
  }
  qDebug() << "Unhandled response in message fetch job: " << tag << data;
}

Message::List PIM::MessageFetchJob::messages() const
{
  return d->messages;
}

void PIM::MessageFetchJob::selectDone( PIM::Job * job )
{
  if ( job->error() ) {
    setError( job->error() );
    emit done( this );
  } else {
    job->deleteLater();
    // the collection is now selected, fetch the message(s)
    d->tag = newTag();
    QByteArray command = d->tag;
    if ( d->uid.isNull() )
      command += " FETCH 1:* (UID FLAGS ENVELOPE)";
    else
      command += " UID FETCH " + d->uid.persistanceID().toLatin1() + " (UID FLAGS RFC822)";
    writeData( command );
  }
}

DataReference PIM::MessageFetchJob::parseUid( const QList< QByteArray > & fetchResponse )
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
  return DataReference( fetchResponse[index + 1], QString() );
}

#include "messagefetchjob.moc"
