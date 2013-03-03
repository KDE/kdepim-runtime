/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GOOGLE_CONTACTS_CONTACTSRESOURCE_H
#define GOOGLE_CONTACTS_CONTACTSRESOURCE_H

#include "common/googleresource.h"

#include <Akonadi/Collection>
#include <Akonadi/Item>

class GoogleSettings;
namespace KGAPI2
{
class Job;
}
class KJob;

class ContactsResource: public GoogleResource
{
    Q_OBJECT

  public:
    explicit ContactsResource( const QString &id );

    ~ContactsResource();

  protected Q_SLOTS:
    virtual void retrieveCollections();

    virtual void retrieveItems( const Akonadi::Collection &collection );
    virtual void retrieveContactsPhotos( const QVariant &argument );

    virtual void itemRemoved( const Akonadi::Item &item );
    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers );
    virtual void itemMoved( const Akonadi::Item &item, const Akonadi::Collection &collectionSource,
                            const Akonadi::Collection &collectionDestination );

    virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    virtual void collectionChanged( const Akonadi::Collection &collection );
    virtual void collectionRemoved( const Akonadi::Collection &collection );

    virtual void itemLinked( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemUnlinked( const Akonadi::Item &item, const Akonadi::Collection &collection );

    void slotItemsRetrieved( KGAPI2::Job *job );
    void slotCollectionsRetrieved( KGAPI2::Job *job );

    void slotUpdatePhotosItemsRetrieved( KJob *job );
    void slotUpdatePhotoFinished( KGAPI2::Job *job, const KGAPI2::ContactPtr &contact );

    void slotCreateJobFinished( KGAPI2::Job *job );

    virtual GoogleSettings *settings() const;
    virtual int runConfigurationDialog( WId windowId );
    virtual void updateResourceName();
    virtual QList< QUrl > scopes() const;

  private:

    QMap<QString, Akonadi::Collection> m_collections;
    Akonadi::Collection m_rootCollection;

};

#endif // CONTACTSRESOURCE_H
