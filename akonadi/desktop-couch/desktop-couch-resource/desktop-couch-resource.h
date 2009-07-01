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

#ifndef DESKTOPCOUCHRESOURCE_H
#define DESKTOPCOUCHRESOURCE_H

#include <akonadi/resourcebase.h>
#include <kabc/addressee.h>
#include <couchdb-qt.h>

class DesktopCouchResource : public Akonadi::ResourceBase
{
  Q_OBJECT

  public:
    DesktopCouchResource( const QString &id );
    ~DesktopCouchResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );
    virtual void retrieveCollections();

  protected Q_SLOTS:
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

  private slots:
    void slotDocumentsListed( const CouchDBDocumentInfoList& );
    void slotDocumentRetrieved( const QVariant& v );
  private:
    CouchDBQt db;
};

#endif
