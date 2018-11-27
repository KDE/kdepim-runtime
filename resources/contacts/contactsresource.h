/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef CONTACTSRESOURCE_H
#define CONTACTSRESOURCE_H

#include <resourcebase.h>

#include <kcontacts/addressee.h>
#include <kcontacts/contactgroup.h>
#include <kcontacts/contactgrouptool.h>
#include <kcontacts/vcardconverter.h>

namespace Akonadi_Contacts_Resource {
class ContactsResourceSettings;
}
class QDir;

class ContactsResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2
{
    Q_OBJECT

public:
    explicit ContactsResource(const QString &id);
    ~ContactsResource() override;

public Q_SLOTS:
    void configure(WId windowId) override;
    void aboutToQuit() override;

protected:
    using ResourceBase::retrieveItems; // suppress -Woverload-virtual warnings

protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;

protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &item) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    // do not hide the other variant, use implementation from base class
    // which just forwards to the one above
    using Akonadi::AgentBase::ObserverV2::collectionChanged;
    void collectionRemoved(const Akonadi::Collection &collection) override;

    void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) override;
    void collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) override;

private:
    Akonadi::Collection::List createCollectionsForDirectory(const QDir &parentDirectory, const Akonadi::Collection &parentCollection) const;
    QString baseDirectoryPath() const;
    void initializeDirectory(const QString &path) const;
    Akonadi::Collection::Rights supportedRights(bool isResourceCollection) const;
    QString directoryForCollection(const Akonadi::Collection &collection) const;

private:
    QStringList mSupportedMimeTypes;
    Akonadi_Contacts_Resource::ContactsResourceSettings *mSettings = nullptr;
};

#endif
