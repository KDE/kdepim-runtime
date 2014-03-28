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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "migrationscheduler.h"

#include <KLocalizedString>
#include <KDebug>
#include <KIcon>
#include <KJobTrackerInterface>

#include "migrationexecutor.h"

void LogModel::message(MigratorBase::MessageType type, const QString &msg)
{
    switch ( type ) {
    case MigratorBase::Success: {
        QStandardItem *item = new QStandardItem(KIcon(QLatin1String("dialog-ok-apply")), msg);
        item->setEditable(false);
        appendRow(item);
        break;
    }
    case MigratorBase::Skip: {
        QStandardItem *item = new QStandardItem(KIcon(QLatin1String("dialog-ok")), msg);
        item->setEditable(false);
        appendRow(item);
        break;
    }
    case MigratorBase::Info: {
        QStandardItem *item = new QStandardItem(KIcon(QLatin1String("dialog-information")), msg);
        item->setEditable(false);
        appendRow(item);
        break;
    }
    case MigratorBase::Warning: {
        QStandardItem *item = new QStandardItem(KIcon(QLatin1String("dialog-warning")), msg);
        item->setEditable(false);
        appendRow(item);
        break;
    }
    case MigratorBase::Error: {
        QStandardItem *item = new QStandardItem(KIcon(QLatin1String("dialog-error")), msg);
        item->setEditable(false);
        appendRow(item);
        break;
    }
    default:
        kError() << "unknown type " << type;
    }
}


Row::Row(const QSharedPointer<MigratorBase> &migrator, MigratorModel &model)
:   QObject(),
    mMigrator(migrator),
    mModel(model)
{
    connect(migrator.data(), SIGNAL(stateChanged(MigratorBase::MigrationState)), this, SLOT(stateChanged(MigratorBase::MigrationState)));
    connect(migrator.data(), SIGNAL(progress(int)), this, SLOT(progress(int)));
}

bool Row::operator==(const Row &other) const
{
    return mMigrator->identifier() == other.mMigrator->identifier();
}

void Row::stateChanged(MigratorBase::MigrationState /*newState*/)
{
    mModel.columnChanged(*this, MigratorModel::State);
}

void Row::progress(int /*prog*/)
{
    mModel.columnChanged(*this, MigratorModel::Progress);
}

int MigratorModel::positionOf(const Row &row)
{
    int pos = 0;
    foreach (const QSharedPointer<Row> &r, mMigrators) {
        if (row == *r) {
            return pos;
        }
        pos++;
    }
    return -1;
}

void MigratorModel::columnChanged(const Row &row, int col)
{
    const int p = positionOf(row);
    Q_ASSERT(p >= 0);
    if (p >= 0) {
        const QModelIndex idx = index(p, col);
        emit dataChanged(idx, idx);
    }
}

bool MigratorModel::addMigrator(const QSharedPointer<MigratorBase> &m)
{
    if (migrator(m->identifier())) {
        kWarning() << "Model already contains a migrator with the identifier: " << m;
        return false;
    }
    const int pos = mMigrators.size();
    beginInsertRows(QModelIndex(), pos, pos);
    mMigrators.append(QSharedPointer<Row>(new Row(m, *this)));
    endInsertRows();
    return true;
}

int MigratorModel::columnCount(const QModelIndex &/*parent*/) const
{
    return ColumnCount;
}

int MigratorModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return mMigrators.size();
    }
    return 0;
}

QModelIndex MigratorModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row >= rowCount(parent) || row < 0) {
        return QModelIndex();
    }
    return createIndex(row, column, static_cast<void*>(mMigrators.at(row).data()));
}

QModelIndex MigratorModel::parent(const QModelIndex &/*child*/) const
{
    return QModelIndex();
}

QVariant MigratorModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (section) {
            case Name:
                return i18nc("Name of the migrator in this row", "Name");
            case Progress:
                return i18nc("Progress of the mgirator in %", "Progress");
            case State:
                return i18nc("Current status of the migrator (done, in progress, ...)", "Status");
            default:
                Q_ASSERT(false);
        }
    }
    return QVariant();
}

QVariant MigratorModel::data(const QModelIndex &index, int role) const
{
    const Row *row = static_cast<Row*>(index.internalPointer());
    const QSharedPointer<MigratorBase> migrator(row->mMigrator);
    if (!migrator) {
        kWarning() << "migrator not found";
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
                case Name:
                    return migrator->displayName();
                case Progress:
                    return QString::fromLatin1("%1 %").arg(migrator->progress());
                case State:
                    return migrator->status();
                default:
                    Q_ASSERT(false);
            }
        case IdentifierRole:
            return migrator->identifier();
        case LogfileRole:
            return migrator->logfile();
        case Qt::ToolTipRole:
            return migrator->description();
        default:
            break;
    }
    return QVariant();
}

QSharedPointer<MigratorBase> MigratorModel::migrator(const QString &identifier) const
{
    foreach (const QSharedPointer<Row> &row, mMigrators) {
        if (row->mMigrator->identifier() == identifier) {
            return row->mMigrator;
        }
    }
    return QSharedPointer<MigratorBase>();
}

QList< QSharedPointer<MigratorBase> > MigratorModel::migrators() const
{
    QList< QSharedPointer<MigratorBase> > migrators;
    foreach (const QSharedPointer<Row> &row, mMigrators) {
        return migrators << row->mMigrator;
    }
    return migrators;
}

MigrationScheduler::MigrationScheduler(KJobTrackerInterface *jobTracker, QObject *parent)
    :QObject(parent),
    mModel(new MigratorModel),
    mJobTracker(jobTracker)
{
}

MigrationScheduler::~MigrationScheduler()
{
    delete mAutostartExecutor;
}

void MigrationScheduler::addMigrator(const QSharedPointer<MigratorBase> &migrator)
{
    if (mModel->addMigrator(migrator)) {
        QSharedPointer<LogModel> logModel(new LogModel);
        connect(migrator.data(), SIGNAL(message(MigratorBase::MessageType,QString)), logModel.data(), SLOT(message(MigratorBase::MessageType,QString)));
        mLogModel.insert(migrator->identifier(), logModel);
        if (migrator->shouldAutostart()) {
            checkForAutostart(migrator);
        }
    }
}

QAbstractItemModel& MigrationScheduler::model()
{
    return *mModel;
}

QStandardItemModel& MigrationScheduler::logModel(const QString &identifier)
{
    Q_ASSERT(mLogModel.contains(identifier));
    return *mLogModel.value(identifier);
}

void MigrationScheduler::checkForAutostart(const QSharedPointer<MigratorBase> &migrator)
{
    if (migrator->migrationState() != MigratorBase::Complete) {

        if (!mAutostartExecutor) {
            mAutostartExecutor = new MigrationExecutor;
            if (mJobTracker) {
                mJobTracker->registerJob(mAutostartExecutor);
            }

            mAutostartExecutor->start();
        }

        mAutostartExecutor->add(migrator);
    }
}

void MigrationScheduler::start(const QString &identifier)
{
    //TODO create separate executor?
    const QSharedPointer<MigratorBase> m = mModel->migrator(identifier);
    if (m) {
        m->start();
    }
}

void MigrationScheduler::pause(const QString &identifier)
{
    const QSharedPointer<MigratorBase> m = mModel->migrator(identifier);
    if (m) {
        m->pause();
    }
}

void MigrationScheduler::abort(const QString &identifier)
{
    const QSharedPointer<MigratorBase> m = mModel->migrator(identifier);
    if (m) {
        m->abort();
    }
}

