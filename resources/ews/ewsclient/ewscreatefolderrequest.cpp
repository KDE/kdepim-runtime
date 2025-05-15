/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewscreatefolderrequest.h"

#include "ewsclient_debug.h"

using namespace Qt::StringLiterals;

EwsCreateFolderRequest::EwsCreateFolderRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent)
{
}

EwsCreateFolderRequest::~EwsCreateFolderRequest() = default;

void EwsCreateFolderRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, u"CreateFolder"_s);

    writer.writeStartElement(ewsMsgNsUri, u"ParentFolderId"_s);
    mParentFolderId.writeFolderIds(writer);
    writer.writeEndElement();

    writer.writeStartElement(ewsMsgNsUri, u"Folders"_s);
    for (const EwsFolder &folder : std::as_const(mFolders)) {
        folder.write(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNC(EWSCLI_REQUEST_LOG) << u"Starting CreateFolder request (%1 folders, parent %2)"_s.arg(mFolders.size()).arg(mParentFolderId.id());

    qCDebug(EWSCLI_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsCreateFolderRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, u"CreateFolder"_s, [this](QXmlStreamReader &reader) {
        return parseItemsResponse(reader);
    });
}

bool EwsCreateFolderRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    if (EWSCLI_REQUEST_LOG().isDebugEnabled()) {
        if (resp.isSuccess()) {
            qCDebug(EWSCLI_REQUEST_LOG) << "Got CreateFolder response - OK";
        } else {
            qCDebug(EWSCLI_REQUEST_LOG) << u"Got CreateFolder response - %1"_s.arg(resp.responseMessage());
        }
    }
    mResponses.append(resp);
    return true;
}

EwsCreateFolderRequest::Response::Response(QXmlStreamReader &reader)
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

        if (reader.name() == QLatin1StringView("Folders")) {
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

#include "moc_ewscreatefolderrequest.cpp"
