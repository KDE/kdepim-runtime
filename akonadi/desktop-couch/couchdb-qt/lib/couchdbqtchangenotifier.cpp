/*
    Copyright (c) 2009 Canonical

    Author: Till Adam <till@kdab.com>

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

#include "couchdbqtchangenotifier.h"

#include <QHttp>
#include <QDebug>
#include <QVariant>

class CouchDBQtChangeNotifier::Private
{
public:
  Private( CouchDBQtChangeNotifier * _q )
  :q( _q )
  {
  }
  CouchDBQtChangeNotifier *q;
  QString db;
  QHttp http;
};

CouchDBQtChangeNotifier::CouchDBQtChangeNotifier( const QString& db )
    :d( new Private( this ) )
{
  d->db = db;
  d->http.setHost( "localhost", 5984 );
  connect( &d->http, SIGNAL( requestFinished(int,bool) ),
           this, SLOT( slotConnect(int, bool) ) );
  connect( &d->http, SIGNAL( readyRead ( const QHttpResponseHeader & ) ),
           this, SLOT( slotNotification( const QHttpResponseHeader & ) ) );
}

void CouchDBQtChangeNotifier::slotConnect( int, bool ok )
{
  // this slot is connected to requestFinished(), which means it's called in
  // two cases: 1) after the initial connect (setHost) succeeds and 2) after
  // the get timed out. In both cases we want to (re-)trigger the get.
  if ( ok )
    d->http.get( QString("/%1/_changes?continuous=true").arg( d->db ) );
  else
    qWarning() << "Failed to connect to the updates trigger for db: " << d->db;
}

void CouchDBQtChangeNotifier::slotNotification( const QHttpResponseHeader &  )
{
  qWarning() << "SLOT NOTIFICATION" << d->http.readAll();
  emit notification( d->db, QVariant() );
}
