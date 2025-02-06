/*
    SPDX-FileCopyrightText: 2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsattachment.h"

#include <QBitArray>

#include "ewsclient_debug.h"
#include "ewsxml.h"

using namespace Qt::StringLiterals;

class EwsAttachmentPrivate : public QSharedData
{
public:
    EwsAttachmentPrivate();
    ~EwsAttachmentPrivate();

    enum Field {
        Id = 0,
        Name,
        ContentType,
        ContentId,
        ContentLocation,
        Size,
        LastModifiedTime,
        IsInline,
        IsContactPhoto,
        Content,
        Item,
        NumFields,
    };

    EwsAttachment::Type mType;
    QString mId;
    QString mName;
    QString mContentType;
    QString mContentId;
    QString mContentLocation;
    long mSize = 0;
    QDateTime mLastModifiedTime;
    bool mIsInline = false;
    bool mIsContactPhoto = false;
    QByteArray mContent;
    EwsItem mItem;
    bool mValid = false;
    QBitArray mValidFields;
};

EwsAttachmentPrivate::EwsAttachmentPrivate()
    : mType(EwsAttachment::UnknownAttachment)
    , mValidFields(NumFields)
{
}

EwsAttachmentPrivate::~EwsAttachmentPrivate() = default;

EwsAttachment::EwsAttachment()
    : d(new EwsAttachmentPrivate())
{
}

EwsAttachment::~EwsAttachment() = default;

EwsAttachment::EwsAttachment(QXmlStreamReader &reader)
    : d(new EwsAttachmentPrivate())
{
    bool ok = true;

    if (reader.namespaceUri() != ewsTypeNsUri) {
        qCWarningNC(EWSCLI_LOG) << "Unexpected namespace in Attachment element:" << reader.namespaceUri();
        reader.skipCurrentElement();
        return;
    }
    const QStringView readerName = reader.name();
    if (readerName == "ItemAttachment"_L1) {
        d->mType = ItemAttachment;
    } else if (readerName == "FileAttachment"_L1) {
        d->mType = FileAttachment;
    } else if (readerName == "ReferenceAttachment"_L1) {
        d->mType = ReferenceAttachment;
    } else {
        qCWarningNC(EWSCLI_LOG) << "Unknown attachment type" << readerName.toString();
        ok = false;
    }

    // Skip this attachment type as it's not clearly documented.
    if (d->mType == ReferenceAttachment) {
        qCWarningNC(EWSCLI_LOG) << "Attachment type ReferenceAttachment not fully supported";
        reader.skipCurrentElement();
        d->mValid = true;
        return;
    }

    while (ok && reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << "Unexpected namespace in Attachment element:" << reader.namespaceUri();
            reader.skipCurrentElement();
            ok = false;
            break;
        }

        const QString elmName = reader.name().toString();
        if (elmName == "AttachmentId"_L1) {
            QXmlStreamAttributes attrs = reader.attributes();
            if (!attrs.hasAttribute(u"Id"_s)) {
                qCWarningNC(EWSCLI_LOG) << "Failed to read Attachment element - missing Id in AttachmentId element.";
                reader.skipCurrentElement();
                ok = false;
            } else {
                d->mId = attrs.value(u"Id"_s).toString();
                d->mValidFields.setBit(EwsAttachmentPrivate::Id);
            }
            reader.skipCurrentElement();
        } else if (elmName == "Name"_L1) {
            d->mName = readXmlElementValue<QString>(reader, ok, u"Attachment"_s);
            d->mValidFields.setBit(EwsAttachmentPrivate::Name, ok);
        } else if (elmName == "ContentType"_L1) {
            d->mContentType = readXmlElementValue<QString>(reader, ok, u"Attachment"_s);
            d->mValidFields.setBit(EwsAttachmentPrivate::ContentType, ok);
        } else if (elmName == "ContentId"_L1) {
            d->mContentId = readXmlElementValue<QString>(reader, ok, u"Attachment"_s);
            d->mValidFields.setBit(EwsAttachmentPrivate::ContentId, ok);
        } else if (elmName == "ContentLocation"_L1) {
            d->mContentLocation = readXmlElementValue<QString>(reader, ok, u"Attachment"_s);
            d->mValidFields.setBit(EwsAttachmentPrivate::ContentLocation, ok);
        } else if (elmName == "AttachmentOriginalUrl"_L1) {
            // Ignore
            reader.skipCurrentElement();
        } else if (elmName == "Size"_L1) {
            d->mSize = readXmlElementValue<long>(reader, ok, u"Attachment"_s);
            d->mValidFields.setBit(EwsAttachmentPrivate::Size, ok);
        } else if (elmName == "LastModifiedTime"_L1) {
            d->mLastModifiedTime = readXmlElementValue<QDateTime>(reader, ok, u"Attachment"_s);
            d->mValidFields.setBit(EwsAttachmentPrivate::LastModifiedTime, ok);
        } else if (elmName == "IsInline"_L1) {
            d->mIsInline = readXmlElementValue<bool>(reader, ok, u"Attachment"_s);
            d->mValidFields.setBit(EwsAttachmentPrivate::IsInline, ok);
        } else if (d->mType == FileAttachment && elmName == "IsContactPhoto"_L1) {
            d->mIsContactPhoto = readXmlElementValue<bool>(reader, ok, u"Attachment"_s);
            d->mValidFields.setBit(EwsAttachmentPrivate::IsContactPhoto, ok);
        } else if (d->mType == FileAttachment && elmName == "Content"_L1) {
            d->mContent = readXmlElementValue<QByteArray>(reader, ok, u"Attachment"_s);
            d->mValidFields.setBit(EwsAttachmentPrivate::Content, ok);
        } else if (d->mType == ItemAttachment
                   && (elmName == "Item"_L1 || elmName == "Message"_L1 || elmName == "CalendarItem"_L1 || elmName == "Contact"_L1
                       || elmName == "MeetingMessage"_L1 || elmName == "MeetingRequest"_L1 || elmName == "MeetingResponse"_L1
                       || elmName == "MeetingCancellation"_L1 || elmName == "Task"_L1)) {
            d->mItem = EwsItem(reader);
            if (!d->mItem.isValid()) {
                qCWarningNC(EWSCLI_LOG) << "Failed to read attachment element - invalid Item element.";
                reader.skipCurrentElement();
                ok = false;
            } else {
                d->mValidFields.setBit(EwsAttachmentPrivate::Item);
            }
        } else {
            qCWarningNC(EWSCLI_LOG) << u"Failed to read attachment element - unknown %1 element."_s.arg(elmName);
            reader.skipCurrentElement();
            ok = false;
        }
    }

    if (!ok) {
        reader.skipCurrentElement();
    }
    d->mValid = ok;
}

EwsAttachment::EwsAttachment(const EwsAttachment &other)
    : d(other.d)
{
}

EwsAttachment::EwsAttachment(EwsAttachment &&other)
    : d(std::move(other.d))
{
}

EwsAttachment &EwsAttachment::operator=(EwsAttachment &&other)
{
    d = std::move(other.d);
    return *this;
}

EwsAttachment &EwsAttachment::operator=(const EwsAttachment &other)
{
    d = other.d;
    return *this;
}

void EwsAttachment::write(QXmlStreamWriter &writer) const
{
    QString elmName;
    switch (d->mType) {
    case ItemAttachment:
        elmName = u"ItemAttachment"_s;
        break;
    case FileAttachment:
        elmName = u"FileAttachment"_s;
        break;
    case ReferenceAttachment:
        elmName = u"ReferenceAttachment"_s;
        break;
    default:
        qCWarning(EWSCLI_LOG) << "Failed to write attachment element - invalid attachment type.";
        return;
    }
    writer.writeStartElement(ewsTypeNsUri, elmName);

    if (d->mType == ReferenceAttachment) {
        qCWarningNC(EWSCLI_LOG) << "Attachment type ReferenceAttachment not fully supported";
        writer.writeEndElement();
        return;
    }

    if (d->mValidFields[EwsAttachmentPrivate::Id]) {
        writer.writeStartElement(ewsTypeNsUri, u"AttachmentId"_s);
        writer.writeAttribute(u"Id"_s, d->mId);
        writer.writeEndElement();
    }
    if (d->mValidFields[EwsAttachmentPrivate::Name]) {
        writer.writeTextElement(ewsTypeNsUri, u"Name"_s, d->mName);
    }
    if (d->mValidFields[EwsAttachmentPrivate::ContentType]) {
        writer.writeTextElement(ewsTypeNsUri, u"ContentType"_s, d->mContentType);
    }
    if (d->mValidFields[EwsAttachmentPrivate::ContentId]) {
        writer.writeTextElement(ewsTypeNsUri, u"ContentId"_s, d->mContentId);
    }
    if (d->mValidFields[EwsAttachmentPrivate::ContentLocation]) {
        writer.writeTextElement(ewsTypeNsUri, u"ContentLocation"_s, d->mContentLocation);
    }
    if (d->mValidFields[EwsAttachmentPrivate::Size]) {
        writer.writeTextElement(ewsTypeNsUri, u"Size"_s, QString::number(d->mSize));
    }
    if (d->mValidFields[EwsAttachmentPrivate::LastModifiedTime]) {
        writer.writeTextElement(ewsTypeNsUri, u"LastModifiedTime"_s, d->mLastModifiedTime.toString(Qt::ISODate));
    }
    if (d->mValidFields[EwsAttachmentPrivate::IsInline]) {
        writer.writeTextElement(ewsTypeNsUri, u"IsInline"_s, d->mIsInline ? u"true"_s : u"false"_s);
    }
    if (d->mType == FileAttachment) {
        if (d->mValidFields[EwsAttachmentPrivate::IsContactPhoto]) {
            writer.writeTextElement(ewsTypeNsUri, u"IsContactPhoto"_s, d->mIsContactPhoto ? u"true"_s : u"false"_s);
        }
        if (d->mValidFields[EwsAttachmentPrivate::Content]) {
            writer.writeTextElement(ewsTypeNsUri, u"Content"_s, QString::fromLatin1(d->mContent.toBase64()));
        }
    } else if (d->mType == ItemAttachment) {
        if (d->mValidFields[EwsAttachmentPrivate::Item]) {
            d->mItem.write(writer);
        }
    }

    writer.writeEndElement();
}

bool EwsAttachment::isValid() const
{
    return d->mValid;
}

EwsAttachment::Type EwsAttachment::type() const
{
    return d->mType;
}

void EwsAttachment::setType(Type type)
{
    d->mType = type;
}

QString EwsAttachment::id() const
{
    return d->mId;
}

void EwsAttachment::setId(const QString &id)
{
    d->mId = id;
    d->mValidFields.setBit(EwsAttachmentPrivate::Id);
}

void EwsAttachment::resetId()
{
    d->mValidFields.clearBit(EwsAttachmentPrivate::Id);
}

bool EwsAttachment::hasId() const
{
    return d->mValidFields[EwsAttachmentPrivate::Id];
}

QString EwsAttachment::name() const
{
    return d->mName;
}

void EwsAttachment::setName(const QString &name)
{
    d->mName = name;
    d->mValidFields.setBit(EwsAttachmentPrivate::Name);
}

void EwsAttachment::resetName()
{
    d->mValidFields.clearBit(EwsAttachmentPrivate::Name);
}

bool EwsAttachment::hasName() const
{
    return d->mValidFields[EwsAttachmentPrivate::Name];
}

QString EwsAttachment::contentType() const
{
    return d->mContentType;
}

void EwsAttachment::setContentType(const QString &contentType)
{
    d->mContentType = contentType;
    d->mValidFields.setBit(EwsAttachmentPrivate::ContentType);
}

void EwsAttachment::resetContentType()
{
    d->mValidFields.clearBit(EwsAttachmentPrivate::ContentType);
}

bool EwsAttachment::hasContentType() const
{
    return d->mValidFields[EwsAttachmentPrivate::ContentType];
}

QString EwsAttachment::contentId() const
{
    return d->mContentId;
}

void EwsAttachment::setContentId(const QString &contentId)
{
    d->mContentId = contentId;
    d->mValidFields.setBit(EwsAttachmentPrivate::ContentId);
}

void EwsAttachment::resetContentId()
{
    d->mValidFields.clearBit(EwsAttachmentPrivate::ContentId);
}

bool EwsAttachment::hasContentId() const
{
    return d->mValidFields[EwsAttachmentPrivate::ContentId];
}

QString EwsAttachment::contentLocation() const
{
    return d->mContentLocation;
}

void EwsAttachment::setContentLocation(const QString &contentLocation)
{
    d->mContentLocation = contentLocation;
    d->mValidFields.setBit(EwsAttachmentPrivate::ContentLocation);
}

void EwsAttachment::resetContentLocation()
{
    d->mValidFields.clearBit(EwsAttachmentPrivate::ContentLocation);
}

bool EwsAttachment::hasContentLocation() const
{
    return d->mValidFields[EwsAttachmentPrivate::ContentLocation];
}

long EwsAttachment::size() const
{
    return d->mSize;
}

void EwsAttachment::setSize(long size)
{
    d->mSize = size;
    d->mValidFields.setBit(EwsAttachmentPrivate::Size);
}

void EwsAttachment::resetSize()
{
    d->mValidFields.clearBit(EwsAttachmentPrivate::Size);
}

bool EwsAttachment::hasSize() const
{
    return d->mValidFields[EwsAttachmentPrivate::Size];
}

QDateTime EwsAttachment::lastModifiedTime() const
{
    return d->mLastModifiedTime;
}

void EwsAttachment::setLastModifiedTime(const QDateTime &time)
{
    d->mLastModifiedTime = time;
    d->mValidFields.setBit(EwsAttachmentPrivate::LastModifiedTime);
}

void EwsAttachment::resetLastModifiedTime()
{
    d->mValidFields.clearBit(EwsAttachmentPrivate::LastModifiedTime);
}

bool EwsAttachment::hasLastModifiedTime() const
{
    return d->mValidFields[EwsAttachmentPrivate::LastModifiedTime];
}

bool EwsAttachment::isInline() const
{
    return d->mIsInline;
}

void EwsAttachment::setIsInline(bool isInline)
{
    d->mIsInline = isInline;
    d->mValidFields.setBit(EwsAttachmentPrivate::IsInline);
}

void EwsAttachment::resetIsInline()
{
    d->mValidFields.clearBit(EwsAttachmentPrivate::IsInline);
}

bool EwsAttachment::hasIsInline() const
{
    return d->mValidFields[EwsAttachmentPrivate::IsInline];
}

bool EwsAttachment::isContactPhoto() const
{
    return d->mIsContactPhoto;
}

void EwsAttachment::setIsContactPhoto(bool isContactPhoto)
{
    d->mIsContactPhoto = isContactPhoto;
    d->mValidFields.setBit(EwsAttachmentPrivate::IsContactPhoto);
}

void EwsAttachment::resetIsContactPhoto()
{
    d->mValidFields.clearBit(EwsAttachmentPrivate::IsContactPhoto);
}

bool EwsAttachment::hasIsContactPhoto() const
{
    return d->mValidFields[EwsAttachmentPrivate::IsContactPhoto];
}

QByteArray EwsAttachment::content() const
{
    return d->mContent;
}

void EwsAttachment::setContent(const QByteArray &content)
{
    d->mContent = content;
    d->mValidFields.setBit(EwsAttachmentPrivate::Content);
}

void EwsAttachment::resetContent()
{
    d->mValidFields.clearBit(EwsAttachmentPrivate::Content);
}

bool EwsAttachment::hasContent() const
{
    return d->mValidFields[EwsAttachmentPrivate::Content];
}

const EwsItem &EwsAttachment::item() const
{
    return d->mItem;
}

void EwsAttachment::setItem(const EwsItem &item)
{
    d->mItem = item;
    d->mValidFields.setBit(EwsAttachmentPrivate::Item);
}

void EwsAttachment::resetItem()
{
    d->mValidFields.clearBit(EwsAttachmentPrivate::Item);
}

bool EwsAttachment::hasItem() const
{
    return d->mValidFields[EwsAttachmentPrivate::Item];
}
