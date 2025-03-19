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

using namespace Qt::StringLiterals;

static constexpr auto distinguishedIdNames = std::to_array({
    "calendar"_L1,
    "contacts"_L1,
    "deleteditems"_L1,
    "drafts"_L1,
    "inbox"_L1,
    "journal"_L1,
    "notes"_L1,
    "outbox"_L1,
    "sentitems"_L1,
    "tasks"_L1,
    "msgfolderroot"_L1,
    "root"_L1,
    "junkemail"_L1,
    "searchfolders"_L1,
    "voicemail"_L1,
    "recoverableitemsroot"_L1,
    "recoverableitemsdeletions"_L1,
    "recoverableitemsversions"_L1,
    "recoverableitemspurges"_L1,
    "archiveroot"_L1,
    "archivemsgfolderroot"_L1,
    "archivedeleteditems"_L1,
    "archiverecoverableitemsroot"_L1,
    "archiverecoverableitemsdeletions"_L1,
    "archiverecoverableitemsversions"_L1,
    "archiverecoverableitemspurges"_L1,
});

EwsId::EwsId(QXmlStreamReader &reader)
    : mDid(EwsDIdCalendar)
{
    // Don't check for this element's name as a folder id may be contained in several elements
    // such as "FolderId" or "ParentFolderId".
    const QXmlStreamAttributes &attrs = reader.attributes();

    QStringView idRef = attrs.value("Id"_L1);
    QStringView changeKeyRef = attrs.value("ChangeKey"_L1);

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
        writer.writeStartElement(ewsTypeNsUri, "DistinguishedFolderId"_L1);
        writer.writeAttribute("Id"_L1, distinguishedIdNames[mDid]);
        writer.writeEndElement();
    } else if (mType == Real) {
        writer.writeStartElement(ewsTypeNsUri, "FolderId"_L1);
        writer.writeAttribute("Id"_L1, mId);
        if (!mChangeKey.isEmpty()) {
            writer.writeAttribute("ChangeKey"_L1, mChangeKey);
        }
        writer.writeEndElement();
    }
}

void EwsId::writeItemIds(QXmlStreamWriter &writer) const
{
    if (mType == Real) {
        writer.writeStartElement(ewsTypeNsUri, "ItemId"_L1);
        writer.writeAttribute("Id"_L1, mId);
        if (!mChangeKey.isEmpty()) {
            writer.writeAttribute("ChangeKey"_L1, mChangeKey);
        }
        writer.writeEndElement();
    }
}

void EwsId::writeAttributes(QXmlStreamWriter &writer) const
{
    if (mType == Real) {
        writer.writeAttribute("Id"_L1, mId);
        if (!mChangeKey.isEmpty()) {
            writer.writeAttribute("ChangeKey"_L1, mChangeKey);
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
