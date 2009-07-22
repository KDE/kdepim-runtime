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

#ifndef COUCHDBQTCHANGENOTIFIER_H
#define COUCHDBQTCHANGENOTIFIER_H

#include <QObject>

class QVariant;
class QString;
class QHttpResponseHeader;

class CouchDBQtChangeNotifier : public QObject
{
  Q_OBJECT
  public:
    CouchDBQtChangeNotifier( const QString& db );
  signals:
    void notification( const QString& db, const QVariant& v );
  private slots:
    void slotNotification( const QHttpResponseHeader & );
    void slotConnect( int, bool );
  private:
    class Private;
    Private * const d;
};

#endif // COUCHDBQTCHANGENOTIFIER_H
