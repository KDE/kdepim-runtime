/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsfolder.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "ewsclient_debug.h"
#include "ewseffectiverights.h"
#include "ewsitembase_p.h"
#include "ewsxml.h"

#define D_PTR EwsFolderPrivate *d = reinterpret_cast<EwsFolderPrivate *>(this->d.data());
#define D_CPTR const EwsFolderPrivate *d = reinterpret_cast<const EwsFolderPrivate *>(this->d.data());

using namespace Qt::StringLiterals;

static constexpr const auto folderTypeNames = std::to_array({
    "Folder"_L1,
    "CalendarFolder"_L1,
    "ContactsFolder"_L1,
    "SearchFolder"_L1,
    "TasksFolder"_L1,
});

class EwsFolderPrivate : public EwsItemBasePrivate
{
public:
    typedef EwsXml<EwsItemFields> XmlProc;

    EwsFolderPrivate();
    EwsFolderPrivate(const EwsItemBasePrivate &other);
    EwsItemBasePrivate *clone() const override
    {
        return new EwsFolderPrivate(*this);
    }

    static bool effectiveRightsReader(QXmlStreamReader &reader, QVariant &val);

    EwsFolderType mType;
    EwsFolder *mParent;
    QList<EwsFolder> mChildren;
    static const XmlProc mStaticEwsXml;
    XmlProc mEwsXml;
};

typedef EwsXml<EwsItemFields> ItemFieldsReader;

static const QList<EwsFolderPrivate::XmlProc::Item> ewsFolderItems = {
    {EwsFolderFieldFolderId, "FolderId"_L1, &ewsXmlIdReader, &ewsXmlIdWriter},
    {EwsFolderFieldParentFolderId, "ParentFolderId"_L1, &ewsXmlIdReader},
    {EwsFolderFieldFolderClass, "FolderClass"_L1, &ewsXmlTextReader, &ewsXmlTextWriter},
    {EwsFolderFieldDisplayName, "DisplayName"_L1, &ewsXmlTextReader, &ewsXmlTextWriter},
    {EwsFolderFieldTotalCount, "TotalCount"_L1, &ewsXmlUIntReader, &ewsXmlUIntWriter},
    {EwsFolderFieldChildFolderCount, "ChildFolderCount"_L1, &ewsXmlUIntReader},
    {EwsItemFieldExtendedProperties, "ExtendedProperty"_L1, &EwsItemBasePrivate::extendedPropertyReader, &EwsItemBasePrivate::extendedPropertyWriter},
    {EwsFolderFieldUnreadCount, "UnreadCount"_L1, &ewsXmlUIntReader},
    {EwsFolderPrivate::XmlProc::Ignore, "SearchParameters"_L1},
    {EwsFolderFieldEffectiveRights, "EffectiveRights"_L1, &EwsFolderPrivate::effectiveRightsReader},
    {EwsFolderPrivate::XmlProc::Ignore, "ManagedFolderInformation"_L1},
};

const EwsFolderPrivate::XmlProc EwsFolderPrivate::mStaticEwsXml(ewsFolderItems);

EwsFolderPrivate::EwsFolderPrivate()
    : EwsItemBasePrivate()
    , mType(EwsFolderTypeUnknown)
    , mParent(nullptr)
    , mEwsXml(mStaticEwsXml)
{
}

bool EwsFolderPrivate::effectiveRightsReader(QXmlStreamReader &reader, QVariant &val)
{
    EwsEffectiveRights rights(reader);
    if (!rights.isValid()) {
        reader.skipCurrentElement();
        return false;
    }
    val = QVariant::fromValue<EwsEffectiveRights>(rights);
    return true;
}

EwsFolder::EwsFolder()
    : EwsItemBase(QSharedDataPointer<EwsItemBasePrivate>(new EwsFolderPrivate()))
{
}

EwsFolder::EwsFolder(QXmlStreamReader &reader)
    : EwsItemBase(QSharedDataPointer<EwsItemBasePrivate>(new EwsFolderPrivate()))
{
    D_PTR

    // Check what item type are we
    uint i = 0;
    d->mType = EwsFolderTypeUnknown;
    for (const auto &name : std::as_const(folderTypeNames)) {
        if (name == reader.name()) {
            d->mType = static_cast<EwsFolderType>(i);
            break;
        }
        i++;
    }
    if (d->mType == EwsFolderTypeUnknown) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unknown folder type %1").arg(reader.name().toString());
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << "Unexpected namespace in folder element:" << reader.namespaceUri();
            return;
        }

        if (!readBaseFolderElement(reader)) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Invalid folder child: %1").arg(reader.qualifiedName().toString());
            return;
        }
    }
    d->mValid = true;
}

EwsFolder::EwsFolder(const EwsFolder &other)
    : EwsItemBase(other.d)
{
    qRegisterMetaType<EwsEffectiveRights>();
}

EwsFolder::EwsFolder(EwsFolder &&other)
    : EwsItemBase(other.d)
{
}

EwsFolder::~EwsFolder()
{
}

EwsFolderType EwsFolder::type() const
{
    D_CPTR
    return d->mType;
}

void EwsFolder::setType(EwsFolderType type)
{
    D_PTR
    d->mType = type;
}

bool EwsFolder::readBaseFolderElement(QXmlStreamReader &reader)
{
    D_PTR

    if (!d->mEwsXml.readItem(reader, QStringLiteral("Folder"), ewsTypeNsUri)) {
        return false;
    }

    d->mFields = d->mEwsXml.values();

    return true;
}

const QList<EwsFolder> EwsFolder::childFolders() const
{
    D_CPTR
    return d->mChildren;
}

void EwsFolder::addChild(EwsFolder &child)
{
    D_PTR

    if (child.parentFolder() != nullptr) {
        qCWarning(EWSCLI_LOG).noquote()
            << QStringLiteral("Attempt to add child folder which already has a parent (child: %1)").arg(child[EwsFolderFieldFolderId].value<EwsId>().id());
    }
    d->mChildren.append(child);
    child.setParentFolder(this);
}

EwsFolder *EwsFolder::parentFolder() const
{
    D_CPTR
    return d->mParent;
}

void EwsFolder::setParentFolder(EwsFolder *parent)
{
    D_PTR
    d->mParent = parent;
}

EwsFolder &EwsFolder::operator=(const EwsFolder &other)
{
    d = other.d;
    return *this;
}

EwsFolder &EwsFolder::operator=(EwsFolder &&other)
{
    d = std::move(other.d);
    return *this;
}

bool EwsFolder::write(QXmlStreamWriter &writer) const
{
    D_CPTR

    writer.writeStartElement(ewsTypeNsUri, folderTypeNames[d->mType]);

    bool status = d->mEwsXml.writeItems(writer, folderTypeNames[d->mType], ewsTypeNsUri, d->mFields);

    writer.writeEndElement();

    return status;
}
