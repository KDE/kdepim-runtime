/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsdeletefolderrequest.h"
#include "ewsclient_debug.h"

using namespace Qt::StringLiterals;

static constexpr auto deleteTypes = std::to_array<QLatin1StringView>({
    "HardDelete"_L1,
    "SoftDelete"_L1,
    "MoveToDeletedItems"_L1,
});

EwsDeleteFolderRequest::EwsDeleteFolderRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent)
    , mType(SoftDelete)
{
}

EwsDeleteFolderRequest::~EwsDeleteFolderRequest()
{
}

void EwsDeleteFolderRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, "DeleteFolder"_L1);

    writer.writeAttribute("DeleteType"_L1, deleteTypes[mType]);

    writer.writeStartElement(ewsMsgNsUri, "FolderIds"_L1);
    for (const EwsId &id : std::as_const(mIds)) {
        id.writeFolderIds(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNC(EWSCLI_REQUEST_LOG) << QStringLiteral("Starting DeleteFolder request (%1 folders)").arg(mIds.size());

    qCDebug(EWSCLI_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsDeleteFolderRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("DeleteFolder"), [this](QXmlStreamReader &reader) {
        return parseItemsResponse(reader);
    });
}

bool EwsDeleteFolderRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    if (EWSCLI_REQUEST_LOG().isDebugEnabled()) {
        if (resp.isSuccess()) {
            qCDebug(EWSCLI_REQUEST_LOG) << QStringLiteral("Got DeleteFolder response - OK");
        } else {
            qCDebug(EWSCLI_REQUEST_LOG) << QStringLiteral("Got DeleteFolder response - %1").arg(resp.responseMessage());
        }
    }

    mResponses.append(resp);
    return true;
}

EwsDeleteFolderRequest::Response::Response(QXmlStreamReader &reader)
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

#include "moc_ewsdeletefolderrequest.cpp"
