/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsitemshape.h"

using namespace Qt::StringLiterals;

void EwsItemShape::write(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsMsgNsUri, "ItemShape"_L1);

    // Write the base shape
    writeBaseShape(writer);

    // Write the IncludeMimeContent element (if applicable)
    if (mFlags.testFlag(IncludeMimeContent)) {
        writer.writeTextElement(ewsTypeNsUri, "IncludeMimeContent"_L1, "true"_L1);
    }

    // Write the BodyType element
    if (mBodyType != BodyNone) {
        QLatin1StringView bodyTypeText;

        switch (mBodyType) {
        case BodyHtml:
            bodyTypeText = "HTML"_L1;
            break;
        case BodyText:
            bodyTypeText = "Text"_L1;
            break;
        default:
            // case BodyBest:
            bodyTypeText = "Best"_L1;
            break;
        }
        writer.writeTextElement(ewsTypeNsUri, "BodyType"_L1, bodyTypeText);
    }

    // Write the FilterHtmlContent element (if applicable)
    if (mBodyType == BodyHtml && mFlags.testFlag(FilterHtmlContent)) {
        writer.writeTextElement(ewsTypeNsUri, "FilterHtmlContent"_L1, "true"_L1);
    }

    // Write the ConvertHtmlCodePageToUTF8 element (if applicable)
    if (mBodyType == BodyHtml && mFlags.testFlag(ConvertHtmlToUtf8)) {
        writer.writeTextElement(ewsTypeNsUri, "ConvertHtmlCodePageToUTF8"_L1, "true"_L1);
    }

    // Write properties (if any)
    writeProperties(writer);

    writer.writeEndElement();
}
