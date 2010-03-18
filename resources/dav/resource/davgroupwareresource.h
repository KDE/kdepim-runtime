/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef DAVGROUPWARERESOURCE_H
#define DAVGROUPWARERESOURCE_H

#include "etagcache.h"

#include <akonadi/resourcebase.h>

namespace Akonadi {
class IncidenceMimeTypeVisitor;
}

class DavGroupwareResource : public Akonadi::ResourceBase,
                            public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    DavGroupwareResource( const QString &id );
    ~DavGroupwareResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &collection );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

  private Q_SLOTS:
    void onRetrieveCollectionsFinished( KJob* );
    void onRetrieveItemsFinished( KJob* );
    void onMultigetFinished( KJob* );
    void onRetrieveItemFinished( KJob* );

    void onItemAddedFinished( KJob* );
    void onItemChangedFinished( KJob* );
    void onItemRemovedFinished( KJob* );

    void onCollectionDiscovered( const QString &collectionUrl, const QString &configuredUrl );

  private:
    bool configurationIsValid();

    Akonadi::IncidenceMimeTypeVisitor *mMimeVisitor;
    Akonadi::Collection mDavCollectionRoot;
    EtagCache mEtagCache;
};

#endif
