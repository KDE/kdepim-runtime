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
namespace KGAPI2 {
class Job;
}
class KJob;

class ContactsResource : public GoogleResource
                       , public Akonadi::AgentBase::ObserverV4
{
    Q_OBJECT

public:
    using Akonadi::AgentBase::ObserverV4::collectionChanged; // So we don't trigger -Woverloaded-virtual
    explicit ContactsResource(const QString &id);

    ~ContactsResource();

protected:
    using ResourceBase::retrieveItems; // Suppress -Woverload-virtual

protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;
    virtual void retrieveContactsPhotos(const QVariant &argument);
    void retrieveTags() override;

    void itemsRemoved(const Akonadi::Item::List &item) override;
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers) override;

    void tagAdded(const Akonadi::Tag &tag) override;
    void tagRemoved(const Akonadi::Tag &tag) override;
    void itemsTagsChanged(const Akonadi::Item::List &items,
                          const QSet<Akonadi::Tag> &addedTags, const QSet<Akonadi::Tag> &removedTags) override;


    void slotItemsRetrieved(KGAPI2::Job *job);

    void slotUpdatePhotosItemsRetrieved(KJob *job);
    void slotUpdatePhotoFinished(KGAPI2::Job *job, const KGAPI2::ContactPtr &contact);

    GoogleSettings *settings() const override;
    int runConfigurationDialog(WId windowId) override;
    void updateResourceName() override;
    QList< QUrl > scopes() const override;
};

#endif // CONTACTSRESOURCE_H
