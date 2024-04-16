/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsid.h"

#include <QDebug>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "ewsclient.h"
#include "ewsclient_debug.h"

static constexpr auto distinguishedIdNames = std::to_array({
    QLatin1StringView("calendar"),
    QLatin1StringView("contacts"),
    QLatin1StringView("deleteditems"),
    QLatin1StringView("drafts"),
    QLatin1StringView("inbox"),
    QLatin1StringView("journal"),
    QLatin1StringView("notes"),
    QLatin1StringView("outbox"),
    QLatin1StringView("sentitems"),
    QLatin1StringView("tasks"),
    QLatin1StringView("msgfolderroot"),
    QLatin1StringView("root"),
    QLatin1StringView("junkemail"),
    QLatin1StringView("searchfolders"),
    QLatin1StringView("voicemail"),
    QLatin1StringView("recoverableitemsroot"),
    QLatin1StringView("recoverableitemsdeletions"),
    QLatin1StringView("recoverableitemsversions"),
    QLatin1StringView("recoverableitemspurges"),
    QLatin1StringView("archiveroot"),
    QLatin1StringView("archivemsgfolderroot"),
    QLatin1StringView("archivedeleteditems"),
    QLatin1StringView("archiverecoverableitemsroot"),
    QLatin1StringView("archiverecoverableitemsdeletions"),
    QLatin1StringView("archiverecoverableitemsversions"),
    QLatin1StringView("archiverecoverableitemspurges"),
});

EwsId::EwsId(QXmlStreamReader &reader)
    : mDid(EwsDIdCalendar)
{
    // Don't check for this element's name as a folder id may be contained in several elements
    // such as "FolderId" or "ParentFolderId".
    const QXmlStreamAttributes &attrs = reader.attributes();

    QStringView idRef = attrs.value(QStringLiteral("Id"));
    QStringView changeKeyRef = attrs.value(QStringLiteral("ChangeKey"));

    if (idRef.isNull()) {
        return;
    }

    mId = idRef.toString();
    if (!changeKeyRef.isNull()) {
        mChangeKey = changeKeyRef.toString();
    }
    mType = Real;
}

EwsId::EwsId(const QString &id, const QString &changeKey)
    : mType(Real)
    , mId(id)
    , mChangeKey(changeKey)
    , mDid(EwsDIdCalendar)
{
}

EwsId &EwsId::operator=(const EwsId &other)
{
    mType = other.mType;
    if (mType == Distinguished) {
        mDid = other.mDid;
    } else if (mType == Real) {
        mId = other.mId;
        mChangeKey = other.mChangeKey;
    }
    return *this;
}

EwsId &EwsId::operator=(EwsId &&other)
{
    mType = other.mType;
    if (mType == Distinguished) {
        mDid = other.mDid;
    } else if (mType == Real) {
        mId = std::move(other.mId);
        mChangeKey = std::move(other.mChangeKey);
    }
    return *this;
}

bool EwsId::operator==(const EwsId &other) const
{
    if (mType != other.mType) {
        return false;
    }

    if (mType == Distinguished) {
        return mDid == other.mDid;
    } else if (mType == Real) {
        return mId == other.mId && mChangeKey == other.mChangeKey;
    }
    return true;
}

bool EwsId::operator<(const EwsId &other) const
{
    if (mType != other.mType) {
        return mType < other.mType;
    }

    if (mType == Distinguished) {
        return mDid < other.mDid;
    } else if (mType == Real) {
        return mId < other.mId && mChangeKey < other.mChangeKey;
    }
    return false;
}

void EwsId::writeFolderIds(QXmlStreamWriter &writer) const
{
    if (mType == Distinguished) {
        writer.writeStartElement(ewsTypeNsUri, QStringLiteral("DistinguishedFolderId"));
        writer.writeAttribute(QStringLiteral("Id"), distinguishedIdNames[mDid]);
        writer.writeEndElement();
    } else if (mType == Real) {
        writer.writeStartElement(ewsTypeNsUri, QStringLiteral("FolderId"));
        writer.writeAttribute(QStringLiteral("Id"), mId);
        if (!mChangeKey.isEmpty()) {
            writer.writeAttribute(QStringLiteral("ChangeKey"), mChangeKey);
        }
        writer.writeEndElement();
    }
}

void EwsId::writeItemIds(QXmlStreamWriter &writer) const
{
    if (mType == Real) {
        writer.writeStartElement(ewsTypeNsUri, QStringLiteral("ItemId"));
        writer.writeAttribute(QStringLiteral("Id"), mId);
        if (!mChangeKey.isEmpty()) {
            writer.writeAttribute(QStringLiteral("ChangeKey"), mChangeKey);
        }
        writer.writeEndElement();
    }
}

void EwsId::writeAttributes(QXmlStreamWriter &writer) const
{
    if (mType == Real) {
        writer.writeAttribute(QStringLiteral("Id"), mId);
        if (!mChangeKey.isEmpty()) {
            writer.writeAttribute(QStringLiteral("ChangeKey"), mChangeKey);
        }
    }
}

QDebug operator<<(QDebug debug, const EwsId &id)
{
    QDebugStateSaver saver(debug);
    QDebug d = debug.nospace().noquote();
    d << QStringLiteral("EwsId(");

    switch (id.mType) {
    case EwsId::Distinguished:
        d << QStringLiteral("Distinguished: ") << distinguishedIdNames[id.mDid];
        break;
    case EwsId::Real: {
        QString name = EwsClient::folderHash.value(id.mId, ewsHash(id.mId));
        d << name << QStringLiteral(", ") << ewsHash(id.mChangeKey);
        break;
    }
    default:
        break;
    }
    d << ')';
    return debug;
}

size_t qHash(const EwsId &id, size_t seed) noexcept
{
    return qHashMulti(seed, id.id(), id.changeKey(), id.type());
}
