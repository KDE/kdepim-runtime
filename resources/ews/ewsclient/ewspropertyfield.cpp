/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewspropertyfield.h"

#include <QString>

#include "ewsclient_debug.h"

using namespace Qt::StringLiterals;

static constexpr auto distinguishedPropSetIdNames = std::to_array({
    "Meeting"_L1,
    "Appointment"_L1,
    "Common"_L1,
    "PublicStrings"_L1,
    "Address"_L1,
    "InternetHeaders"_L1,
    "CalendarAssistant"_L1,
    "UnifiedMessaging"_L1,
});

static constexpr auto propertyTypeNames = std::to_array({
    "ApplicationTime"_L1, "ApplicationTimeArray"_L1, "Binary"_L1,     "BinaryArray"_L1,     "Boolean"_L1, "CLSID"_L1,       "CLSIDArray"_L1,
    "Currency"_L1,        "CurrencyArray"_L1,        "Double"_L1,     "DoubleArray"_L1,     "Error"_L1,   "Float"_L1,       "FloatArray"_L1,
    "Integer"_L1,         "IntegerArray"_L1,         "Long"_L1,       "LongArray"_L1,       "Null"_L1,    "Object"_L1,      "ObjectArray"_L1,
    "Short"_L1,           "ShortArray"_L1,           "SystemTime"_L1, "SystemTimeArray"_L1, "String"_L1,  "StringArray"_L1,
});

class EwsPropertyFieldPrivate : public QSharedData
{
public:
    EwsPropertyFieldPrivate()
        : mPropType(EwsPropertyField::UnknownField)
        , mIndex(0)
        , mPsIdType(DistinguishedPropSet)
        , mPsDid(EwsPropSetMeeting)
        , mIdType(PropName)
        , mId(0)
        , mHasTag(false)
        , mTag(0)
        , mType(EwsPropTypeNull)
        , mHash(0)
    {
    }

    enum PropSetIdType {
        DistinguishedPropSet,
        RealPropSet,
    };

    enum PropIdType {
        PropName,
        PropId,
    };

    EwsPropertyField::Type mPropType;

    QString mUri;
    unsigned mIndex;

    PropSetIdType mPsIdType;
    EwsDistinguishedPropSetId mPsDid;
    QString mPsId;

    PropIdType mIdType;
    unsigned mId;
    QString mName;

    bool mHasTag;
    unsigned mTag;

    EwsPropertyType mType;

    uint mHash; // Precalculated hash for the qHash() function.

    void recalcHash();
};

void EwsPropertyFieldPrivate::recalcHash()
{
    mHash = 0;
    switch (mPropType) {
    case EwsPropertyField::Field:
        mHash = 0x00000000 | (qHash(mUri) & 0x3FFFFFFF);
        break;
    case EwsPropertyField::IndexedField:
        mHash = 0x80000000 | ((qHash(mUri) ^ mIndex) & 0x3FFFFFFF);
        break;
    case EwsPropertyField::ExtendedField:
        if (mHasTag) {
            mHash = 0x40000000 | mTag;
        } else {
            if (mPsIdType == DistinguishedPropSet) {
                mHash |= mPsDid << 16;
            } else {
                mHash |= (qHash(mPsId) & 0x1FFF) << 16;
            }

            if (mIdType == PropId) {
                mHash |= mId & 0xFFFF;
            } else {
                mHash |= (qHash(mName) & 0xFFFF);
            }
            mHash |= 0xC0000000;
        }
        break;
    default:
        break;
    }
}

EwsPropertyField::EwsPropertyField()
    : d(new EwsPropertyFieldPrivate())
{
}

EwsPropertyField::EwsPropertyField(const QString &uri)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = Field;
    d->mUri = uri;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(const QString &uri, unsigned index)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = IndexedField;
    d->mUri = uri;
    d->mIndex = index;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(EwsDistinguishedPropSetId psid, unsigned id, EwsPropertyType type)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = ExtendedField;

    d->mPsIdType = EwsPropertyFieldPrivate::DistinguishedPropSet;
    d->mPsDid = psid;

    d->mIdType = EwsPropertyFieldPrivate::PropId;
    d->mId = id;

    d->mType = type;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(EwsDistinguishedPropSetId psid, const QString &name, EwsPropertyType type)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = ExtendedField;

    d->mPsIdType = EwsPropertyFieldPrivate::DistinguishedPropSet;
    d->mPsDid = psid;

    d->mIdType = EwsPropertyFieldPrivate::PropName;
    d->mName = name;

    d->mType = type;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(const QString &psid, unsigned id, EwsPropertyType type)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = ExtendedField;

    d->mPsIdType = EwsPropertyFieldPrivate::RealPropSet;
    d->mPsId = psid;

    d->mIdType = EwsPropertyFieldPrivate::PropId;
    d->mId = id;

    d->mType = type;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(const QString &psid, const QString &name, EwsPropertyType type)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = ExtendedField;

    d->mPsIdType = EwsPropertyFieldPrivate::RealPropSet;
    d->mPsId = psid;

    d->mIdType = EwsPropertyFieldPrivate::PropName;
    d->mName = name;

    d->mType = type;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(unsigned tag, EwsPropertyType type)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = ExtendedField;

    d->mHasTag = true;
    d->mTag = tag;

    d->mType = type;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(const EwsPropertyField &other)
    : d(other.d)
{
}

EwsPropertyField::EwsPropertyField(EwsPropertyField &&other)
    : d(other.d)
{
}

EwsPropertyField &EwsPropertyField::operator=(EwsPropertyField &&other)
{
    d = other.d;
    return *this;
}

EwsPropertyField::~EwsPropertyField()
{
}

EwsPropertyField &EwsPropertyField::operator=(const EwsPropertyField &other)
{
    d = other.d;
    return *this;
}

bool EwsPropertyField::operator==(const EwsPropertyField &other) const
{
    if (d == other.d) {
        return true;
    }

    const EwsPropertyFieldPrivate *od = other.d;

    if (d->mPropType != od->mPropType) {
        return false;
    }

    switch (d->mPropType) {
    case UnknownField:
        return true;
    case Field:
        return d->mUri == od->mUri;
    case IndexedField:
        return (d->mUri == od->mUri) && (d->mIndex == od->mIndex);
    case ExtendedField:
        if (d->mType != od->mType) {
            return false;
        }

        if (d->mHasTag != od->mHasTag) {
            return false;
        } else if (d->mHasTag) {
            return d->mTag == od->mTag;
        }

        if (d->mPsIdType != od->mPsIdType) {
            return false;
        } else if ((d->mPsIdType == EwsPropertyFieldPrivate::DistinguishedPropSet) && (d->mPsDid != od->mPsDid)) {
            return false;
        } else if ((d->mPsIdType == EwsPropertyFieldPrivate::RealPropSet) && (d->mPsId != od->mPsId)) {
            return false;
        }

        if (d->mIdType != od->mIdType) {
            return false;
        } else if ((d->mIdType == EwsPropertyFieldPrivate::PropId) && (d->mId != od->mId)) {
            return false;
        } else if ((d->mIdType == EwsPropertyFieldPrivate::PropName) && (d->mName != od->mName)) {
            return false;
        }
        return true;
    default:
        return false;
    }

    // Shouldn't get here.
    return false;
}

void EwsPropertyField::write(QXmlStreamWriter &writer) const
{
    switch (d->mPropType) {
    case Field:
        writer.writeStartElement(ewsTypeNsUri, "FieldURI"_L1);
        writer.writeAttribute("FieldURI"_L1, d->mUri);
        writer.writeEndElement();
        break;
    case IndexedField: {
        writer.writeStartElement(ewsTypeNsUri, "IndexedFieldURI"_L1);
        writer.writeAttribute("FieldURI"_L1, d->mUri);
        QStringList tokens = d->mUri.split(u':');
        writer.writeAttribute("FieldIndex"_L1, tokens[1] + QString::number(d->mIndex));
        writer.writeEndElement();
        break;
    }
    case ExtendedField:
        writer.writeStartElement(ewsTypeNsUri, "ExtendedFieldURI"_L1);
        if (d->mHasTag) {
            writer.writeAttribute("PropertyTag"_L1, "0x"_L1 + QString::number(d->mTag, 16));
        } else {
            if (d->mPsIdType == EwsPropertyFieldPrivate::DistinguishedPropSet) {
                writer.writeAttribute("DistinguishedPropertySetId"_L1, distinguishedPropSetIdNames[d->mPsDid]);
            } else {
                writer.writeAttribute("PropertySetId"_L1, d->mPsId);
            }

            if (d->mIdType == EwsPropertyFieldPrivate::PropId) {
                writer.writeAttribute("PropertyId"_L1, QString::number(d->mId));
            } else {
                writer.writeAttribute("PropertyName"_L1, d->mName);
            }
        }
        writer.writeAttribute("PropertyType"_L1, propertyTypeNames[d->mType]);
        writer.writeEndElement();
        break;
    case UnknownField:
        break;
    }
}

bool EwsPropertyField::read(QXmlStreamReader &reader)
{
    if (reader.namespaceUri() != ewsTypeNsUri) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading property field - invalid namespace.");
        return false;
    }

    QXmlStreamAttributes attrs = reader.attributes();
    bool ok;

    // First check the property type
    if (reader.name() == "FieldURI"_L1) {
        if (!attrs.hasAttribute("FieldURI"_L1)) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading property field - missing %1 attribute.").arg(QStringLiteral("FieldURI"));
            return false;
        }
        d->mPropType = Field;
        d->mUri = attrs.value("FieldURI"_L1).toString();
    } else if (reader.name() == "IndexedFieldURI"_L1) {
        if (!attrs.hasAttribute("FieldURI"_L1)) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading property field - missing %1 attribute.").arg(QStringLiteral("FieldURI"));
            return false;
        }
        if (!attrs.hasAttribute("FieldIndex"_L1)) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading property field - missing %1 attribute.").arg(QStringLiteral("FieldIndex"));
            return false;
        }
        QString uri = attrs.value("FieldURI"_L1).toString();
        QStringList tokens = uri.split(u':');
        QString indexStr = attrs.value("FieldIndex"_L1).toString();
        if (!indexStr.startsWith(tokens[1])) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading property field - malformed %1 attribute.").arg(QStringLiteral("FieldIndex"));
            return false;
        }
        unsigned index = QStringView(indexStr).mid(tokens[1].size()).toUInt(&ok, 0);
        if (!ok) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading property field - error reading %1 attribute.").arg(QStringLiteral("FieldIndex"));
            return false;
        }
        d->mPropType = IndexedField;
        d->mUri = uri;
        d->mIndex = index;
    } else if (reader.name() == "ExtendedFieldURI"_L1) {
        if (!attrs.hasAttribute("PropertyType"_L1)) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading property field - missing %1 attribute.").arg(QStringLiteral("PropertyType"));
            return false;
        }
        auto propTypeText = attrs.value("PropertyType"_L1);
        unsigned i;
        EwsPropertyType propType;
        for (i = 0; i < sizeof(propertyTypeNames) / sizeof(propertyTypeNames[0]); ++i) {
            if (propTypeText == propertyTypeNames[i]) {
                propType = static_cast<EwsPropertyType>(i);
                break;
            }
        }
        if (i == sizeof(propertyTypeNames) / sizeof(propertyTypeNames[0])) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading property field - error reading %1 attribute.").arg(QStringLiteral("PropertyType"));
            return false;
        }

        if (attrs.hasAttribute("PropertyTag"_L1)) {
            unsigned tag = attrs.value("PropertyTag"_L1).toUInt(&ok, 0);
            if (!ok) {
                qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading property field - error reading %1 attribute.").arg(QStringLiteral("PropertyTag"));
                return false;
            }
            d->mHasTag = true;
            d->mTag = tag;
        } else {
            EwsPropertyFieldPrivate::PropSetIdType psIdType = EwsPropertyFieldPrivate::DistinguishedPropSet;
            EwsDistinguishedPropSetId psDid = EwsPropSetMeeting;
            QString psId;

            EwsPropertyFieldPrivate::PropIdType idType = EwsPropertyFieldPrivate::PropName;
            unsigned id = 0;
            QString name;
            if (attrs.hasAttribute("PropertyId"_L1)) {
                id = attrs.value("PropertyId"_L1).toUInt(&ok, 0);
                if (!ok) {
                    qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading property field - error reading %1 attribute.").arg(QStringLiteral("PropertyId"));
                    return false;
                }
                idType = EwsPropertyFieldPrivate::PropId;
            } else if (attrs.hasAttribute("PropertyName"_L1)) {
                name = attrs.value("PropertyName"_L1).toString();
                idType = EwsPropertyFieldPrivate::PropName;
            } else {
                qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading property field - missing one of %1 or %2 attributes.")
                                               .arg(QStringLiteral("PropertyId").arg(QStringLiteral("PropertyName")));
                return false;
            }

            if (attrs.hasAttribute("DistinguishedPropertySetId"_L1)) {
                auto didText = attrs.value("DistinguishedPropertySetId"_L1);
                unsigned i;
                for (i = 0; i < sizeof(distinguishedPropSetIdNames) / sizeof(distinguishedPropSetIdNames[0]); ++i) {
                    if (didText == distinguishedPropSetIdNames[i]) {
                        psDid = static_cast<EwsDistinguishedPropSetId>(i);
                        break;
                    }
                }
                if (i == sizeof(distinguishedPropSetIdNames) / sizeof(distinguishedPropSetIdNames[0])) {
                    qCWarningNC(EWSCLI_LOG)
                        << QStringLiteral("Error reading property field - error reading %1 attribute.").arg(QStringLiteral("DistinguishedPropertySetId"));
                    return false;
                }
                psIdType = EwsPropertyFieldPrivate::DistinguishedPropSet;
            } else if (attrs.hasAttribute("PropertySetId"_L1)) {
                psId = attrs.value("PropertySetId"_L1).toString();
                psIdType = EwsPropertyFieldPrivate::RealPropSet;
            } else {
                qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading property field - missing one of %1 or %2 attributes.")
                                               .arg(QStringLiteral("DistinguishedPropertySetId").arg(QStringLiteral("PropertySetId")));
                return false;
            }
            d->mPsIdType = psIdType;
            d->mPsDid = psDid;
            d->mPsId = psId;
            d->mIdType = idType;
            d->mId = id;
            d->mName = name;
        }

        d->mType = propType;
        d->mPropType = ExtendedField;
    }
    d->recalcHash();
    return true;
}

size_t qHash(const EwsPropertyField &prop, size_t seed) noexcept
{
    return qHash(prop.d->mHasTag, seed);
}

QDebug operator<<(QDebug debug, const EwsPropertyField &prop)
{
    QDebugStateSaver saver(debug);
    QDebug d = debug.nospace().noquote();
    d << QStringLiteral("EwsPropertyField(");

    switch (prop.d->mPropType) {
    case EwsPropertyField::Field:
        d << QStringLiteral("FieldUri: ") << prop.d->mUri;
        break;
    case EwsPropertyField::IndexedField:
        d << QStringLiteral("IndexedFieldUri: ") << prop.d->mUri << '@' << prop.d->mIndex;
        break;
    case EwsPropertyField::ExtendedField:
        d << QStringLiteral("ExtendedFieldUri: ");
        if (prop.d->mHasTag) {
            d << QStringLiteral("tag: 0x") << QString::number(prop.d->mTag, 16);
        } else {
            if (prop.d->mPsIdType == EwsPropertyFieldPrivate::DistinguishedPropSet) {
                d << QStringLiteral("psdid: ") << distinguishedPropSetIdNames[prop.d->mPsDid];
            } else {
                d << QStringLiteral("psid: ") << prop.d->mPsId;
            }
            d << QStringLiteral(", ");

            if (prop.d->mIdType == EwsPropertyFieldPrivate::PropId) {
                d << QStringLiteral("id: 0x") << QString::number(prop.d->mId, 16);
            } else {
                d << QStringLiteral("name: ") << prop.d->mName;
            }
        }
        d << QStringLiteral(", type: ") << propertyTypeNames[prop.d->mType];
        break;
    case EwsPropertyField::UnknownField:
        d << QStringLiteral("Unknown");
        break;
    }
    d << ')';
    return debug;
}

bool EwsPropertyField::writeWithValue(QXmlStreamWriter &writer, const QVariant &value) const
{
    switch (d->mPropType) {
    case Field: {
        QStringList tokens = d->mUri.split(u':');
        if (tokens.size() != 2) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Invalid field URI: %1").arg(d->mUri);
            return false;
        }
        writer.writeStartElement(ewsTypeNsUri, tokens[1]);
        writeValue(writer, value);
        writer.writeEndElement();
        break;
    }
    case IndexedField: {
        QStringList tokens = d->mUri.split(u':');
        if (tokens.size() != 2) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Invalid field URI: %1").arg(d->mUri);
            return false;
        }
        writer.writeStartElement(ewsTypeNsUri, tokens[1] + QStringLiteral("es"));
        writer.writeStartElement(ewsTypeNsUri, "Entry"_L1);
        writer.writeAttribute("Key"_L1, tokens[1] + QString::number(d->mIndex));
        writeValue(writer, value);
        writer.writeEndElement();
        writer.writeEndElement();
        break;
    }
    case ExtendedField:
        writer.writeStartElement(ewsTypeNsUri, "ExtendedProperty"_L1);
        write(writer);
        writeExtendedValue(writer, value);
        writer.writeEndElement();
        break;
    default:
        return false;
    }

    return true;
}

void EwsPropertyField::writeValue(QXmlStreamWriter &writer, const QVariant &value) const
{
    switch (value.userType()) {
    case QMetaType::QStringList: {
        const QStringList list = value.toStringList();
        for (const QString &str : list) {
            writer.writeTextElement(ewsTypeNsUri, "String"_L1, str);
        }
        break;
    }
    case QMetaType::QString:
        writer.writeCharacters(value.toString());
        break;
    default:
        qCWarning(EWSCLI_LOG) << QStringLiteral("Unknown variant type to write: %1").arg(QString::fromLatin1(value.typeName()));
    }
}

void EwsPropertyField::writeExtendedValue(QXmlStreamWriter &writer, const QVariant &value) const
{
    switch (value.userType()) {
    case QMetaType::QStringList: {
        const QStringList list = value.toStringList();
        writer.writeStartElement(ewsTypeNsUri, "Values"_L1);
        for (const QString &str : list) {
            writer.writeTextElement(ewsTypeNsUri, "Value"_L1, str);
        }
        writer.writeEndElement();
        break;
    }
    case QMetaType::QString:
        writer.writeStartElement(ewsTypeNsUri, "Value"_L1);
        writer.writeCharacters(value.toString());
        writer.writeEndElement();
        break;
    default:
        qCWarning(EWSCLI_LOG) << QStringLiteral("Unknown variant type to write: %1").arg(QString::fromLatin1(value.typeName()));
    }
}

EwsPropertyField::Type EwsPropertyField::type() const
{
    return d->mPropType;
}

QString EwsPropertyField::uri() const
{
    if (d->mPropType == Field || d->mPropType == IndexedField) {
        return d->mUri;
    } else {
        return QString();
    }
}
