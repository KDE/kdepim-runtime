/******************************************************************************
 *
 *  File : datarfc822.cpp
 *  Created on Sat 20 Jun 2009 14:36:26 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Mail Filtering Agent
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "datarfc822.h"

#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemcopyjob.h>
#include <akonadi/itemmovejob.h>

#include <akonadi/kmime/messageparts.h>

#include <akonadi/filter/datamemberdescriptor.h>
#include <akonadi/filter/componentfactoryrfc822.h>

#include <KLocale>
#include <KDebug>

DataRfc822::DataRfc822( const Akonadi::Item &item )
  : Data(), mItem( item ), mFetchedBody( false )
{
}

DataRfc822::~DataRfc822()
{
}

bool DataRfc822::executeCommand( const Akonadi::Filter::CommandDescriptor * command, const QList< QVariant > &params, QString &error )
{
  switch( command->id() )
  {
    case Akonadi::Filter::CommandRfc822MoveMessageToCollection:
    {
      Q_ASSERT( params.count() >= 1 ); // must have at least one parameter (tollerate buggy sieve scripts which have more)
      bool ok;
      qlonglong id = params.first().toLongLong( &ok );
      if ( !ok )
      {
        kWarning() << "Moving the item" << mItem.id() << "failed: the target collection id is not valid";
        error = i18n( "The target collection doesn't exist" );
        return false;
      }

      Akonadi::ItemMoveJob * job = new Akonadi::ItemMoveJob( mItem, Akonadi::Collection( id ) );
      if ( !job->exec() )
      {
        kWarning() << "Moving the item" << mItem.id() << "failed:" << job->errorString();
        error = job->errorString();
        return false;
      }
      // The Akonadi::Job docs say that the job deletes itself...
      return true;
    }
    break;
    case Akonadi::Filter::CommandRfc822CopyMessageToCollection:
    {
      Q_ASSERT( params.count() >= 1 ); // must have at least one parameter (tollerate buggy sieve scripts which have more)
      bool ok;
      qlonglong id = params.first().toLongLong( &ok );
      if ( !ok )
      {
        kWarning() << "Copying the item" << mItem.id() << "failed: the target collection id is not valid";
        error = i18n( "The target collection doesn't exist" );
        return false;
      }

      Akonadi::ItemCopyJob * job = new Akonadi::ItemCopyJob( mItem, Akonadi::Collection( id ) );
      if ( !job->exec() )
      {
        kWarning() << "Copying the item" << mItem.id() << "failed:" << job->errorString();
        error = job->errorString();
        return false;
      }
      // The Akonadi::Job docs say that the job deletes itself...
      return true;
    }
    break;
    case Akonadi::Filter::CommandRfc822DeleteMessage:
    {
      Akonadi::ItemDeleteJob * job = new Akonadi::ItemDeleteJob( mItem );
      if ( !job->exec() )
      {
        kWarning() << "Deleting the item" << mItem.id() << "failed:" << job->errorString();
        error = job->errorString();
        return false;
      }
      // The Akonadi::Job docs say that the job deletes itself...

      Q_ASSERT( command->isTerminal() );
      return true;
    }
    break;
    case Akonadi::Filter::CommandRfc822RunProgram:
      // TODO
      return true;
    break;
    case Akonadi::Filter::CommandRfc822PipeThrough:
      // TODO
      return true;
    break;
    default:
      // not my command: fall through
    break;
  }

  return Data::executeCommand( command, params, error );
}


QVariant DataRfc822::getPropertyValue( const Akonadi::Filter::FunctionDescriptor * function, const Akonadi::Filter::DataMemberDescriptor * dataMember )
{
  // Optimize certain properties, when possible

  if( !mMessage )
  {
    // We need AT LEAST the header.
    if( !fetchHeader() )
    {
      kWarning() << "Fetching message header failed!";
      return QVariant();
    }
  }

  switch( function->id() )
  {
    case Akonadi::Filter::FunctionDateIn:
      if( dataMember->id() == Akonadi::Filter::DataMemberRfc822Date )
      {
        // This is pre-parsed by KMime
        kDebug() << "Returning message Date in QDateTime format (string is '" << mMessage->date()->asUnicodeString() << "')";
        return QVariant( mMessage->date()->dateTime().dateTime() );
      }
    break;
  }

  return Data::getPropertyValue( function, dataMember );
}

QVariant DataRfc822::getDataMemberValue( const Akonadi::Filter::DataMemberDescriptor * dataMember )
{
  kDebug() << "Fetching data member" << dataMember->name();

  if( !mMessage )
  {
    // We need AT LEAST the header.
    if( !fetchHeader() )
    {
      kWarning() << "Fetching message header failed!";
      return QVariant();
    }
  }

  switch( dataMember->id() )
  {
    case Akonadi::Filter::DataMemberRfc822FromHeader:
      kDebug() << "Returning From header '" << mMessage->from()->asUnicodeString() << "'";
      return mMessage->from()->asUnicodeString();
    break;
    case Akonadi::Filter::DataMemberRfc822ToHeader:
      kDebug() << "Returning To header '" << mMessage->to()->asUnicodeString() << "'";
      return mMessage->to()->asUnicodeString();
    break;
    case Akonadi::Filter::DataMemberRfc822ReplyToHeader:
      kDebug() << "Returning Reply-To header '" << mMessage->replyTo()->asUnicodeString() << "'";
      return mMessage->replyTo()->asUnicodeString();
    break;
    case Akonadi::Filter::DataMemberRfc822SubjectHeader:
      kDebug() << "Returning Subject header '" << mMessage->subject()->asUnicodeString() << "'";
      return mMessage->subject()->asUnicodeString();
    break;
    case Akonadi::Filter::DataMemberRfc822CcHeader:
      kDebug() << "Returning CC header '" << mMessage->cc()->asUnicodeString() << "'";
      return mMessage->cc()->asUnicodeString();
    break;
    case Akonadi::Filter::DataMemberRfc822BccHeader:
      kDebug() << "Returning BCC header '" << mMessage->bcc()->asUnicodeString() << "'";
      return mMessage->bcc()->asUnicodeString();
    break;
    case Akonadi::Filter::DataMemberRfc822MessageID:
      kDebug() << "Returning MessageId header '" << mMessage->messageID()->asUnicodeString() << "'";
      return mMessage->messageID()->asUnicodeString();
    break;
    case Akonadi::Filter::DataMemberRfc822Date:
      kDebug() << "Returning Date header '" << mMessage->date()->asUnicodeString() << "'";
      return mMessage->date()->asUnicodeString();
    break;
    case Akonadi::Filter::DataMemberRfc822Organization:
      kDebug() << "Returning Organization header '" << mMessage->organization()->asUnicodeString() << "'";
      return mMessage->organization()->asUnicodeString();
    break;
    case Akonadi::Filter::DataMemberRfc822References:
      kDebug() << "Returning References header '" << mMessage->references()->asUnicodeString() << "'";
      return mMessage->references()->asUnicodeString();
    break;
    case Akonadi::Filter::DataMemberRfc822UserAgent:
      kDebug() << "Returning UserAgent header '" << mMessage->userAgent()->asUnicodeString() << "'";
      return mMessage->userAgent()->asUnicodeString();
    break;
    case Akonadi::Filter::DataMemberRfc822InReplyTo:
      kDebug() << "Returning InReplyTo header '" << mMessage->inReplyTo()->asUnicodeString() << "'";
      return mMessage->inReplyTo()->asUnicodeString();
    break;
    case Akonadi::Filter::DataMemberRfc822AllRecipientHeaders:
    {
      QStringList recipients;
      QString hdr = mMessage->to()->asUnicodeString();
      if( !hdr.isEmpty() )
        recipients << hdr;
      hdr = mMessage->cc()->asUnicodeString();
      if( !hdr.isEmpty() )
        recipients << hdr;
      hdr = mMessage->bcc()->asUnicodeString();
      if( !hdr.isEmpty() )
        recipients << hdr;
      return recipients;
    }
    break;
    case Akonadi::Filter::DataMemberRfc822AllHeaders:
      // FIXME: Which is the head encoding ? ... should we decode all the headers ?
      return QString::fromAscii( mMessage->head() );
    break;
    case Akonadi::Filter::DataMemberRfc822MessageBody:
      if( !mFetchedBody )
      {
        if( !fetchBody() )
        {
          kWarning() << "Fetching message body failed!";
          return QVariant();
        }
      }
      // Hum.. not sure about this..
      return QString::fromAscii( mMessage->body() );
    break;
    case Akonadi::Filter::DataMemberRfc822WholeMessage:
      if( !mFetchedBody )
      {
        if( !fetchBody() )
        {
          kWarning() << "Fetching message body failed!";
          return QVariant();
        }
      }
      // Hum.. not sure about this..
      return QString::fromAscii( "%1\r\n%2" ).arg( QString::fromAscii( mMessage->head() ) ).arg( QString::fromAscii( mMessage->body() ) );
    break;
    default:
      // not my data member: fall through
    break;
  }

  return QVariant();
}

bool DataRfc822::fetchHeader()
{
  kDebug() << "Fetching message header...";

  Akonadi::ItemFetchJob * job = new Akonadi::ItemFetchJob( mItem );

  //job->setCollection( mCollection );

  // FIXME: this doesn't work with IMAP for now :/
  //job->fetchScope().fetchPayloadPart( QByteArray( Akonadi::MessagePart::Header ) ); 
  job->fetchScope().fetchFullPayload();

  if( !job->exec() )
  {
    kWarning() << "Fetching the message header via" << Akonadi::MessagePart::Header << "failed with error '" << job->errorString() << "'";
    return false;
  }

  Q_ASSERT( job->items().count() == 1 );

  mItem = job->items().first();

  kDebug() << "Fetched item has id" << mItem.id() << "and mimetype" << mItem.mimeType();

  kDebug() << "Raw payload is" << mItem.payloadData();

  if( !mItem.hasPayload< MessagePtr >() )
  {
    kWarning() << "Fetching the message header via" << Akonadi::MessagePart::Header << "failed: the fetched item has no MessagePtr payload!";
    return false;
  }

  mMessage = mItem.payload< MessagePtr >();

  kDebug() << "Message header fetched: head is '" << mMessage->head() << "'";

  return mMessage;
}

bool DataRfc822::fetchBody()
{
  Q_ASSERT( !mFetchedBody );

  kDebug() << "Fetching message body...";

  Akonadi::ItemFetchJob * job = new Akonadi::ItemFetchJob( mItem );

  //job->setCollection( mCollection );

  job->fetchScope().fetchFullPayload();

  if( !job->exec() )
    return false;

  Q_ASSERT( job->items().count() == 1 );

  mItem = job->items().first();

  kDebug() << "Fetched item has id" << mItem.id() << "and mimetype" << mItem.mimeType();

  kDebug() << "Raw payload is" << mItem.payloadData();

  if( !mItem.hasPayload< MessagePtr >() )
    return false;

  mMessage = mItem.payload< MessagePtr >();

  kDebug() << "Message body fetched: head is '" << mMessage->head() << "'";

  return mMessage;
}
