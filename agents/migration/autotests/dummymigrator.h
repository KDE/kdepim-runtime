/*
 * Copyright 2013  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef DUMMYMIGRATOR_H
#define DUMMYMIGRATOR_H

#include <migration/migratorbase.h>

/**
 * Dummy migrator that simply completes after 10s and always autostarts.
 * Add to the scheduler to play with the migrationagent.
 */
class DummyMigrator : public MigratorBase
{
    Q_OBJECT
public:
    explicit DummyMigrator(const QString &identifier);

    QString displayName() const override;
    void startWork() override;

    bool shouldAutostart() const override;
    bool canStart() override;
    void pause() override;

    void abort() override;
private Q_SLOTS:
    void onTimerElapsed();
};

#endif
