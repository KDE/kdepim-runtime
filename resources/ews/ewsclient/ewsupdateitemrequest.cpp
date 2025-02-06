/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsupdateitemrequest.h"
#include "ewsclient_debug.h"

using namespace Qt::StringLiterals;

static constexpr auto conflictResolutionNames = std::to_array<QLatin1StringView>({
    "NeverOverwrite"_L1,
    "AutoResolve"_L1,
    "AlwaysOverwrite"_L1,
});

static constexpr auto messageDispositionNames = std::to_array<QLatin1StringView>({
    "SaveOnly"_L1,
    "SendOnly"_L1,
    "SendAndSaveCopy"_L1,
});

static constexpr auto meetingDispositionNames = std::to_array<QLatin1StringView>({
    "SendToNone"_L1,
    "SendOnlyToAll"_L1,
    "SendOnlyToChanged"_L1,
    "SendToAllAndSaveCopy"_L1,
    "SendToChangedAndSaveCopy"_L1,
});

static constexpr auto updateTypeElementNames = std::to_array<QLatin1StringView>({
    "AppendToItemField"_L1,
    "SetItemField"_L1,
    "DeleteItemField"_L1,
});

EwsUpdateItemRequest::EwsUpdateItemRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent)
    , mMessageDisp(EwsDispSaveOnly)
    , mConflictResol(EwsResolAlwaysOverwrite)
    , mMeetingDisp(EwsMeetingDispUnspecified)
{
}

EwsUpdateItemRequest::~EwsUpdateItemRequest() = default;

void EwsUpdateItemRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("UpdateItem"));

    writer.writeAttribute(QStringLiteral("ConflictResolution"), conflictResolutionNames[mConflictResol]);

    writer.writeAttribute(QStringLiteral("MessageDisposition"), messageDispositionNames[mMessageDisp]);

    if (mMeetingDisp != EwsMeetingDispUnspecified) {
        writer.writeAttribute(QStringLiteral("SendMeetingInvitationsOrCancellations"), meetingDispositionNames[mMeetingDisp]);
    }

    if (mSavedFolderId.type() != EwsId::Unspecified) {
        writer.writeStartElement(ewsMsgNsUri, QStringLiteral("SavedItemFolderId"));
        mSavedFolderId.writeFolderIds(writer);
        writer.writeEndElement();
    }

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("ItemChanges"));
    for (const ItemChange &ch : std::as_const(mChanges)) {
        ch.write(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNC(EWSCLI_REQUEST_LOG) << QStringLiteral("Starting UpdateItem request (%1 changes)").arg(mChanges.size());

    qCDebug(EWSCLI_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsUpdateItemRequest::parseResult(QXmlStreamReader &reader)
{
    mResponses.reserve(mChanges.size());
    return parseResponseMessage(reader, QStringLiteral("UpdateItem"), [this](QXmlStreamReader &reader) {
        return parseItemsResponse(reader);
    });
}

bool EwsUpdateItemRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    if (EWSCLI_REQUEST_LOG().isDebugEnabled()) {
        if (resp.isSuccess()) {
            qCDebugNC(EWSCLI_REQUEST_LOG) << "Got UpdateItem response - OK";
        } else {
            qCDebugNC(EWSCLI_REQUEST_LOG) << u"Got UpdateItem response - %1"_s.arg(resp.responseMessage());
        }
    }

    mResponses.append(resp);
    return true;
}

EwsUpdateItemRequest::Response::Response(QXmlStreamReader &reader)
    : EwsRequest::Response(reader)
    , mConflictCount(0)
{
    if (mClass == EwsResponseParseError) {
        return;
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsMsgNsUri && reader.namespaceUri() != ewsTypeNsUri) {
            setErrorMsg(u"Unexpected namespace in %1 element: %2"_s.arg(u"ResponseMessage"_s, reader.namespaceUri().toString()));
            return;
        }

        if (reader.name() == "Items"_L1) {
            if (reader.readNextStartElement()) {
                EwsItem item(reader);
                if (!item.isValid()) {
                    return;
                }
                mId = item[EwsItemFieldItemId].value<EwsId>();
            }

            // Finish the Items element.
            reader.skipCurrentElement();
        } else if (reader.name() == "ConflictResults"_L1) {
            if (!reader.readNextStartElement()) {
                setErrorMsg(QStringLiteral("Failed to read EWS request - expected a %1 element inside %2 element.")
                                .arg(QStringLiteral("Value"), QStringLiteral("ConflictResults")));
                return;
            }

            if (reader.name() != "Count"_L1) {
                setErrorMsg(u"Failed to read EWS request - expected a %1 element inside %2 element."_s.arg(u"Count"_s, u"ConflictResults"_s));
                return;
            }

            bool ok;
            mConflictCount = reader.readElementText().toUInt(&ok);

            if (!ok) {
                setErrorMsg(u"Failed to read EWS request - invalid %1 element."_s.arg(u"ConflictResults/Value"_s));
            }
            // Finish the Value element.
            reader.skipCurrentElement();
        } else if (!readResponseElement(reader)) {
            setErrorMsg(u"Failed to read EWS request - invalid response element %1."_s.arg(reader.name().toString()));
            return;
        }
    }
}

bool EwsUpdateItemRequest::Update::write(QXmlStreamWriter &writer, EwsItemType itemType) const
{
    bool retVal = true;

    writer.writeStartElement(ewsTypeNsUri, updateTypeElementNames[mType]);

    mField.write(writer);

    if (mType != Delete) {
        writer.writeStartElement(ewsTypeNsUri, ewsItemTypeNames[itemType]);
        retVal = mField.writeWithValue(writer, mValue);
        writer.writeEndElement();
    }

    writer.writeEndElement();

    return retVal;
}

bool EwsUpdateItemRequest::ItemChange::write(QXmlStreamWriter &writer) const
{
    bool retVal = true;

    writer.writeStartElement(ewsTypeNsUri, u"ItemChange"_s);

    mId.writeItemIds(writer);

    writer.writeStartElement(ewsTypeNsUri, u"Updates"_s);

    for (const QSharedPointer<const Update> &upd : std::as_const(mUpdates)) {
        if (!upd->write(writer, mType)) {
            retVal = false;
            break;
        }
    }

    writer.writeEndElement();

    writer.writeEndElement();

    return retVal;
}

#include "moc_ewsupdateitemrequest.cpp"
