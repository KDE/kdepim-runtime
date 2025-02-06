/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsupdatefolderrequest.h"
#include "ewsclient_debug.h"

using namespace Qt::StringLiterals;

static constexpr auto updateTypeElementNames = std::to_array<QLatin1StringView>({
    "AppendToFolderField"_L1,
    "SetFolderField"_L1,
    "DeleteFolderField"_L1,
});

static constexpr auto folderTypeNames = std::to_array<QLatin1StringView>({
    "Folder"_L1,
    "CalendarFolder"_L1,
    "ContactsFolder"_L1,
    "SearchFolder"_L1,
    "TasksFolder"_L1,
});

EwsUpdateFolderRequest::EwsUpdateFolderRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent)
{
}

EwsUpdateFolderRequest::~EwsUpdateFolderRequest() = default;

void EwsUpdateFolderRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, u"UpdateFolder"_s);

    writer.writeStartElement(ewsMsgNsUri, u"FolderChanges"_s);
    for (const FolderChange &ch : std::as_const(mChanges)) {
        ch.write(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNC(EWSCLI_REQUEST_LOG) << u"Starting UpdateFolder request (%1 changes)"_s.arg(mChanges.size());

    qCDebug(EWSCLI_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsUpdateFolderRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, u"UpdateFolder"_s, [this](QXmlStreamReader &reader) {
        return parseItemsResponse(reader);
    });
}

bool EwsUpdateFolderRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    if (EWSCLI_REQUEST_LOG().isDebugEnabled()) {
        if (resp.isSuccess()) {
            qCDebugNC(EWSCLI_REQUEST_LOG) << u"Got UpdateFolder response - OK"_s;
        } else {
            qCDebugNC(EWSCLI_REQUEST_LOG) << u"Got UpdateFolder response - %1"_s.arg(resp.responseMessage());
        }
    }

    mResponses.append(resp);
    return true;
}

EwsUpdateFolderRequest::Response::Response(QXmlStreamReader &reader)
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
            }

            // Finish the Folders element.
            reader.skipCurrentElement();
        } else if (!readResponseElement(reader)) {
            setErrorMsg(u"Failed to read EWS request - invalid response element."_s);
            return;
        }
    }
}

bool EwsUpdateFolderRequest::Update::write(QXmlStreamWriter &writer, EwsFolderType folderType) const
{
    bool retVal = true;

    writer.writeStartElement(ewsTypeNsUri, updateTypeElementNames[mType]);

    mField.write(writer);

    if (mType != Delete) {
        writer.writeStartElement(ewsTypeNsUri, folderTypeNames[folderType]);
        retVal = mField.writeWithValue(writer, mValue);
        writer.writeEndElement();
    }

    writer.writeEndElement();

    return retVal;
}

bool EwsUpdateFolderRequest::FolderChange::write(QXmlStreamWriter &writer) const
{
    bool retVal = true;

    writer.writeStartElement(ewsTypeNsUri, u"FolderChange"_s);

    mId.writeFolderIds(writer);

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

#include "moc_ewsupdatefolderrequest.cpp"
