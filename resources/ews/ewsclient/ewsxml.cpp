/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsxml.h"

#include <QDateTime>

#include "ewsfolder.h"
#include "ewsid.h"
#include "ewsitem.h"

using namespace Qt::StringLiterals;

static constexpr auto messageSensitivityNames = std::to_array<QLatin1StringView>({
    "Normal"_L1,
    "Personal"_L1,
    "Private"_L1,
    "Confidential"_L1,
});

static constexpr auto messageImportanceNames = std::to_array<QLatin1StringView>({
    "Low"_L1,
    "Normal"_L1,
    "High"_L1,
});

static constexpr auto calendarItemTypeNames = std::to_array<QLatin1StringView>({
    "Single"_L1,
    "Occurrence"_L1,
    "Exception"_L1,
    "RecurringMaster"_L1,
});

static constexpr auto legacyFreeBusyStatusNames = std::to_array<QLatin1StringView>({
    "Free"_L1,
    "Tentative"_L1,
    "Busy"_L1,
    "OOF"_L1,
    "NoData"_L1,
});

static constexpr auto responseTypeNames = std::to_array<QLatin1StringView>({
    "Unknown"_L1,
    "Organizer"_L1,
    "Tentative"_L1,
    "Accept"_L1,
    "Decline"_L1,
    "NoResponseReceived"_L1,
});

bool ewsXmlBoolReader(QXmlStreamReader &reader, QVariant &val)
{
    const QString elmText = reader.readElementText();
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading %1 element").arg(reader.name().toString());
        reader.skipCurrentElement();
        return false;
    }
    if (elmText == "true"_L1) {
        val = true;
    } else if (elmText == "false"_L1) {
        val = false;
    } else {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unexpected invalid boolean value in %1 element:").arg(reader.name().toString()) << elmText;
        return false;
    }

    return true;
}

bool ewsXmlBoolWriter(QXmlStreamWriter &writer, const QVariant &val)
{
    writer.writeCharacters(val.toBool() ? "true"_L1 : "false"_L1);

    return true;
}

bool ewsXmlBase64Reader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    val = QByteArray::fromBase64(reader.readElementText().toLatin1());
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid content.").arg(elmName);
        reader.skipCurrentElement();
        return false;
    }

    return true;
}

bool ewsXmlBase64Writer(QXmlStreamWriter &writer, const QVariant &val)
{
    writer.writeCharacters(QString::fromLatin1(val.toByteArray().toBase64()));

    return true;
}

bool ewsXmlIdReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    EwsId id = EwsId(reader);
    if (id.type() == EwsId::Unspecified) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid content.").arg(elmName);
        reader.skipCurrentElement();
        return false;
    }
    val = QVariant::fromValue(id);
    reader.skipCurrentElement();
    return true;
}

bool ewsXmlIdWriter(QXmlStreamWriter &writer, const QVariant &val)
{
    EwsId id = val.value<EwsId>();
    if (id.type() == EwsId::Unspecified) {
        return false;
    }

    id.writeAttributes(writer);

    return true;
}

bool ewsXmlTextReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    val = reader.readElementText();
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid content.").arg(elmName);
        reader.skipCurrentElement();
        return false;
    }
    return true;
}

bool ewsXmlTextWriter(QXmlStreamWriter &writer, const QVariant &val)
{
    writer.writeCharacters(val.toString());

    return true;
}

bool ewsXmlUIntReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    bool ok;
    val = reader.readElementText().toUInt(&ok);
    if (reader.error() != QXmlStreamReader::NoError || !ok) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid content.").arg(elmName);
        return false;
    }
    return true;
}

bool ewsXmlUIntWriter(QXmlStreamWriter &writer, const QVariant &val)
{
    writer.writeCharacters(QString::number(val.toUInt()));

    return true;
}

bool ewsXmlDateTimeReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    QDateTime dt = QDateTime::fromString(reader.readElementText(), Qt::ISODate);
    if (reader.error() != QXmlStreamReader::NoError || !dt.isValid()) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid content.").arg(elmName);
        return false;
    }
    val = QVariant::fromValue<QDateTime>(dt);
    return true;
}

bool ewsXmlItemReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    EwsItem item = EwsItem(reader);
    if (!item.isValid()) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid content.").arg(elmName);
        reader.skipCurrentElement();
        return false;
    }
    val = QVariant::fromValue(item);
    return true;
}

bool ewsXmlFolderReader(QXmlStreamReader &reader, QVariant &val)
{
    const QString elmName = reader.name().toString();
    EwsFolder folder = EwsFolder(reader);
    if (!folder.isValid()) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid content.").arg(elmName);
        reader.skipCurrentElement();
        return false;
    }
    val = QVariant::fromValue(folder);
    return true;
}

template<std::size_t SIZE>
bool ewsXmlEnumReader(QXmlStreamReader &reader, QVariant &val, const std::array<QLatin1StringView, SIZE> &items)
{
    const QString elmName = reader.name().toString();
    QString text = reader.readElementText();
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid content.").arg(elmName);
        reader.skipCurrentElement();
        return false;
    }
    int i = 0;
    auto it = items.cbegin();
    for (; it != items.cend(); ++it, i++) {
        if (text == *it) {
            val = i;
            break;
        }
    }

    if (it == items.cend()) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - unknown value %2.").arg(elmName, text);
        return false;
    }
    return true;
}

bool ewsXmlSensitivityReader(QXmlStreamReader &reader, QVariant &val)
{
    return ewsXmlEnumReader(reader, val, messageSensitivityNames);
}

bool ewsXmlImportanceReader(QXmlStreamReader &reader, QVariant &val)
{
    return ewsXmlEnumReader(reader, val, messageImportanceNames);
}

bool ewsXmlCalendarItemTypeReader(QXmlStreamReader &reader, QVariant &val)
{
    return ewsXmlEnumReader(reader, val, calendarItemTypeNames);
}

bool ewsXmlLegacyFreeBusyStatusReader(QXmlStreamReader &reader, QVariant &val)
{
    return ewsXmlEnumReader(reader, val, legacyFreeBusyStatusNames);
}

bool ewsXmlResponseTypeReader(QXmlStreamReader &reader, QVariant &val)
{
    return ewsXmlEnumReader(reader, val, responseTypeNames);
}

template<>
QString readXmlElementValue(QXmlStreamReader &reader, bool &ok, const QString &parentElement)
{
    ok = true;
    const QStringView elmName = reader.name();
    QString val = reader.readElementText();
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.").arg(parentElement, elmName.toString());
        reader.skipCurrentElement();
        val.clear();
        ok = false;
    }

    return val;
}

template<>
int readXmlElementValue(QXmlStreamReader &reader, bool &ok, const QString &parentElement)
{
    const QStringView elmName = reader.name();
    QString valStr = readXmlElementValue<QString>(reader, ok, parentElement);
    int val = 0;
    if (ok) {
        val = valStr.toInt(&ok);
        if (!ok) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.").arg(parentElement, elmName.toString());
        }
    }

    return val;
}

template<>
long readXmlElementValue(QXmlStreamReader &reader, bool &ok, const QString &parentElement)
{
    const QStringView elmName = reader.name();
    QString valStr = readXmlElementValue<QString>(reader, ok, parentElement);
    long val = 0;
    if (ok) {
        val = valStr.toLong(&ok);
        if (!ok) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.").arg(parentElement, elmName.toString());
        }
    }

    return val;
}

template<>
QDateTime readXmlElementValue(QXmlStreamReader &reader, bool &ok, const QString &parentElement)
{
    const QStringView elmName = reader.name();
    QString valStr = readXmlElementValue<QString>(reader, ok, parentElement);
    QDateTime val;
    if (ok) {
        val = QDateTime::fromString(valStr, Qt::ISODate);
        if (!val.isValid()) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.").arg(parentElement, elmName.toString());
            ok = false;
        }
    }

    return val;
}

template<>
bool readXmlElementValue(QXmlStreamReader &reader, bool &ok, const QString &parentElement)
{
    const QStringView elmName = reader.name();
    QString valStr = readXmlElementValue<QString>(reader, ok, parentElement);
    bool val = false;
    if (ok) {
        if (valStr == "true"_L1) {
            val = true;
        } else if (valStr == "false"_L1) {
            val = false;
        } else {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.").arg(parentElement, elmName.toString());
            ok = false;
        }
    }

    return val;
}

template<>
QByteArray readXmlElementValue(QXmlStreamReader &reader, bool &ok, const QString &parentElement)
{
    QString valStr = readXmlElementValue<QString>(reader, ok, parentElement);
    QByteArray val;
    if (ok) {
        /* QByteArray::fromBase64() does not perform any input validity checks
           and skips invalid input characters */
        val = QByteArray::fromBase64(valStr.toLatin1());
    }

    return val;
}
