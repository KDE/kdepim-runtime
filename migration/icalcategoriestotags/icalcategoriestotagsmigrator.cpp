/*
    SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "icalcategoriestotagsmigrator.h"
#include "migration_debug.h"

#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/CollectionStatistics>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <KLocalizedString>

#include <KCompositeJob>

#include <KCalendarCore/Incidence>

#include <QTimer>
#include <akonadi/itemmodifyjob.h>

class MigrationJob : public KCompositeJob
{
    Q_OBJECT
public:
    enum Error {
        Canceled = KCompositeJob::UserDefinedError
    };

    explicit MigrationJob(const Akonadi::Collection &collection, QObject *parent = nullptr)
        : KCompositeJob(parent)
        , mCollection(collection)
    {
    }

    void start() override
    {
        auto fetch = new Akonadi::ItemFetchJob(mCollection, this);
        fetch->fetchScope().fetchFullPayload(true);
        fetch->fetchScope().setFetchTags(true);
        connect(fetch, &Akonadi::ItemFetchJob::result, this, [this, fetch]() {
            if (fetch->error()) {
                qCWarning(MIGRATION_LOG) << "Failed to fetch items:" << fetch->errorString();
                setError(fetch->error());
                emitResult();
                return;
            }

            if (mStop) {
                setError(Canceled);
                emitResult();
                return;
            }

            mItems = fetch->items();
            setTotalAmount(Unit::Items, mItems.size());

            migrateNextItem();
        });
    }

    void stop()
    {
        mStop = true;
    }

private:
    void migrateNextItem()
    {
        if (mItems.empty()) {
            emitResult();
            return;
        }

        if (mStop) {
            setError(Canceled);
            emitResult();
            return;
        }

        const auto item = mItems.takeFirst();
        const auto missingTags = calculateMissingTags(item);
        if (!missingTags.empty()) {
            migrateItem(item, missingTags);
        } else {
            setProcessedAmount(Unit::Items, ++mProcessed);
            QTimer::singleShot(0, this, &MigrationJob::migrateNextItem);
        }
    }

    Akonadi::Tag::List calculateMissingTags(const Akonadi::Item &item) const
    {
        const auto payload = item.payload<KCalendarCore::Incidence::Ptr>();
        if (!payload) {
            return {};
        }

        const auto tags = item.tags();
        const auto categories = payload->categories();
        Akonadi::Tag::List missingTags;

        for (const auto &category : categories) {
            if (std::find_if(tags.begin(),
                             tags.end(),
                             [category](const auto &tag) {
                                 return tag.name() == category;
                             })
                == tags.end()) {
                missingTags.emplace_back(category);
            }
        }

        return missingTags;
    }

    void migrateItem(const Akonadi::Item &item, const Akonadi::Tag::List &tags)
    {
        Akonadi::Item modified = item;
        modified.setTags(tags);

        auto *modifyJob = new Akonadi::ItemModifyJob(modified, this);
        connect(modifyJob, &Akonadi::ItemModifyJob::result, this, [this, modified, modifyJob]() {
            if (modifyJob->error()) {
                qCWarning(MIGRATION_LOG) << "Failed to add tags to item:" << modifyJob->errorString();
                setError(modifyJob->error());
                setErrorText(modifyJob->errorString());
                emitResult();
                return;
            }

            setProcessedAmount(Unit::Items, ++mProcessed);

            migrateNextItem();
        });
    }

private:
    Akonadi::Collection mCollection;
    qsizetype mProcessed = 0;
    Akonadi::Item::List mItems;
    bool mStop = false;
};

ICalCategoriesToTagsMigrator::ICalCategoriesToTagsMigrator()
    : MigratorBase(QStringLiteral("icalcategoriestotagsmigrator"))
{
}

void ICalCategoriesToTagsMigrator::pause()
{
    mPause = true;
    if (auto *job = dynamic_cast<MigrationJob *>(mCurrentJob); job) {
        job->stop();
    }
}

void ICalCategoriesToTagsMigrator::abort()
{
    mAbort = true;
    if (auto *job = dynamic_cast<MigrationJob *>(mCurrentJob); job) {
        job->stop();
    }
}

void ICalCategoriesToTagsMigrator::resume()
{
    mPause = false;
    discoverCalendarCollections();
}

QString ICalCategoriesToTagsMigrator::displayName() const
{
    return i18nc("Name of the migrator", "iCal Categories to Tags Migrator");
}

QString ICalCategoriesToTagsMigrator::description() const
{
    return i18nc("Description of the migrator", "Migrates iCal categories to Akonadi tags.");
}

bool ICalCategoriesToTagsMigrator::shouldAutostart() const
{
    return true;
}

void ICalCategoriesToTagsMigrator::startWork()
{
    discoverCalendarCollections();
}

void ICalCategoriesToTagsMigrator::discoverCalendarCollections()
{
    auto job = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive);
    job->fetchScope().setContentMimeTypes({QStringLiteral("text/calendar"),
                                           QStringLiteral("application/x-vnd.akonadi.calendar.event"),
                                           QStringLiteral("application/x-vnd.akonadi.calendar.todo")});
    job->fetchScope().setIncludeStatistics(true);
    connect(job, &Akonadi::CollectionFetchJob::result, this, [this, job]() {
        if (job->error()) {
            qCWarning(MIGRATION_LOG) << "Failed to fetch collections:" << job->errorString();
            setMigrationState(MigrationState::Failed);
            return;
        }

        mPendingCollections = job->collections();
        mTotalItemsCount = std::accumulate(mPendingCollections.begin(), mPendingCollections.end(), 0, [](auto acc, const auto &col) {
            return acc + col.statistics().count();
        });
        qCDebug(MIGRATION_LOG) << "Discovered" << mPendingCollections.size() << "calendar collections with" << mTotalItemsCount << "items in total";
        migrateNextCollection();
    });
}

void ICalCategoriesToTagsMigrator::migrateNextCollection()
{
    if (mPendingCollections.empty()) {
        setMigrationState(MigrationState::Complete);
        return;
    }

    if (mAbort) {
        setMigrationState(MigrationState::Aborted);
        return;
    }

    if (mPause) {
        setMigrationState(MigrationState::Paused);
        return;
    }

    auto collection = mPendingCollections.takeFirst();
    if (mCompletedCollections.contains(collection)) {
        QTimer::singleShot(0, this, &ICalCategoriesToTagsMigrator::migrateNextCollection);
        return;
    }

    mProcessedItemsCount = 0;
    auto *job = new MigrationJob(collection, this);
    mCurrentJob = job;
    connect(job, &MigrationJob::result, this, [this, collection, job]() {
        if (job->error() == MigrationJob::Canceled) {
            // Return the collection back to the pending list
            mPendingCollections.push_back(collection);
        } else {
            mTotalProcessedItemsCount += mProcessedItemsCount;
            mCompletedCollections.push_back(collection);
        }
        mCurrentJob = nullptr;
        QTimer::singleShot(0, this, &ICalCategoriesToTagsMigrator::migrateNextCollection);
    });
    connect(job, &MigrationJob::processedAmountChanged, this, [this](KJob *, KJob::Unit, qulonglong amount) {
        mProcessedItemsCount = amount;
        setProgress((static_cast<float>(mTotalProcessedItemsCount + mProcessedItemsCount) / static_cast<float>(mTotalItemsCount)) * 100);
    });

    job->start();
}

void ICalCategoriesToTagsMigrator::migrateCollection(const Akonadi::Collection &collection)
{
    auto job = new Akonadi::ItemFetchJob(collection, this);
    job->fetchScope().fetchFullPayload(true);
    job->fetchScope().setFetchTags(true);
}

#include "icalcategoriestotagsmigrator.moc"
#include "moc_icalcategoriestotagsmigrator.cpp"
