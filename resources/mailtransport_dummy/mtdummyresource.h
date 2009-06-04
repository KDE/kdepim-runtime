/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as published
    by the Free Software Foundation; either version 2 of the License or
    ( at your option ) version 3 or, at the discretion of KDE e.V.
    ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef MTDUMMYRESOURCE_H
#define MTDUMMYRESOURCE_H

//#include <Akonadi/TransportResource>
// FIXME why doens't it work CamelCase?
#include <akonadi/transportresource.h>

class MTDummyResource : public Akonadi::TransportResource
{
  Q_OBJECT

  public:
    MTDummyResource( const QString &id );
    ~MTDummyResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

    virtual void send( Akonadi::Item::Id message );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    virtual void aboutToQuit();

};

#endif
