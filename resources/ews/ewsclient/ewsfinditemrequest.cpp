/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsfinditemrequest.h"

#include <memory>

#include <QXmlStreamWriter>

#include "ewsclient_debug.h"

using namespace Qt::StringLiterals;

static constexpr auto traversalTypeNames = std::to_array({
    "Shallow"_L1,
    "Deep"_L1,
    "SoftDeleted"_L1,
    "Associated"_L1,
});

class EwsFindItemResponse : public EwsRequest::Response
{
public:
    EwsFindItemResponse(QXmlStreamReader &reader);
    bool parseRootFolder(QXmlStreamReader &reader);
    EwsItem *readItem(QXmlStreamReader &reader);

    QList<EwsItem> mItems;
    unsigned mTotalItems;
    int mNextOffset;
    int mNextNumerator;
    int mNextDenominator;
    bool mIncludesLastItem;
};

EwsFindItemRequest::EwsFindItemRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent)
    , mTraversal(EwsTraversalShallow)
    , mPagination(false)
    , mPageBasePoint(EwsBasePointBeginning)
    , mPageOffset(0)
    , mFractional(false)
    , mMaxItems(-1)
    , mFracNumerator(0)
    , mFracDenominator(0)
    , mTotalItems(0)
    , mNextOffset(-1)
    , mNextNumerator(-1)
    , mNextDenominator(-1)
    , mIncludesLastItem(false)
{
}

EwsFindItemRequest::~EwsFindItemRequest()
{
}

void EwsFindItemRequest::setFolderId(const EwsId &id)
{
    mFolderId = id;
}

void EwsFindItemRequest::setItemShape(const EwsItemShape &shape)
{
    mShape = shape;
}

void EwsFindItemRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, "FindItem"_L1);
    writer.writeAttribute("Traversal"_L1, traversalTypeNames[mTraversal]);

    mShape.write(writer);

    if (mPagination) {
        writer.writeStartElement(ewsMsgNsUri, "IndexedPageItemView"_L1);
        if (mMaxItems > 0) {
            writer.writeAttribute("MaxEntriesReturned"_L1, QString::number(mMaxItems));
        }
        writer.writeAttribute("Offset"_L1, QString::number(mPageOffset));
        writer.writeAttribute("BasePoint"_L1, (mPageBasePoint == EwsBasePointEnd) ? "End"_L1 : "Beginning"_L1);
        writer.writeEndElement();
    } else if (mFractional) {
        writer.writeStartElement(ewsMsgNsUri, "FractionalPageItemView"_L1);
        if (mMaxItems > 0) {
            writer.writeAttribute("MaxEntriesReturned"_L1, QString::number(mMaxItems));
        }
        writer.writeAttribute("Numerator"_L1, QString::number(mFracNumerator));
        writer.writeAttribute("Denominator"_L1, QString::number(mFracDenominator));
        writer.writeEndElement();
    }

    writer.writeStartElement(ewsMsgNsUri, "ParentFolderIds"_L1);
    mFolderId.writeFolderIds(writer);
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebug(EWSCLI_PROTO_LOG) << reqString;

    qCDebugNC(EWSCLI_REQUEST_LOG) << QStringLiteral("Starting FindItems request (folder: ") << mFolderId << QStringLiteral(")");

    prepare(reqString);

    doSend();
}

bool EwsFindItemRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("FindItem"), [this](QXmlStreamReader &reader) {
        return parseItemsResponse(reader);
    });
}

bool EwsFindItemRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    auto resp = new EwsFindItemResponse(reader);
    if (resp->responseClass() == EwsResponseUnknown) {
        return false;
    }

    mItems = resp->mItems;
    mTotalItems = resp->mTotalItems;
    mNextOffset = resp->mNextOffset;
    mNextNumerator = resp->mNextNumerator;
    mNextDenominator = resp->mNextDenominator;
    mIncludesLastItem = resp->mIncludesLastItem;

    if (EWSCLI_REQUEST_LOG().isDebugEnabled()) {
        if (resp->isSuccess()) {
            qCDebugNC(EWSCLI_REQUEST_LOG) << QStringLiteral("Got FindItems response (%1 items, last included: %2)")
                                                 .arg(mItems.size())
                                                 .arg(mIncludesLastItem ? QStringLiteral("true") : QStringLiteral("false"));
        } else {
            qCDebug(EWSCLI_REQUEST_LOG) << QStringLiteral("Got FindItems response - %1").arg(resp->responseMessage());
        }
    }

    return true;
}

EwsFindItemResponse::EwsFindItemResponse(QXmlStreamReader &reader)
    : EwsRequest::Response(reader)
{
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsMsgNsUri && reader.namespaceUri() != ewsTypeNsUri) {
            setErrorMsg(QStringLiteral("Unexpected namespace in %1 element: %2").arg(QStringLiteral("ResponseMessage"), reader.namespaceUri().toString()));
            return;
        }

        if (reader.name() == "RootFolder"_L1) {
            if (!parseRootFolder(reader)) {
                return;
            }
        } else if (!readResponseElement(reader)) {
            setErrorMsg(QStringLiteral("Failed to read EWS request - invalid response element."));
            return;
        }
    }
}

bool EwsFindItemResponse::parseRootFolder(QXmlStreamReader &reader)
{
    if (reader.namespaceUri() != ewsMsgNsUri || reader.name() != "RootFolder"_L1) {
        return setErrorMsg(
            QStringLiteral("Failed to read EWS request - expected %1 element (got %2).").arg(QStringLiteral("RootFolder"), reader.qualifiedName().toString()));
    }

    if (!reader.attributes().hasAttribute("TotalItemsInView"_L1) || !reader.attributes().hasAttribute("TotalItemsInView"_L1)) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - missing attributes of %1 element.").arg(QStringLiteral("RootFolder")));
    }
    bool ok;
    QXmlStreamAttributes attrs = reader.attributes();
    mTotalItems = attrs.value("TotalItemsInView"_L1).toUInt(&ok);
    if (!ok) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - failed to read %1 attribute.").arg(QStringLiteral("TotalItemsInView")));
    }
    mIncludesLastItem = attrs.value("IncludesLastItemInRange"_L1) == "true"_L1;

    if (attrs.hasAttribute("IndexedPagingOffset"_L1)) {
        mNextOffset = attrs.value("IndexedPagingOffset"_L1).toInt(&ok);
        if (!ok) {
            return setErrorMsg(QStringLiteral("Failed to read EWS request - failed to read %1 attribute.").arg(QStringLiteral("IndexedPagingOffset")));
        }
    }

    if (attrs.hasAttribute("NumeratorOffset"_L1)) {
        mNextNumerator = attrs.value("NumeratorOffset"_L1).toInt(&ok);
        if (!ok) {
            return setErrorMsg(QStringLiteral("Failed to read EWS request - failed to read %1 attribute.").arg(QStringLiteral("NumeratorOffset")));
        }
    }

    if (attrs.hasAttribute("AbsoluteDenominator"_L1)) {
        mNextDenominator = attrs.value("AbsoluteDenominator"_L1).toInt(&ok);
        if (!ok) {
            return setErrorMsg(QStringLiteral("Failed to read EWS request - failed to read %1 attribute.").arg(QStringLiteral("AbsoluteDenominator")));
        }
    }

    if (!reader.readNextStartElement()) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected a child element in %1 element.").arg(QStringLiteral("RootFolder")));
    }

    if (reader.namespaceUri() != ewsTypeNsUri || reader.name() != "Items"_L1) {
        return setErrorMsg(
            QStringLiteral("Failed to read EWS request - expected %1 element (got %2).").arg(QStringLiteral("Items"), reader.qualifiedName().toString()));
    }

    if (!reader.readNextStartElement()) {
        // An empty Items element means no items.
        reader.skipCurrentElement();
        return true;
    }

    if (reader.namespaceUri() != ewsTypeNsUri) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected child element from types namespace."));
    }

    do {
        EwsItem *item = readItem(reader);
        if (item) {
            mItems.append(*item);
        }
    } while (reader.readNextStartElement());

    // Finish the Items element
    reader.skipCurrentElement();

    // Finish the RootFolder element
    reader.skipCurrentElement();

    return true;
}

EwsItem *EwsFindItemResponse::readItem(QXmlStreamReader &reader)
{
    EwsItem *item = nullptr;
    const QStringView readerName = reader.name();
    if (readerName == "Item"_L1 || readerName == "Message"_L1 || readerName == "CalendarItem"_L1 || readerName == "Contact"_L1
        || readerName == "DistributionList"_L1 || readerName == "MeetingMessage"_L1 || readerName == "MeetingRequest"_L1 || readerName == "MeetingResponse"_L1
        || readerName == "MeetingCancellation"_L1 || readerName == "Task"_L1) {
        qCDebug(EWSCLI_LOG).noquote() << QStringLiteral("Processing %1").arg(readerName.toString());
        item = new EwsItem(reader);
        if (!item->isValid()) {
            setErrorMsg(QStringLiteral("Failed to read EWS request - invalid %1 element.").arg(readerName.toString()));
            delete item;
            return nullptr;
        }
    } else {
        qCWarning(EWSCLI_LOG).noquote() << QStringLiteral("Unsupported folder type %1").arg(readerName.toString());
        reader.skipCurrentElement();
    }

    return item;
}

#include "moc_ewsfinditemrequest.cpp"
