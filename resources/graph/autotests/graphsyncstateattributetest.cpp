/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Unit tests for the delta-link collection attribute.
*/

#include "graphsyncstateattribute.h"

#include <QTest>

#include <memory>

class GraphSyncStateAttributeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void shouldBeEmptyByDefault()
    {
        const GraphSyncStateAttribute attr;
        QVERIFY(attr.deltaLink().isEmpty());
        QCOMPARE(attr.type(), QByteArrayLiteral("graphsyncstate"));
    }

    void shouldRoundTripThroughSerialization()
    {
        const QString link = QStringLiteral("https://graph.microsoft.com/v1.0/me/mailFolders/AAA/messages/delta?$deltatoken=äöü%20token");
        GraphSyncStateAttribute attr(link);
        QCOMPARE(attr.deltaLink(), link);

        GraphSyncStateAttribute copy;
        copy.deserialize(attr.serialized());
        QCOMPARE(copy.deltaLink(), link);
    }

    void shouldCloneIndependently()
    {
        GraphSyncStateAttribute attr(QStringLiteral("first"));
        const std::unique_ptr<Akonadi::Attribute> clone(attr.clone());
        attr.setDeltaLink(QStringLiteral("second"));

        const auto cloned = static_cast<GraphSyncStateAttribute *>(clone.get());
        QCOMPARE(cloned->deltaLink(), QStringLiteral("first"));
        QCOMPARE(cloned->type(), attr.type());
    }
};

QTEST_GUILESS_MAIN(GraphSyncStateAttributeTest)

#include "graphsyncstateattributetest.moc"
