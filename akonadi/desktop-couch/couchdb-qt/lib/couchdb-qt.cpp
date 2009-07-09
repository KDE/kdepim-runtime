/*
    Copyright (c) 2009 Canonical

    Author: Till Adam <till@kdab.net>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2.1 or version 3 of the license,
    or later versions approved by the membership of KDE e.V.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "couchdb-qt.h"

#include "json_driver.hh"

#include <QtNetwork/QHttp>
#include <QtCore/QUrl>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtCore/QVariant>

class CouchDBDocumentInfo::Private
{
public:
  Private()
  {
  }

  QString dbname;
  QString docid;
};

CouchDBDocumentInfo::CouchDBDocumentInfo()
:d( new Private )
{
}

CouchDBDocumentInfo::CouchDBDocumentInfo( const CouchDBDocumentInfo& other )
:d( new Private( *other.d ) )
{
}

CouchDBDocumentInfo CouchDBDocumentInfo::operator= ( CouchDBDocumentInfo other )
{
  if( this != &other )
    other.swap( *this );
  return *this;
}


void CouchDBDocumentInfo::swap( CouchDBDocumentInfo& other )
{
  *other.d = *d;
}

void CouchDBDocumentInfo::setId( const QString& id )
{
  d->docid = id;
}

QString CouchDBDocumentInfo::id() const
{
  return d->docid;
}

void CouchDBDocumentInfo::setDatabase( const QString& db )
{
  d->dbname = db;
}

QString CouchDBDocumentInfo::database() const
{
  return d->dbname;
}


class CouchDBQt::Private
{
  public:
    Private()
    :requestId(-1)
    {
    }
    QVariant parseJSONString( const QString& json );
    CouchDBDocumentInfoList variantMapToDocInfoList( const QVariant& map );
    QString serializeToJSONString( const QVariant& v);

    int requestId;
    QString database;
    QHttp http;
    JSonDriver driver;
};

QVariant CouchDBQt::Private::parseJSONString( const QString& json )
{

  bool ok;
  QVariant data = driver.parse ( json, &ok );
  if ( !ok ) {
    qCritical("%i - Error: %s", driver.errorLine(), driver.error().toLatin1().data());
    exit (1);
  }
  else {
    qDebug() << "json object successfully converted to:";
    //qDebug() << data;
  }
  return data;
}

QString CouchDBQt::Private::serializeToJSONString( const QVariant& v )
{
    return driver.serialize( v );
}

CouchDBDocumentInfoList CouchDBQt::Private::variantMapToDocInfoList( const QVariant& vmap )
{
  CouchDBDocumentInfoList list;
  QVariantMap map = vmap.toMap();
  bool ok = false;
  const int rows = map["total_rows"].toInt( &ok );
  if ( !ok )
    return list;
  const int offset = map["offset"].toInt( &ok );
  if ( !ok )
    return list;
  qDebug() << rows << offset;
  QVariantList rowlist = map["rows"].toList();
  if ( rowlist.count() != rows ) {
    qWarning() << "Inconsistent database: " << database;
    return list;
  }

  Q_FOREACH( QVariant v, rowlist ) {
    QVariantMap m = v.toMap();
    const QString id = m["id"].toString();
    if ( !id.isEmpty() ) {
      CouchDBDocumentInfo info;
      info.setDatabase( database );
      info.setId( id );
      list << info;
    }
  }
  return list;
}

CouchDBQt::CouchDBQt()
    :d( new Private )
{
  QMetaObject::invokeMethod( this, "init", Qt::QueuedConnection );
}

CouchDBQt::~CouchDBQt()
{
}

void CouchDBQt::init()
{
  d->requestId = d->http.setHost("localhost", 5984);
}

void CouchDBQt::requestDatabaseListing()
{
  d->http.disconnect( SIGNAL( requestFinished(int,bool) ) );
  connect( &d->http, SIGNAL( requestFinished(int,bool) ),
           this, SLOT( slotDatabaseListingFinished(int, bool) ) );
  d->requestId = d->http.get( QLatin1String("/_all_dbs") );
}

void CouchDBQt::slotDatabaseListingFinished(int request, bool error)
{
  if (error) {
    qWarning() << d->http.errorString();
    return;
  }
  if ( request != d->requestId ) {
    return;
  }
  // FIXME DOS?
  const QString dbstring = d->http.readAll();
  const QVariant dbs = d->parseJSONString( dbstring );
  QStringList dbstringlist;
  Q_FOREACH( QVariant v, dbs.toList() ) {
    dbstringlist << v.toString();
  }
  emit databasesListed( dbstringlist );
}

void CouchDBQt::requestDatabaseCreation( const QString& db )
{
  // FIXME
}

void CouchDBQt::requestDatabaseDeletion( const QString& db )
{
  // FIXME
}

void CouchDBQt::requestDocumentListing( const QString& db )
{
  d->http.disconnect( SIGNAL( requestFinished(int,bool) ) );
  connect( &d->http, SIGNAL( requestFinished(int,bool) ),
           this, SLOT( slotDocumentListingFinished(int, bool) ) );
  d->requestId = d->http.get( QString("/%1/_all_docs").arg( db ) );
  d->database = db;
}

void CouchDBQt::slotDocumentListingFinished(int request, bool error)
{
  if (error) {
    qWarning() << d->http.errorString();
    return;
  }
  if ( request != d->requestId ) {
    return;
  }
  // FIXME DOS?
  const QString docs = d->http.readAll();
  //qDebug() << docs;
  const QVariant docsAsVariant = d->parseJSONString( docs );
  const CouchDBDocumentInfoList docList = d->variantMapToDocInfoList( docsAsVariant );
  emit documentsListed( docList );
}

void CouchDBQt::requestDocument( const CouchDBDocumentInfo & info )
{
  if ( info.id().isEmpty() )
    return;

  d->http.disconnect( SIGNAL( requestFinished(int,bool) ) );
  connect( &d->http, SIGNAL( requestFinished(int,bool) ),
           this, SLOT( slotDocumentRetrievalFinished(int, bool) ) );
  d->requestId = d->http.get( QString("/%1/%2").arg( info.database() )
                                               .arg( info.id() ) );
}

void CouchDBQt::slotDocumentRetrievalFinished(int request, bool error)
{
  if (error) {
    qWarning() << d->http.errorString();
    return;
  }
  if ( request != d->requestId ) {
    return;
  }
  // FIXME DOS?
  const QString doc = d->http.readAll();
  //qDebug() << doc;
  const QVariant docAsVariant = d->parseJSONString( doc );
  emit documentRetrieved( docAsVariant );
}


void CouchDBQt::updateDocument( const CouchDBDocumentInfo& info, const QVariant& v )
{
  const QString str = d->serializeToJSONString( v );
  d->http.disconnect( SIGNAL( requestFinished(int,bool) ) );
  connect( &d->http, SIGNAL( requestFinished(int,bool) ),
           this, SLOT( slotDocumentUpdatedFinished(int, bool) ) );

  d->requestId = d->http.post( QString("/%1/%2").arg( info.database() ).arg( info.id() ),
                               str.toUtf8() );
}

void CouchDBQt::slotDocumentUpdateFinished(int request, bool error)
{
  if ( error ) {
    emit documentUpdated( false, d->http.errorString() );
    return;
  }
  if ( request != d->requestId ) {
    emit documentUpdated( false, tr("Invalid request id") );
    return;
  }
  emit documentUpdated( true );
}

