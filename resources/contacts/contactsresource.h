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

namespace Akonadi_Contacts_Resource
{
class ContactsResourceSettings;
}
class QDir;

class ContactsResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2
{
    Q_OBJECT

public:
    ContactsResource(const QString &id);
    ~ContactsResource();

public Q_SLOTS:
    void configure(WId windowId) Q_DECL_OVERRIDE;
    void aboutToQuit() Q_DECL_OVERRIDE;

protected Q_SLOTS:
    void retrieveCollections() Q_DECL_OVERRIDE;
    void retrieveItems(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;

protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    void itemRemoved(const Akonadi::Item &item) Q_DECL_OVERRIDE;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) Q_DECL_OVERRIDE;
    void collectionChanged(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    // do not hide the other variant, use implementation from base class
    // which just forwards to the one above
    using Akonadi::AgentBase::ObserverV2::collectionChanged;
    void collectionRemoved(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;

    virtual void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource,
                           const Akonadi::Collection &collectionDestination) Q_DECL_OVERRIDE;
    virtual void collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &collectionSource,
                                 const Akonadi::Collection &collectionDestination) Q_DECL_OVERRIDE;

private:
    Akonadi::Collection::List createCollectionsForDirectory(const QDir &parentDirectory,
            const Akonadi::Collection &parentCollection) const;
    QString baseDirectoryPath() const;
    void initializeDirectory(const QString &path) const;
    Akonadi::Collection::Rights supportedRights(bool isResourceCollection) const;
    QString directoryForCollection(const Akonadi::Collection &collection) const;

private:
    QStringList mSupportedMimeTypes;
    Akonadi_Contacts_Resource::ContactsResourceSettings *mSettings;
};

#endif
