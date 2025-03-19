/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsfoldershape.h"

using namespace Qt::StringLiterals;

static constexpr auto shapeNames = std::to_array({
    "IdOnly"_L1,
    "Default"_L1,
    "AllProperties"_L1,
});

void EwsFolderShape::write(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsMsgNsUri, "FolderShape"_L1);

    // Write the base shape
    writeBaseShape(writer);

    // Write properties (if any)
    writeProperties(writer);

    writer.writeEndElement();
}

void EwsFolderShape::writeBaseShape(QXmlStreamWriter &writer) const
{
    writer.writeTextElement(ewsTypeNsUri, "BaseShape"_L1, shapeNames[mBaseShape]);
}

void EwsFolderShape::writeProperties(QXmlStreamWriter &writer) const
{
    if (!mProps.isEmpty()) {
        writer.writeStartElement(ewsTypeNsUri, "AdditionalProperties"_L1);

        for (const EwsPropertyField &prop : std::as_const(mProps)) {
            prop.write(writer);
        }

        writer.writeEndElement();
    }
}
