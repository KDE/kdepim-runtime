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

#ifndef COUCHDBQT_H
#define COUCHDBQT_H

#include <QObject>

class CouchDBDocumentInfo
{
public:
    CouchDBDocumentInfo();
    CouchDBDocumentInfo( const CouchDBDocumentInfo& );
    CouchDBDocumentInfo operator=( CouchDBDocumentInfo );
    QString id() const;
    void setId( const QString& );
    QString database() const;
    void setDatabase( const QString& );
private:
    void swap( CouchDBDocumentInfo& );
    class Private;
    Private * const d;
};

typedef QList<CouchDBDocumentInfo> CouchDBDocumentInfoList;

class CouchDBQt : public QObject
{
  Q_OBJECT
public:
    CouchDBQt();
    virtual ~CouchDBQt();

public slots:
    void requestDatabaseListing();
    void requestDatabaseCreation( const QString& db );
    void requestDatabaseDeletion( const QString& db );
    void requestDocumentListing( const QString& db );
    void requestDocument( const CouchDBDocumentInfo& info );

signals:
    void databasesListed( const QStringList& );
    void documentsListed( const CouchDBDocumentInfoList& );
    void documentRetrieved( const QVariant & v );

private slots:
    void init();
    void slotDatabaseListingFinished(int, bool);
    void slotDocumentListingFinished(int, bool);
    void slotDocumentRetrievalFinished(int, bool);
private:
    class Private;
    Private * const d;
};

#endif // COUCHDBQT_H
