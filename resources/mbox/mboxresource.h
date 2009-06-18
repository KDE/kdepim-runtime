/*
    Copyright (c) 2009 Bertjan Broeksem <b.broeksema@kdemail.net>

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

#ifndef MBOX_RESOURCE_H
#define MBOX_RESOURCE_H

#include "settings.h"
#include "singlefileresource.h"

class MBox;

class MboxResource : public Akonadi::SingleFileResource<Settings>
{
  Q_OBJECT

  public:
    MboxResource( const QString &id );
    ~MboxResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    virtual void retrieveItems( const Akonadi::Collection &col );
    virtual bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

    // From SingleFileResourceBase
    virtual bool readFromFile( const QString &fileName );
    virtual bool writeToFile( const QString &fileName );

  private Q_SLOTS:
    void onCollectionFetch( KJob *job );
    void onCollectionModify( KJob *job );

  private:
    QMap<KJob*, Akonadi::Item> mCurrentItemDeletions;
    MBox *mMBox;
};

#endif
