/*
    Copyright (c) 2020 Daniel Vrátil <dvratil@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef GOOGLERESOURCEMIGRATOR_H
#define GOOGLERESOURCEMIGRATOR_H

#include <agentmanager.h>
#include <migratorbase.h>

#include <AkonadiCore/AgentInstance>

#include <QMap>

class GoogleResourceMigrator : public MigratorBase
{
    Q_OBJECT
public:
    explicit GoogleResourceMigrator();

    QString displayName() const override;
    QString description() const override;

    bool shouldAutostart() const override;

protected:
    void startWork() override;
    void migrateNextAccount();

private:
    struct Instances {
        Akonadi::AgentInstance calendarResource;
        Akonadi::AgentInstance contactResource;
        bool alreadyExists = false;
    };

    template<typename T>
    struct ResourceValues {
        explicit ResourceValues() = default;
        template<typename U, typename V>
        ResourceValues(U &&calendar, V &&contacts)
            : calendar(calendar), contacts(contacts)
        {}

        T calendar{};
        T contacts{};
    };

    bool migrateAccount(const QString &account, const Instances &oldInstances, const Akonadi::AgentInstance &newInstance);
    void removeLegacyInstances(const QString &account, const Instances &instances);
    QString mergeAccountNames(const ResourceValues<QString> &accountName, const Instances &oldInstances) const;
    int mergeAccountIds(const ResourceValues<int> &accountId, const Instances &oldInstances) const;

    QMap<QString, Instances> mMigrations;
    int mMigrationCount = 0;
    int mMigrationsDone = 0;
};

#endif
