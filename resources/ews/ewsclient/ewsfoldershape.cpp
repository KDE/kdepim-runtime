/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsfoldershape.h"

static constexpr auto shapeNames = std::to_array({
    QLatin1StringView("IdOnly"),
    QLatin1StringView("Default"),
    QLatin1StringView("AllProperties"),
});

void EwsFolderShape::write(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("FolderShape"));

    // Write the base shape
    writeBaseShape(writer);

    // Write properties (if any)
    writeProperties(writer);

    writer.writeEndElement();
}

void EwsFolderShape::writeBaseShape(QXmlStreamWriter &writer) const
{
    writer.writeTextElement(ewsTypeNsUri, QStringLiteral("BaseShape"), shapeNames[mBaseShape]);
}

void EwsFolderShape::writeProperties(QXmlStreamWriter &writer) const
{
    if (!mProps.isEmpty()) {
        writer.writeStartElement(ewsTypeNsUri, QStringLiteral("AdditionalProperties"));

        for (const EwsPropertyField &prop : std::as_const(mProps)) {
            prop.write(writer);
        }

        writer.writeEndElement();
    }
}
