/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsmovefolderrequest.h"
#include "ewsclient_debug.h"

using namespace Qt::StringLiterals;

EwsMoveFolderRequest::EwsMoveFolderRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent)
{
}

EwsMoveFolderRequest::~EwsMoveFolderRequest() = default;

void EwsMoveFolderRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, "MoveFolder"_L1);

    writer.writeStartElement(ewsMsgNsUri, "ToFolderId"_L1);
    mDestFolderId.writeFolderIds(writer);
    writer.writeEndElement();

    writer.writeStartElement(ewsMsgNsUri, "FolderIds"_L1);
    for (const EwsId &id : std::as_const(mIds)) {
        id.writeFolderIds(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNC(EWSCLI_REQUEST_LOG) << u"Starting MoveFolder request (%1 folders, to %2"_s.arg(mIds.size()).arg(mDestFolderId.id());

    qCDebug(EWSCLI_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsMoveFolderRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, u"MoveFolder"_s, [this](QXmlStreamReader &reader) {
        return parseItemsResponse(reader);
    });
}

bool EwsMoveFolderRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    if (EWSCLI_REQUEST_LOG().isDebugEnabled()) {
        if (resp.isSuccess()) {
            qCDebug(EWSCLI_REQUEST_LOG) << u"Got MoveFolder response - OK"_s;
        } else {
            qCDebug(EWSCLI_REQUEST_LOG) << u"Got MoveFolder response - %1"_s.arg(resp.responseMessage());
        }
    }
    mResponses.append(resp);
    return true;
}

EwsMoveFolderRequest::Response::Response(QXmlStreamReader &reader)
    : EwsRequest::Response(reader)
{
    if (mClass == EwsResponseParseError) {
        return;
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsMsgNsUri && reader.namespaceUri() != ewsTypeNsUri) {
            setErrorMsg(u"Unexpected namespace in %1 element: %2"_s.arg(u"ResponseMessage"_s, reader.namespaceUri().toString()));
            return;
        }

        if (reader.name() == "Folders"_L1) {
            if (reader.readNextStartElement()) {
                EwsFolder folder(reader);
                if (!folder.isValid()) {
                    return;
                }
                mId = folder[EwsFolderFieldFolderId].value<EwsId>();

                // Finish the Folders element.
                reader.skipCurrentElement();
            }
        } else if (!readResponseElement(reader)) {
            setErrorMsg(u"Failed to read EWS request - invalid response element."_s);
            return;
        }
    }
}

#include "moc_ewsmovefolderrequest.cpp"
