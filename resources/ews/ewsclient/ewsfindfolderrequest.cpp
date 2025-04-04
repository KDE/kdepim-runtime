/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsfindfolderrequest.h"

#include <memory>

#include <QXmlStreamWriter>

#include "ewsclient_debug.h"

using namespace Qt::StringLiterals;

static constexpr auto traversalTypeNames = std::to_array({
    "Shallow"_L1,
    "Deep"_L1,
    "SoftDeleted"_L1,
});

class EwsFindFolderResponse : public EwsRequest::Response
{
public:
    EwsFindFolderResponse(QXmlStreamReader &reader);
    bool parseRootFolder(QXmlStreamReader &reader);
    EwsFolder *readFolder(QXmlStreamReader &reader);
    unsigned readChildFolders(EwsFolder &parent, unsigned count, QXmlStreamReader &reader);

    QList<EwsFolder> mFolders;
};

EwsFindFolderRequest::EwsFindFolderRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent)
    , mTraversal(EwsTraversalDeep)
{
}

EwsFindFolderRequest::~EwsFindFolderRequest()
{
}

void EwsFindFolderRequest::setParentFolderId(const EwsId &id)
{
    mParentId = id;
}

void EwsFindFolderRequest::setFolderShape(const EwsFolderShape &shape)
{
    mShape = shape;
}

void EwsFindFolderRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, "FindFolder"_L1);
    writer.writeAttribute("Traversal"_L1, traversalTypeNames[mTraversal]);

    mShape.write(writer);

    writer.writeStartElement(ewsMsgNsUri, "ParentFolderIds"_L1);
    mParentId.writeFolderIds(writer);
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebug(EWSCLI_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsFindFolderRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("FindFolder"), [this](QXmlStreamReader &reader) {
        return parseFoldersResponse(reader);
    });
}

bool EwsFindFolderRequest::parseFoldersResponse(QXmlStreamReader &reader)
{
    auto resp = new EwsFindFolderResponse(reader);
    if (resp->responseClass() == EwsResponseUnknown) {
        return false;
    }

    mFolders = resp->mFolders;

    return true;
}

EwsFindFolderResponse::EwsFindFolderResponse(QXmlStreamReader &reader)
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

bool EwsFindFolderResponse::parseRootFolder(QXmlStreamReader &reader)
{
    if (reader.namespaceUri() != ewsMsgNsUri || reader.name() != "RootFolder"_L1) {
        return setErrorMsg(
            QStringLiteral("Failed to read EWS request - expected %1 element (got %2).").arg(QStringLiteral("RootFolder"), reader.qualifiedName().toString()));
    }

    if (!reader.attributes().hasAttribute("TotalItemsInView"_L1) || !reader.attributes().hasAttribute("TotalItemsInView"_L1)) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - missing attributes of %1 element.").arg(QStringLiteral("RootFolder")));
    }
    bool ok;
    unsigned totalItems = reader.attributes().value("TotalItemsInView"_L1).toUInt(&ok);
    if (!ok) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - failed to read %1 attribute.").arg(QStringLiteral("TotalItemsInView")));
    }

    if (!reader.readNextStartElement()) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected a child element in %1 element.").arg(QStringLiteral("RootFolder")));
    }

    if (reader.namespaceUri() != ewsTypeNsUri || reader.name() != "Folders"_L1) {
        return setErrorMsg(
            QStringLiteral("Failed to read EWS request - expected %1 element (got %2).").arg(QStringLiteral("Folders"), reader.qualifiedName().toString()));
    }

    if (!reader.readNextStartElement()) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected a child element in %1 element.").arg(QStringLiteral("Folders")));
    }

    if (reader.namespaceUri() != ewsTypeNsUri) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected child element from types namespace."));
    }

    unsigned i = 0;
    for (i = 0; i < totalItems; ++i) {
        EwsFolder *folder = readFolder(reader);
        reader.readNextStartElement();
        if (folder) {
            bool okInt;
            int childCount = (*folder)[EwsFolderFieldChildFolderCount].toUInt(&okInt);
            if (childCount > 0) {
                unsigned readCount = readChildFolders(*folder, childCount, reader);
                if (readCount == 0) {
                    return false;
                }
                i += readCount;
            }
            mFolders.append(*folder);
            delete folder;
        }
    }

    // Finish the Folders element
    reader.skipCurrentElement();

    // Finish the RootFolder element
    reader.skipCurrentElement();

    return true;
}

EwsFolder *EwsFindFolderResponse::readFolder(QXmlStreamReader &reader)
{
    EwsFolder *folder = nullptr;
    const QStringView readerName = reader.name();
    if (readerName == "Folder"_L1 || readerName == "CalendarFolder"_L1 || readerName == "ContactsFolder"_L1 || readerName == "TasksFolder"_L1
        || readerName == "SearchFolder"_L1) {
        folder = new EwsFolder(reader);
        if (!folder->isValid()) {
            setErrorMsg(QStringLiteral("Failed to read EWS request - invalid %1 element.").arg(QStringLiteral("Folder")));
            delete folder;
            return nullptr;
        }
        QVariant dn = (*folder)[EwsFolderFieldDisplayName];
        if (!dn.isNull()) {
            EwsClient::folderHash[(*folder)[EwsFolderFieldFolderId].value<EwsId>().id()] = dn.toString();
        }
    } else {
        qCWarning(EWSCLI_LOG).noquote() << QStringLiteral("Unsupported folder type %1").arg(readerName.toString());
        reader.skipCurrentElement();
    }

    return folder;
}

unsigned EwsFindFolderResponse::readChildFolders(EwsFolder &parent, unsigned count, QXmlStreamReader &reader)
{
    unsigned readCount = 0;
    for (unsigned i = 0; i < count; ++i) {
        EwsFolder *folder = readFolder(reader);
        reader.readNextStartElement();
        if (folder) {
            bool ok;
            int childCount = (*folder)[EwsFolderFieldChildFolderCount].toUInt(&ok);
            if (ok && childCount > 0) {
                unsigned readCount2 = readChildFolders(*folder, childCount, reader);
                if (readCount2 == 0) {
                    return false;
                }
                readCount += readCount2;
            }
            parent.addChild(*folder);
        }
        ++readCount;
    }
    return readCount;
}

#include "moc_ewsfindfolderrequest.cpp"
