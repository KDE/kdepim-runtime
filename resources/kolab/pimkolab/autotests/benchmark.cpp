/*
 * Copyright (C) 2011  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "benchmark.h"
#include "kolabformatV2/event.h"
#include "conversion/kcalconversion.h"
#include <kmime/kmime_message.h>
#include <kolabformat.h>
#include "testutils.h"

KMime::Message::Ptr readMimeFile(const QString &fileName)
{
    QFile file(fileName);
    file.open(QFile::ReadOnly);
    const QByteArray data = file.readAll();
    Q_ASSERT(!data.isEmpty());

    KMime::Message *msg = new KMime::Message;
    msg->setContent(data);
    msg->parse();
    return KMime::Message::Ptr(msg);
}

KMime::Content *findContentByType(const KMime::Message::Ptr &data, const QByteArray &type)
{
    const KMime::Content::List list = data->contents();
    for (KMime::Content *c : list) {
        if (c->contentType()->mimeType() == type) {
            return c;
        }
    }
    return nullptr;
}

void BenchmarkTests::parsingBenchmarkComparison_data()
{
    QTest::addColumn<bool>("v2Parser");
    QTest::newRow("v2") << true;
    QTest::newRow("v3") << false;
}

void BenchmarkTests::parsingBenchmarkComparison()
{
    const KMime::Message::Ptr kolabItem = readMimeFile(TESTFILEDIR+QLatin1String("/v2/event/complex.ics.mime"));
    KMime::Content *xmlContent = findContentByType(kolabItem, "application/x-vnd.kolab.event");
    QVERIFY(xmlContent);
    const QByteArray xmlData = xmlContent->decodedContent();
    //     qDebug() << xmlData;
    const QDomDocument xmlDoc = KolabV2::Event::loadDocument(QString::fromUtf8(xmlData));
    QVERIFY(!xmlDoc.isNull());
    const KCalCore::Event::Ptr i = KolabV2::Event::fromXml(xmlDoc, QStringLiteral("Europe/Berlin"));
    QVERIFY(i);
    const Kolab::Event &event = Kolab::Conversion::fromKCalCore(*i);
    const std::string &v3String = Kolab::writeEvent(event);

    QFETCH(bool, v2Parser);

    //     Kolab::readEvent(v3String, false); //init parser (doesn't really change the results it seems)
    //     qDebug() << QString::fromUtf8(xmlData);
    //     qDebug() << "------------------------------------------------------------------------------------";
    //     qDebug() << QString::fromStdString(v3String);
    if (v2Parser) {
        QBENCHMARK {
            KolabV2::Event::fromXml(KolabV2::Event::loadDocument(QString::fromUtf8(xmlData)), QStringLiteral("Europe/Berlin"));
        }
    } else {
        QBENCHMARK {
            Kolab::readEvent(v3String, false);
        }
    }
}

QTEST_MAIN(BenchmarkTests)
