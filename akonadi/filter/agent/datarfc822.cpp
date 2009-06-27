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

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/kmime/messageparts.h>

#include <akonadi/filter/datamemberdescriptor.h>

DataRfc822::DataRfc822( const Akonadi::Item &item, const Akonadi::Collection &collection )
  : Data(), mItem( item ), mCollection( collection ), mFetchedBody( false )
{
}

DataRfc822::~DataRfc822()
{
}

QVariant DataRfc822::getPropertyValue( const Akonadi::Filter::FunctionDescriptor * function, const Akonadi::Filter::DataMemberDescriptor * dataMember )
{
  return Data::getPropertyValue( function, dataMember );
}

QVariant DataRfc822::getDataMemberValue( const Akonadi::Filter::DataMemberDescriptor * dataMember )
{
  if( !mMessage )
  {
     if( !fetchHeader() )
       return QVariant();
  }

  switch( dataMember->id() )
  {
    case Akonadi::Filter::StandardDataMemberFromHeader:
      return mMessage->from()->asUnicodeString();
    break;
    case Akonadi::Filter::StandardDataMemberSubjectHeader:
      return mMessage->subject()->asUnicodeString();
    break;
    case Akonadi::Filter::StandardDataMemberToHeader:
      return mMessage->to()->asUnicodeString();
    break;
    case Akonadi::Filter::StandardDataMemberCcHeader:
      return mMessage->cc()->asUnicodeString();
    break;
    case Akonadi::Filter::StandardDataMemberBccHeader:
      return mMessage->bcc()->asUnicodeString();
    break;
    default:
    break;
  }

  return QVariant();
}

bool DataRfc822::fetchHeader()
{
  Akonadi::ItemFetchJob job( mItem );
  job.setCollection( mCollection );
  job.fetchScope().fetchPayloadPart( QByteArray( Akonadi::MessagePart::Header ) );
  if( !job.exec() )
    return false;
  Q_ASSERT( job.items().count() == 1 );
  mItem = job.items().first();
  if( !mItem.hasPayload< MessagePtr >() )
    return false;
  mMessage = mItem.payload< MessagePtr >();
  return mMessage;
}

