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

#ifndef MIGRATIONSCHEDULER_H
#define MIGRATIONSCHEDULER_H

#include "migratorbase.h"
#include <QObject>
#include <QAbstractItemModel>
#include <QSharedPointer>
#include <QPointer>
#include <QStandardItemModel>

class MigrationExecutor;
class KJobTrackerInterface;
class MigratorModel;

class LogModel : public QStandardItemModel
{
    Q_OBJECT
public Q_SLOTS:
    void message(MigratorBase::MessageType type, const QString &msg);
};

class Row : public QObject
{
    Q_OBJECT
public:
    QSharedPointer<MigratorBase> mMigrator;
    MigratorModel &mModel;

    explicit Row(const QSharedPointer<MigratorBase> &migrator, MigratorModel &model);

    bool operator==(const Row &other) const;

private Q_SLOTS:
    void stateChanged(MigratorBase::MigrationState);
    void progress(int);
};

/**
 * The model serves as container for the migrators and exposes the status of each migrator.
 *
 * It can be plugged into a Listview to inform about the migration progress.
 */
class MigratorModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Roles {
        IdentifierRole = Qt::UserRole + 1,
        LogfileRole
    };
    bool addMigrator(const QSharedPointer<MigratorBase> &migrator);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QSharedPointer<MigratorBase> migrator(const QString &identifier) const;
    QList< QSharedPointer<MigratorBase> > migrators() const;

private:
    enum Columns {
        Name = 0,
        Progress = 1,
        State = 2,
        ColumnCount
    };
    friend class Row;
    int positionOf(const Row &);
    void columnChanged(const Row &, int column);
    QList< QSharedPointer<Row> > mMigrators;
};

/**
 * Scheduler for migration jobs.
 *
 * Status information is exposed via getModel, which returns a list model containing all migrators with basic information.
 * Additionally a logmodel is available via getLogModel for each migrator. The logmodel is continuously filled with information, and can be requested and displayed at any time.
 *
 * Migrators which return true on shouldAutostart() automatically enter a queue to be processed one after the other.
 * When manually triggered it is possible though to run multiple jobs in parallel.
 */
class MigrationScheduler : public QObject
{
    Q_OBJECT
public:
    explicit MigrationScheduler(KJobTrackerInterface *jobTracker = nullptr, QObject *parent = nullptr);
    ~MigrationScheduler() override;

    void addMigrator(const QSharedPointer<MigratorBase> &migrator);

    //A model for the view
    QAbstractItemModel &model();
    QStandardItemModel &logModel(const QString &identifier);

    //Control
    void start(const QString &identifier);
    void pause(const QString &identifier);
    void abort(const QString &identifier);

private:
    void checkForAutostart(const QSharedPointer<MigratorBase> &migrator);

    QScopedPointer<MigratorModel> mModel;
    QHash<QString, QSharedPointer<LogModel> > mLogModel;
    QPointer<MigrationExecutor> mAutostartExecutor;
    KJobTrackerInterface *mJobTracker = nullptr;
};

#endif // MIGRATIONSCHEDULER_H
