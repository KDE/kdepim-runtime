/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

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

#include "imaplibresource.h"

#include <QtCore/QDebug>
#include <QtDBus/QDBusConnection>

#include <kdebug.h>
#include <klocale.h>

#include <libakonadi/collectionlistjob.h>
#include <libakonadi/collectionmodifyjob.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>

#include <kmime/kmime_message.h>

using namespace Akonadi;

ImaplibResource::ImaplibResource( const QString &id )
   :ResourceBase( id )
{
  kDebug(50002) << "Implement me!";
}

ImaplibResource::~ImaplibResource()
{
  kDebug(50002) << "Implement me!";
}

bool ImaplibResource::retrieveItem( const Akonadi::Item &item, const QStringList &parts )
{
  kDebug(50002) << "Implement me!";
  return false;

  /*
  KMime::Message *mail = new KMime::Message();
  mail->setContent( data );
  mail->parse();

  Item i( item );
  i.setMimeType( "message/rfc822" );
  i.setPayload( MessagePtr( mail ) );
  itemRetrieved( i );
  return true;
  */
}

void ImaplibResource::configure()
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& collection )
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::itemChanged( const Akonadi::Item& item, const QStringList& parts )
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::itemRemoved(const Akonadi::DataReference & ref)
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::retrieveCollections()
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::retrieveItems(const Akonadi::Collection & col, const QStringList &parts)
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::collectionAdded(const Collection & collection, const Collection &parent)
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::collectionChanged(const Collection & collection)
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::collectionRemoved(int id, const QString & remoteId)
{
  qDebug() << "Implement me!";
}

#include "imaplibresource.moc"
