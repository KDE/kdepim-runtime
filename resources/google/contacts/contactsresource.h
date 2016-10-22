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

#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>

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
    using GoogleResource::collectionChanged; // So we don't trigger -Woverloaded-virtual
    explicit ContactsResource(const QString &id);

    ~ContactsResource();

protected:
    using ResourceBase::retrieveItems; // Suppress -Woverload-virtual

protected Q_SLOTS:
    void retrieveCollections() Q_DECL_OVERRIDE;
    void retrieveItems(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    virtual void retrieveContactsPhotos(const QVariant &argument);

    void itemRemoved(const Akonadi::Item &item) Q_DECL_OVERRIDE;
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers) Q_DECL_OVERRIDE;
    void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource,
                   const Akonadi::Collection &collectionDestination) Q_DECL_OVERRIDE;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) Q_DECL_OVERRIDE;
    void collectionChanged(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void collectionRemoved(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;

    void itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;

    void slotItemsRetrieved(KGAPI2::Job *job);
    void slotCollectionsRetrieved(KGAPI2::Job *job);

    void slotUpdatePhotosItemsRetrieved(KJob *job);
    void slotUpdatePhotoFinished(KGAPI2::Job *job, const KGAPI2::ContactPtr &contact);

    void slotCreateJobFinished(KGAPI2::Job *job);

    GoogleSettings *settings() const Q_DECL_OVERRIDE;
    int runConfigurationDialog(WId windowId) Q_DECL_OVERRIDE;
    void updateResourceName() Q_DECL_OVERRIDE;
    QList< QUrl > scopes() const Q_DECL_OVERRIDE;

private:

    QMap<QString, Akonadi::Collection> m_collections;
    Akonadi::Collection m_rootCollection;

};

#endif // CONTACTSRESOURCE_H
