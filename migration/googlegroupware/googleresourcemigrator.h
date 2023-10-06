/*
    SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/AgentManager>
#include <migratorbase.h>

#include <Akonadi/AgentInstance>

#include <QMap>

class GoogleResourceMigrator : public MigratorBase
{
    Q_OBJECT
public:
    explicit GoogleResourceMigrator();

    [[nodiscard]] QString displayName() const override;
    [[nodiscard]] QString description() const override;

    [[nodiscard]] bool shouldAutostart() const override;

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
            : calendar(calendar)
            , contacts(contacts)
        {
        }

        T calendar{};
        T contacts{};
    };

    [[nodiscard]] bool migrateAccount(const QString &account, const Instances &oldInstances, const Akonadi::AgentInstance &newInstance);
    void removeLegacyInstances(const QString &account, const Instances &instances);
    [[nodiscard]] QString mergeAccountNames(const ResourceValues<QString> &accountName, const Instances &oldInstances) const;
    [[nodiscard]] int mergeAccountIds(ResourceValues<int> accountId, const Instances &oldInstances) const;

    QMap<QString, Instances> mMigrations;
    int mMigrationCount = 0;
    int mMigrationsDone = 0;
};
