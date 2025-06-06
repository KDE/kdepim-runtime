/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsdeleteitemrequest.h"
#include "ewsclient_debug.h"

using namespace Qt::StringLiterals;

static const constexpr auto deleteTypes = std::to_array<QLatin1StringView>({
    "HardDelete"_L1,
    "SoftDelete"_L1,
    "MoveToDeletedItems"_L1,
});

EwsDeleteItemRequest::EwsDeleteItemRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent)
    , mType(SoftDelete)
{
}

EwsDeleteItemRequest::~EwsDeleteItemRequest()
{
}

void EwsDeleteItemRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, "DeleteItem"_L1);

    writer.writeAttribute("DeleteType"_L1, deleteTypes[mType]);

    writer.writeStartElement(ewsMsgNsUri, "ItemIds"_L1);
    for (const EwsId &id : std::as_const(mIds)) {
        id.writeItemIds(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNCS(EWSCLI_REQUEST_LOG) << QStringLiteral("Starting DeleteItem request (") << mIds << ")";

    qCDebug(EWSCLI_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsDeleteItemRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("DeleteItem"), [this](QXmlStreamReader &reader) {
        return parseItemsResponse(reader);
    });
}

bool EwsDeleteItemRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    if (EWSCLI_REQUEST_LOG().isDebugEnabled()) {
        if (resp.isSuccess()) {
            qCDebugNC(EWSCLI_REQUEST_LOG) << QStringLiteral("Got DeleteItem response - OK, deleted items") << mIds;
        } else {
            qCDebugNC(EWSCLI_REQUEST_LOG) << QStringLiteral("Got DeleteItem response - %1").arg(resp.responseMessage());
        }
    }

    mResponses.append(resp);
    return true;
}

EwsDeleteItemRequest::Response::Response(QXmlStreamReader &reader)
    : EwsRequest::Response::Response(reader)
{
    if (mClass == EwsResponseParseError) {
        return;
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsMsgNsUri && reader.namespaceUri() != ewsTypeNsUri) {
            setErrorMsg(QStringLiteral("Unexpected namespace in %1 element: %2").arg(QStringLiteral("ResponseMessage"), reader.namespaceUri().toString()));
            return;
        }

        if (!readResponseElement(reader)) {
            setErrorMsg(QStringLiteral("Failed to read EWS request - invalid response element."));
            return;
        }
    }
}

#include "moc_ewsdeleteitemrequest.cpp"
