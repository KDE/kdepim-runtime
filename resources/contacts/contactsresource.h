/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/ResourceBase>

#include <kcontacts/addressee.h>
#include <kcontacts/contactgroup.h>
#include <kcontacts/contactgrouptool.h>
#include <kcontacts/vcardconverter.h>

class QDir;

class ContactsResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2
{
    Q_OBJECT

public:
    explicit ContactsResource(const QString &id);
    ~ContactsResource() override;

public Q_SLOTS:
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
    void collectionMoved(const Akonadi::Collection &collection,
                         const Akonadi::Collection &collectionSource,
                         const Akonadi::Collection &collectionDestination) override;

private:
    void slotReloadConfig();
    Akonadi::Collection::List createCollectionsForDirectory(const QDir &parentDirectory, const Akonadi::Collection &parentCollection) const;
    QString baseDirectoryPath() const;
    void initializeDirectory(const QString &path) const;
    Akonadi::Collection::Rights supportedRights(bool isResourceCollection) const;
    QString directoryForCollection(const Akonadi::Collection &collection) const;

private:
    QStringList mSupportedMimeTypes;
};

