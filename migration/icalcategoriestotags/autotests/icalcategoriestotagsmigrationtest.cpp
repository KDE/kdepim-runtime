/*
    SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "icalcategoriestotags/icalcategoriestotagsmigrator.h"

#include <akonadi/collectionfetchscope.h>
#include <akonadi/qtest_akonadi.h>

#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/TagFetchScope>

#include <KCalendarCore/Event>

#include <QObject>

class ICalCategoriesToTagsMigrationTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }

    void testMigration()
    {
        // Create an item with an event
        auto event = KCalendarCore::Event::Ptr::create();
        event->setSummary(QStringLiteral("PIM Sprint"));
        event->setAllDay(true);
        event->setDtStart(QDateTime{{2024, 6, 14}, {0, 0, 0}});
        event->setDtEnd(QDateTime{{2024, 6, 16}, {0, 0, 0}});
        event->setCategories({QStringLiteral("KDE"), QStringLiteral("PIM")});

        const Akonadi::Collection parent{AkonadiTest::collectionIdFromPath(QStringLiteral("res1/events"))};
        QVERIFY(parent.isValid());

        Akonadi::Item item;
        item.setParentCollection(parent);
        item.setMimeType(QStringLiteral("text/calendar"));
        item.setPayload<KCalendarCore::Event::Ptr>(event);

        auto createJob = new Akonadi::ItemCreateJob(item, item.parentCollection());
        AKVERIFYEXEC(createJob);
        const auto createdItem = createJob->item();

        ICalCategoriesToTagsMigrator migrator;
        QSignalSpy stateSpy(&migrator, &ICalCategoriesToTagsMigrator::stateChanged);
        QSignalSpy progressSpy(&migrator, qOverload<int>(&ICalCategoriesToTagsMigrator::progress));

        migrator.start();

        while (!stateSpy.isEmpty() && stateSpy.last().first().value<MigratorBase::MigrationState>() != MigratorBase::Complete) {
            stateSpy.clear();
            QTRY_VERIFY(stateSpy.count() > 0);
        }

        auto fetchJob = new Akonadi::ItemFetchJob(createdItem, this);
        fetchJob->fetchScope().setFetchTags(true);
        fetchJob->fetchScope().tagFetchScope().setFetchIdOnly(false);
        AKVERIFYEXEC(fetchJob);

        QCOMPARE(fetchJob->items().size(), 1);
        const auto fetchedItem = fetchJob->items().constFirst();

        auto tags = fetchedItem.tags();
        std::sort(tags.begin(), tags.end(), [](const auto &lhs, const auto &rhs) {
            return lhs.name() < rhs.name();
        });
        QCOMPARE(tags.size(), 2);
        QCOMPARE(tags[0].name(), QStringLiteral("KDE"));
        QCOMPARE(tags[1].name(), QStringLiteral("PIM"));
    }
};

QTEST_AKONADIMAIN(ICalCategoriesToTagsMigrationTest)

#include "icalcategoriestotagsmigrationtest.moc"
