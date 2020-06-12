/*
 * Copyright (C) 2020 by Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ETESYNCRESOURCE_H
#define ETESYNCRESOURCE_H

#include <etesync/etesync.h>
#include <kcontacts/addressee.h>
#include <kcontacts/vcardconverter.h>

#include <AkonadiAgentBase/ResourceBase>

class etesyncResource : public Akonadi::ResourceBase,
                        public Akonadi::AgentBase::Observer
{
    Q_OBJECT

public:
    explicit etesyncResource(const QString &id);
    ~etesyncResource() override;

Q_SIGNALS:

    void initialiseDone(bool successful);

protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &col) override;
    bool retrieveItem(const Akonadi::Item &item,
                      const QSet<QByteArray> &parts) override;

protected:
    void aboutToQuit() override;

    void itemAdded(const Akonadi::Item &item,
                   const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item,
                     const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &item) override;

    void initialise();

    int setupCollection(Akonadi::Collection &collection, EteSyncJournal *journal);

private Q_SLOTS:
    void onReloadConfiguration();

private:
    EteSync *etesync;
    QString derived;
    EteSyncJournalManager *journalManager;
    EteSyncAsymmetricKeyPair *keypair;
    QString username, password, serverUrl, encryptionPassword;
};

#endif
